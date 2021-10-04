/*
 * Copyright (c) Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <exosphere.hpp>
#include "../fusee_fatal.hpp"
#include "fusee_mtc.hpp"
#include "fusee_mtc_timing_table_erista.hpp"

namespace ams::nxboot {

    namespace {

        constexpr inline const uintptr_t CLKRST  = secmon::MemoryRegionPhysicalDeviceClkRst.GetAddress();
        constexpr inline const uintptr_t MC      = MC_BASE;
        constexpr inline const uintptr_t EMC     = EMC_BASE;
        constexpr inline const uintptr_t EMC0    = EMC0_BASE;
        constexpr inline const uintptr_t EMC1    = EMC1_BASE;

        static constinit bool g_next_pll = false;
        static constinit bool g_did_first_training = false;
        static constinit bool g_fsp_for_next_freq = false;

        #include "fusee_mtc_tables_erista.inc"
        #include "fusee_mtc_ram_training_pattern.inc"

        #define DECLARE_OFFSET_HANDLER(BASE, REG, NAME) REG,
        #define DECLARE_REGISTER_HANDLER(BASE, REG, NAME) BASE + REG,

        constexpr inline const u16 BurstRegistersOffsets[] = {
            FOREACH_BURST_REG(DECLARE_OFFSET_HANDLER)
        };

        constexpr inline const u32 TrimRegisters[] = {
            FOREACH_TRIM_REG(DECLARE_REGISTER_HANDLER)
        };

        constexpr inline const u32 BurstMcRegisters[] = {
            FOREACH_BURST_MC_REG(DECLARE_REGISTER_HANDLER)
        };

        constexpr inline const u32 LaScaleRegisters[] = {
            FOREACH_LA_SCALE_REG(DECLARE_REGISTER_HANDLER)
        };

        constexpr inline const u32 PerChannelTrimRegisters[] = {
            FOREACH_PER_CHANNEL_TRIM_REG(DECLARE_REGISTER_HANDLER)
        };

        constexpr inline const u32 PerChannelBurstRegisters[] = {
            FOREACH_PER_CHANNEL_BURST_REG(DECLARE_REGISTER_HANDLER)
        };

        constexpr inline const u32 PerChannelVrefRegisters[] = {
            FOREACH_PER_CHANNEL_VREF_REG(DECLARE_REGISTER_HANDLER)
        };

        constexpr inline const u32 PerChannelTrainingModRegisters[] = {
            FOREACH_PER_CHANNEL_TRAINING_MOD_REG(DECLARE_REGISTER_HANDLER)
        };

        using EmcDvfsTimingTable = erista::EmcDvfsTimingTable;

        EmcDvfsTimingTable *GetEmcDvfsTimingTables(int index, void *mtc_tables_buffer) {
            switch (index) {
                case 0:
                case 3:
                    //std::memcpy(mtc_tables_buffer, T210SdevEmcDvfsTableS4gb01, sizeof(T210SdevEmcDvfsTableS4gb01));
                    return reinterpret_cast<EmcDvfsTimingTable *>(const_cast<u8 *>(T210SdevEmcDvfsTableS4gb01));
                case 1:
                    //std::memcpy(mtc_tables_buffer, T210SdevEmcDvfsTableS6gb01, sizeof(T210SdevEmcDvfsTableS6gb01));
                    return reinterpret_cast<EmcDvfsTimingTable *>(const_cast<u8 *>(T210SdevEmcDvfsTableS6gb01));
                case 2:
                    //std::memcpy(mtc_tables_buffer, T210SdevEmcDvfsTableH4gb01, sizeof(T210SdevEmcDvfsTableH4gb01));
                    return reinterpret_cast<EmcDvfsTimingTable *>(const_cast<u8 *>(T210SdevEmcDvfsTableH4gb01));
                default:
                    ShowFatalError("Unknown EmcDvfsTimingTableIndex: %d\n", index);
            }
        }

        bool IsSamePll(u32 next_2x, u32 prev_2x) {
            if (next_2x == prev_2x) {
                return true;
            } else if ((next_2x == PLLM_OUT0 || next_2x == PLLM_UD) && (prev_2x == PLLM_OUT0 || prev_2x == PLLM_UD)) {
                return true;
            } else {
                return false;
            }
        }

        bool PllReprogram(u32 next_rate_khz, u32 next_clk_src, u32 prev_rate_khz, u32 prev_clk_src) {
            /* Get current pll/divp value. */
            u32 pll_base, pll_p;
            switch (reg::GetValue(CLKRST + CLK_RST_CONTROLLER_CLK_SOURCE_EMC, CLK_RST_REG_BITS_MASK(CLK_SOURCE_EMC_EMC_2X_CLK_SRC))) {
                case PLLM_UD:
                case PLLM_OUT0:
                    pll_base = reg::Read(CLKRST + CLK_RST_CONTROLLER_PLLM_BASE);
                    pll_p    = reg::GetField(pll_base, CLK_RST_REG_BITS_MASK(PLLM_BASE_PLLM_DIVP));
                    break;
                case PLLMB_UD:
                case PLLMB_OUT0:
                    pll_base = reg::Read(CLKRST + CLK_RST_CONTROLLER_PLLMB_BASE);
                    pll_p    = reg::GetField(pll_base, CLK_RST_REG_BITS_MASK(PLLMB_BASE_PLLMB_DIVP));
                    break;
                default:
                    pll_base = 0;
                    pll_p    = 0;
            }

            /* Check pll divp. */
            if (pll_p > 5) {
                ShowFatalError("Invalid PLL divp: %" PRIu32 "\n", pll_p);
            }

            /* Get clk src/divisor. */
            const u32 next_2x  = reg::GetField(next_clk_src, CLK_RST_REG_BITS_MASK(CLK_SOURCE_EMC_EMC_2X_CLK_SRC));
            const u32 prev_2x  = reg::GetField(prev_clk_src, CLK_RST_REG_BITS_MASK(CLK_SOURCE_EMC_EMC_2X_CLK_SRC));
            u32 next_div = reg::GetField(next_clk_src, CLK_RST_REG_BITS_MASK(CLK_SOURCE_EMC_EMC_2X_CLK_DIVISOR));
            u32 prev_div = reg::GetField(prev_clk_src, CLK_RST_REG_BITS_MASK(CLK_SOURCE_EMC_EMC_2X_CLK_DIVISOR));

            /* Update divisor, if necessary. */
            if (next_2x == PLLM_UD || next_2x == PLLMB_UD) {
                next_div = 0;
            }
            if (prev_2x == PLLM_UD || prev_2x == PLLMB_UD) {
                prev_div = 0;
            }

            /* If the pll is different, reprogramming is necessary. */
            if (!IsSamePll(next_2x, prev_2x)) {
                return true;
            }

            /* Return whether the ratios are different. */
            const float next_freq = next_rate_khz * (1 + (next_div >> 1) + (0.5 * (next_div & 1))) * (pll_p + 1);
            const float prev_freq = prev_rate_khz * (1 + (prev_div >> 1) + (0.5 * (prev_div & 1))) * (pll_p + 1);

            const float ratio = prev_freq / next_freq;

            return ratio > 1.01 || ratio < 0.99;
        }

        u32 ProgramPllm(u32 next_rate_khz, u32 next_clk_src, bool is_pllmb) {
            /* Hardcode values for 1600MHz. */
            u32 divn, divm, divp;
            if (next_rate_khz == 1600000) {
                divn = 0x7D;
                divm = 0x03;
                divp = 0x00;
            } else if (next_rate_khz == 800000) {
                divn = 0x7D;
                divm = 0x03;
                divp = 0x01;
            } else {
                ShowFatalError("Unexpected ProgramPllm next rate %" PRIu32 "\n", next_rate_khz);
            }

            const auto next_2x = reg::GetField(next_clk_src, CLK_RST_REG_BITS_MASK(CLK_SOURCE_EMC_EMC_2X_CLK_SRC));
            if (is_pllmb) {
                /* Set divisors. */
                reg::Write(CLKRST + CLK_RST_CONTROLLER_PLLMB_BASE, CLK_RST_REG_BITS_VALUE(PLLMB_BASE_PLLMB_DIVM, divm),
                                                                   CLK_RST_REG_BITS_VALUE(PLLMB_BASE_PLLMB_DIVN, divn),
                                                                   CLK_RST_REG_BITS_VALUE(PLLMB_BASE_PLLMB_DIVP, divp));
                reg::Read(CLKRST + CLK_RST_CONTROLLER_PLLMB_BASE);

                /* Set enable. */
                reg::ReadWrite(CLKRST + CLK_RST_CONTROLLER_PLLMB_BASE, CLK_RST_REG_BITS_ENUM(PLLMB_BASE_PLLMB_ENABLE, ENABLE));

                /* Adjust next clock source. */
                if (next_2x == PLLM_UD) {
                    reg::SetField(next_clk_src, CLK_RST_REG_BITS_VALUE(CLK_SOURCE_EMC_EMC_2X_CLK_SRC, PLLMB_UD));
                } else if (next_2x == PLLM_OUT0) {
                    reg::SetField(next_clk_src, CLK_RST_REG_BITS_VALUE(CLK_SOURCE_EMC_EMC_2X_CLK_SRC, PLLMB_OUT0));
                }

                /* Wait for pll to lock. */
                while (!reg::HasValue(CLKRST + CLK_RST_CONTROLLER_PLLMB_BASE, CLK_RST_REG_BITS_ENUM(PLLMB_BASE_PLLMB_LOCK, LOCK))) {
                    /* ... */
                }
            } else {
                /* Set divisors. */
                reg::Write(CLKRST + CLK_RST_CONTROLLER_PLLM_BASE, CLK_RST_REG_BITS_VALUE(PLLM_BASE_PLLM_DIVM, divm),
                                                                  CLK_RST_REG_BITS_VALUE(PLLM_BASE_PLLM_DIVN, divn),
                                                                  CLK_RST_REG_BITS_VALUE(PLLM_BASE_PLLM_DIVP, divp));
                reg::Read(CLKRST + CLK_RST_CONTROLLER_PLLM_BASE);

                /* Set LKCDET. */
                reg::ReadWrite(CLKRST + CLK_RST_CONTROLLER_PLLM_MISC2, CLK_RST_REG_BITS_ENUM(PLLM_MISC2_PLLM_EN_LCKDET, ENABLE));

                /* Set enable. */
                reg::ReadWrite(CLKRST + CLK_RST_CONTROLLER_PLLM_BASE, CLK_RST_REG_BITS_ENUM(PLLM_BASE_PLLM_ENABLE, ENABLE));

                /* Adjust next clock source. */
                if (next_2x == PLLM_UD) {
                    reg::SetField(next_clk_src, CLK_RST_REG_BITS_VALUE(CLK_SOURCE_EMC_EMC_2X_CLK_SRC, PLLM_UD));
                } else if (next_2x == PLLM_OUT0) {
                    reg::SetField(next_clk_src, CLK_RST_REG_BITS_VALUE(CLK_SOURCE_EMC_EMC_2X_CLK_SRC, PLLM_OUT0));
                }

                /* Wait for pll to lock. */
                while (!reg::HasValue(CLKRST + CLK_RST_CONTROLLER_PLLM_BASE, CLK_RST_REG_BITS_ENUM(PLLM_BASE_PLLM_LOCK, LOCK))) {
                    /* ... */
                }
            }

            return next_clk_src;
        }

        u32 GetDllState(EmcDvfsTimingTable *timing) {
            return (!(timing->emc_emrs & 0x1)) ? DLL_ON : DLL_OFF;
        }

        int WaitForUpdate(u32 reg_offset, u32 mask, bool updated, u32 fbio_cfg7) {
            constexpr int StatusUpdateTimeout = 1000;

            int result = 0;

            if (true /* reg::HasValue(fbio_cfg7, EMC_REG_BITS_ENUM(FBIO_CFG7_CH0_ENABLE, ENABLE)) */) {
                bool success = false;
                for (int i = 0; i < StatusUpdateTimeout; ++i) {
                    if (((reg::Read(EMC + reg_offset) & mask) != 0) == updated) {
                        success = true;
                        break;
                    }
                    util::WaitMicroSeconds(1);
                }
                result |= success ? 0 : 4;
            }

            if (reg::GetField(fbio_cfg7, EMC_REG_BITS_MASK(FBIO_CFG7_CH1_ENABLE)) == EMC_FBIO_CFG7_CH1_ENABLE_ENABLE) {
                bool success = false;
                for (int i = 0; i < StatusUpdateTimeout; ++i) {
                    if (((reg::Read(EMC1 + reg_offset) & mask) != 0) == updated) {
                        success = true;
                        break;
                    }
                    util::WaitMicroSeconds(1);
                }
                result |= success ? 0 : 4;
            }

            return result;
        }

        void TimingUpdate(u32 fbio_cfg7) {
            /* Trigger the timing update event. */
            reg::Write(EMC + EMC_TIMING_CONTROL, 1);

            /* Wait for the update to finish. */
            WaitForUpdate(EMC_EMC_STATUS, 0x800000, false, fbio_cfg7);
        }

        void DllDisable(u32 fbio_cfg7) {
            /* Disable dll. */
            reg::ClearBits(EMC + EMC_CFG_DIG_DLL, 0x1);

            /* Request a timing update event */
            TimingUpdate(fbio_cfg7);

            /* Wait until CFG_DLL_EN is cleared. */
            WaitForUpdate(EMC_CFG_DIG_DLL, 0x1, false, fbio_cfg7);
        }

        void DllEnableStall(u32 fbio_cfg7) {
            /* Enable DLL */
            uint32_t emc_cfg_dig_dll = (reg::Read(EMC + EMC_CFG_DIG_DLL) & 0xFFFFFF24) | 0x89;
            reg::Write(EMC + EMC_CFG_DIG_DLL, emc_cfg_dig_dll);

            /* Request a timing update event */
            TimingUpdate(fbio_cfg7);

            /* Wait until CFG_DLL_EN is set for EMC */
            WaitForUpdate(EMC_CFG_DIG_DLL, 0x1, true, fbio_cfg7);
        }

        void ChangeDllSrc(EmcDvfsTimingTable *dst_timing, u32 next_clk_src) {
            u32 dll_setting = ((next_clk_src & 0xE00000FF) | (dst_timing->dll_clk_src & 0x1FFFFF00)) & 0xFFFFF3FF;
            switch (reg::GetField(next_clk_src, CLK_RST_REG_BITS_MASK(CLK_SOURCE_EMC_EMC_2X_CLK_SRC))) {
                case PLLMB_UD:
                    dll_setting |= 0x400; /* PLLM_VCOB */
                    break;
                case PLLM_UD:
                    dll_setting |= 0x000; /* PLLM_VCOA */
                    break;
                default:
                    dll_setting |= 0x800; /* EMC_DLL_SWITCH_OUT */
                    break;
            }

            reg::Write(CLKRST + CLK_RST_CONTROLLER_CLK_SOURCE_EMC_DLL, dll_setting);
            reg::Read(CLKRST + CLK_RST_CONTROLLER_CLK_OUT_ENB_X);
            util::WaitMicroSeconds(2);

            if (dst_timing->clk_out_enb_x_0_clk_enb_emc_dll) {
                reg::Write(CLKRST + CLK_RST_CONTROLLER_CLK_ENB_X_SET, CLK_RST_REG_BITS_ENUM(CLK_ENB_X_CLK_ENB_EMC_DLL, ENABLE));
            } else {
                reg::Write(CLKRST + CLK_RST_CONTROLLER_CLK_ENB_X_CLR, CLK_RST_REG_BITS_ENUM(CLK_ENB_X_CLK_ENB_EMC_DLL, ENABLE));
            }
            reg::Read(CLKRST + CLK_RST_CONTROLLER_CLK_OUT_ENB_X);
            util::WaitMicroSeconds(2);
        }

        void SetShadowBypass(u32 val) {
            reg::ReadWrite(EMC + EMC_DBG, EMC_REG_BITS_VALUE(DBG_WRITE_MUX, val));
        }

        u32 DllPrelock(EmcDvfsTimingTable *dst_timing, bool training_enabled, u32 next_clk_src) {
            /* Check for dual channel LPDDR4 */
            u32 fbio_cfg7 = reg::Read(EMC + EMC_FBIO_CFG7);

            uint32_t emc_dig_dll_status = 0;
            uint32_t emc_cfg_dig_dll = (reg::Read(EMC1 + EMC_CFG_DIG_DLL) & 0xFFFFF824) | 0x3C8;

            /* Update EMC_CFG_DIG_DLL_0 */
            reg::Write(EMC + EMC_CFG_DIG_DLL, emc_cfg_dig_dll);

            /* Request a timing update event */
            TimingUpdate(fbio_cfg7);

            /* Wait until CFG_DLL_EN is cleared for EMC */
            WaitForUpdate(EMC_CFG_DIG_DLL, 0x1, false, fbio_cfg7);

            reg::Write(EMC + EMC_DLL_CFG_0, dst_timing->burst_regs.emc_dll_cfg_0);
            reg::Write(EMC + EMC_DLL_CFG_1, dst_timing->burst_regs.emc_dll_cfg_1);

            /* Configure the clock and reset controller for EMC DLL */
            ChangeDllSrc(dst_timing, next_clk_src);

            /* Enable DLL */
            reg::SetBits(EMC + EMC_CFG_DIG_DLL, 0x1);

            /* Request a timing update event */
            TimingUpdate(fbio_cfg7);

            /* Wait until CFG_DLL_EN is set for EMC */
            WaitForUpdate(EMC_CFG_DIG_DLL, 0x1, true, fbio_cfg7);

            /* Wait until DLL_PRIV_UPDATED or DLL_LOCK have been cleared */
            do {
                emc_dig_dll_status = reg::Read(EMC + EMC_DIG_DLL_STATUS);
            } while ((~emc_dig_dll_status & 0x28000) != 0);

            if (training_enabled) {
                /* Set WRITE_MUX to ACTIVE */
                SetShadowBypass(ACTIVE);

                /* Disable DLL */
                reg::ClearBits(EMC + EMC_CFG_DIG_DLL, 0x1);

                /* Set WRITE_MUX to ASSEMBLY */
                SetShadowBypass(ASSEMBLY);

                /* Wait until CFG_DLL_EN is cleared for EMC */
                WaitForUpdate(EMC_CFG_DIG_DLL, 0x1, false, fbio_cfg7);
            }

            /* Return the DLL_OUT value */
            return (reg::Read(EMC1 + EMC_DIG_DLL_STATUS) & 0x7FF);
        }

        void CcfifoWrite(u32 addr, u32 data, u32 wait) {
            reg::Write(EMC + EMC_CCFIFO_DATA, data);
            reg::Write(EMC + EMC_CCFIFO_ADDR, (addr & 0xFFFF) | ((wait & 0x7FFF) << 16) | 0x80000000);
        }

        u32 ActualOscClocks(u32 in) {
            if (in < 0x40) {
                return in * 0x10;
            } else if (in < 0x80) {
                return 0x800;
            } else if (in < 0xC0) {
                return 0x1000;
            } else {
                return 0x2000;
            }
        }

        void StartPeriodicCompensation() {
            reg::Write(EMC + EMC_MPC, 0x4B);
            reg::Read(EMC + EMC_MPC);
        }

        u32 UpdateClockTreeDelay(EmcDvfsTimingTable *src_timing, EmcDvfsTimingTable *dst_timing, u32 dram_dev_num, u32 fbio_cfg7, int type) {
            uint32_t mrr_req = 0, mrr_data = 0;
            uint32_t temp0_0 = 0, temp0_1 = 0, temp1_0 = 0, temp1_1 = 0;
            int tdel = 0, tmdel = 0, adel = 0;
            uint32_t cval;
            uint32_t src_timing_rate_mhz = (src_timing->rate_khz / 1000);
            uint32_t dst_timing_rate_mhz = (dst_timing->rate_khz / 1000);
            bool dvfs_pt1 = (type == DVFS_PT1);
            bool training_pt1 = (type == TRAINING_PT1);
            bool dvfs_update = (type == DVFS_UPDATE);
            bool training_update = (type == TRAINING_UPDATE);
            bool periodic_training_update = (type == PERIODIC_TRAINING_UPDATE);

            const bool dual_channel = reg::GetField(fbio_cfg7, EMC_REG_BITS_MASK(FBIO_CFG7_CH1_ENABLE)) == EMC_FBIO_CFG7_CH1_ENABLE_ENABLE;

            /* Dev0 MSB. */
            if (dvfs_pt1 || training_pt1 || periodic_training_update) {
                mrr_req = ((2 << 30) | (19 << 16));
                reg::Write(EMC + EMC_MRR, mrr_req);

                WaitForUpdate(EMC_EMC_STATUS, 0x100000, true, fbio_cfg7);

                mrr_data = ((reg::Read(EMC + EMC_MRR) & 0xFFFF) << 0);

                temp0_0 = ((mrr_data & 0xff) << 8);
                temp0_1 = (mrr_data & 0xff00);

                if (dual_channel) {
                    mrr_data = ((reg::Read(EMC1 + EMC_MRR) & 0xFFFF) << 0);
                    temp1_0 = ((mrr_data & 0xff) << 8);
                    temp1_1 = (mrr_data & 0xff00);
                }

                /* Dev0 LSB. */
                mrr_req = ((mrr_req & ~0xFF0000) | (18 << 16));
                reg::Write(EMC + EMC_MRR, mrr_req);

                WaitForUpdate(EMC_EMC_STATUS, 0x100000, true, fbio_cfg7);

                mrr_data = ((reg::Read(EMC + EMC_MRR) & 0xFFFF) << 0);

                temp0_0 |= (mrr_data & 0xff);
                temp0_1 |= ((mrr_data & 0xff00) >> 8);

                if (dual_channel) {
                    mrr_data = ((reg::Read(EMC1 + EMC_MRR) & 0xFFFF) << 0);
                    temp1_0 |= (mrr_data & 0xff);
                    temp1_1 |= ((mrr_data & 0xff00) >> 8);
                }
            }

            cval = ((1000000 * ActualOscClocks(src_timing->run_clocks)) / (src_timing_rate_mhz * 2 * temp0_0));

            if (dvfs_pt1 || training_pt1)
                __INCREMENT_PTFV(c0d0u0, cval);
            else if (dvfs_update)
                __AVERAGE_PTFV(c0d0u0);
            else if (training_update)
                __AVERAGE_WRITE_PTFV(c0d0u0);
            else if (periodic_training_update)
                __WEIGHTED_UPDATE_PTFV(c0d0u0, cval);

            if (dvfs_update || training_update || periodic_training_update) {
                tdel = (dst_timing->current_dram_clktree_c0d0u0 - __MOVAVG_AC(dst_timing, c0d0u0));
                tmdel = (tdel < 0) ? -1 * tdel : tdel;
                adel = tmdel;
                if ((tmdel * 128 * dst_timing_rate_mhz / 1000000) > dst_timing->tree_margin)
                    dst_timing->current_dram_clktree_c0d0u0 = __MOVAVG_AC(dst_timing, c0d0u0);
            }

            cval = ((1000000 * ActualOscClocks(src_timing->run_clocks)) /
                (src_timing_rate_mhz * 2 * temp0_1));

            if (dvfs_pt1 || training_pt1)
                __INCREMENT_PTFV(c0d0u1, cval);
            else if (dvfs_update)
                __AVERAGE_PTFV(c0d0u1);
            else if (training_update)
                __AVERAGE_WRITE_PTFV(c0d0u1);
            else if (periodic_training_update)
                __WEIGHTED_UPDATE_PTFV(c0d0u1, cval);

            if (dvfs_update || training_update || periodic_training_update) {
                tdel = dst_timing->current_dram_clktree_c0d0u1 - __MOVAVG_AC(dst_timing, c0d0u1);
                tmdel = (tdel < 0) ? -1 * tdel : tdel;

                if (tmdel > adel)
                    adel = tmdel;

                if ((tmdel * 128 * dst_timing_rate_mhz / 1000000) > dst_timing->tree_margin)
                    dst_timing->current_dram_clktree_c0d0u1 = __MOVAVG_AC(dst_timing, c0d0u1);
            }

            if (dual_channel) {
                cval = ((1000000 * ActualOscClocks(src_timing->run_clocks)) / (src_timing_rate_mhz * 2 * temp1_0));

                if (dvfs_pt1 || training_pt1)
                    __INCREMENT_PTFV(c1d0u0, cval);
                else if (dvfs_update)
                    __AVERAGE_PTFV(c1d0u0);
                else if (training_update)
                    __AVERAGE_WRITE_PTFV(c1d0u0);
                else if (periodic_training_update)
                    __WEIGHTED_UPDATE_PTFV(c1d0u0, cval);

                if (dvfs_update || training_update || periodic_training_update) {
                    tdel = dst_timing->current_dram_clktree_c1d0u0 - __MOVAVG_AC(dst_timing, c1d0u0);
                    tmdel = (tdel < 0) ? -1 * tdel : tdel;

                    if (tmdel > adel)
                        adel = tmdel;

                    if ((tmdel * 128 * dst_timing_rate_mhz / 1000000) > dst_timing->tree_margin)
                        dst_timing->current_dram_clktree_c1d0u0 = __MOVAVG_AC(dst_timing, c1d0u0);
                }

                cval = ((1000000 * ActualOscClocks(src_timing->run_clocks)) / (src_timing_rate_mhz * 2 * temp1_1));

                if (dvfs_pt1 || training_pt1)
                    __INCREMENT_PTFV(c1d0u1, cval);
                else if (dvfs_update)
                    __AVERAGE_PTFV(c1d0u1);
                else if (training_update)
                    __AVERAGE_WRITE_PTFV(c1d0u1);
                else if (periodic_training_update)
                    __WEIGHTED_UPDATE_PTFV(c1d0u1, cval);

                if (dvfs_update || training_update || periodic_training_update) {
                    tdel = dst_timing->current_dram_clktree_c1d0u1 - __MOVAVG_AC(dst_timing, c1d0u1);
                    tmdel = (tdel < 0) ? -1 * tdel : tdel;

                    if (tmdel > adel)
                        adel = tmdel;

                    if ((tmdel * 128 * dst_timing_rate_mhz / 1000000) > dst_timing->tree_margin)
                        dst_timing->current_dram_clktree_c1d0u1 = __MOVAVG_AC(dst_timing, c1d0u1);
                }
            }

            if (dram_dev_num != TWO_RANK)
                return adel;

            /* Dev1 MSB. */
            if (dvfs_pt1 || training_pt1 || periodic_training_update) {
                mrr_req = ((1 << 30) | (19 << 16));
                reg::Write(EMC + EMC_MRR, mrr_req);

                WaitForUpdate(EMC_EMC_STATUS, 0x100000, true, fbio_cfg7);

                mrr_data = ((reg::Read(EMC + EMC_MRR) & 0xFFFF) << 0);

                temp0_0 = ((mrr_data & 0xff) << 8);
                temp0_1 = (mrr_data & 0xff00);

                if (dual_channel) {
                    mrr_data = ((reg::Read(EMC1 + EMC_MRR) & 0xFFFF) << 0);
                    temp1_0 = ((mrr_data & 0xff) << 8);
                    temp1_1 = (mrr_data & 0xff00);
                }

                /* Dev1 LSB. */
                mrr_req = ((mrr_req & ~0xFF0000) | (18 << 16));
                reg::Write(EMC + EMC_MRR, mrr_req);

                WaitForUpdate(EMC_EMC_STATUS, 0x100000, true, fbio_cfg7);

                mrr_data = ((reg::Read(EMC + EMC_MRR) & 0xFFFF) << 0);

                temp0_0 |= (mrr_data & 0xff);
                temp0_1 |= ((mrr_data & 0xff00) >> 8);

                if (dual_channel) {
                    mrr_data = ((reg::Read(EMC1 + EMC_MRR) & 0xFFFF) << 0);
                    temp1_0 |= (mrr_data & 0xff);
                    temp1_1 |= ((mrr_data & 0xff00) >> 8);
                }
            }

            cval = ((1000000 * ActualOscClocks(src_timing->run_clocks)) / (src_timing_rate_mhz * 2 * temp0_0));

            if (dvfs_pt1 || training_pt1)
                __INCREMENT_PTFV(c0d1u0, cval);
            else if (dvfs_update)
                __AVERAGE_PTFV(c0d1u0);
            else if (training_update)
                __AVERAGE_WRITE_PTFV(c0d1u0);
            else if (periodic_training_update)
                __WEIGHTED_UPDATE_PTFV(c0d1u0, cval);

            if (dvfs_update || training_update || periodic_training_update) {
                tdel = dst_timing->current_dram_clktree_c0d1u0 - __MOVAVG_AC(dst_timing, c0d1u0);
                tmdel = (tdel < 0) ? -1 * tdel : tdel;

                if (tmdel > adel)
                    adel = tmdel;

                if ((tmdel * 128 * dst_timing_rate_mhz / 1000000) > dst_timing->tree_margin)
                    dst_timing->current_dram_clktree_c0d1u0 = __MOVAVG_AC(dst_timing, c0d1u0);
            }

            cval = ((1000000 * ActualOscClocks(src_timing->run_clocks)) / (src_timing_rate_mhz * 2 * temp0_1));

            if (dvfs_pt1 || training_pt1)
                __INCREMENT_PTFV(c0d1u1, cval);
            else if (dvfs_update)
                __AVERAGE_PTFV(c0d1u1);
            else if (training_update)
                __AVERAGE_WRITE_PTFV(c0d1u1);
            else if (periodic_training_update)
                __WEIGHTED_UPDATE_PTFV(c0d1u1, cval);

            if (dvfs_update || training_update || periodic_training_update) {
                tdel = dst_timing->current_dram_clktree_c0d1u1 - __MOVAVG_AC(dst_timing, c0d1u1);
                tmdel = (tdel < 0) ? -1 * tdel : tdel;

                if (tmdel > adel)
                    adel = tmdel;

                if ((tmdel * 128 * dst_timing_rate_mhz / 1000000) > dst_timing->tree_margin)
                    dst_timing->current_dram_clktree_c0d1u1 = __MOVAVG_AC(dst_timing, c0d1u1);
            }

            if (dual_channel) {
                cval = ((1000000 * ActualOscClocks(src_timing->run_clocks)) / (src_timing_rate_mhz * 2 * temp1_0));

                if (dvfs_pt1 || training_pt1)
                    __INCREMENT_PTFV(c1d1u0, cval);
                else if (dvfs_update)
                    __AVERAGE_PTFV(c1d1u0);
                else if (training_update)
                    __AVERAGE_WRITE_PTFV(c1d1u0);
                else if (periodic_training_update)
                    __WEIGHTED_UPDATE_PTFV(c1d1u0, cval);

                if (dvfs_update || training_update || periodic_training_update) {
                    tdel = dst_timing->current_dram_clktree_c1d1u0 - __MOVAVG_AC(dst_timing, c1d1u0);

                    tmdel = (tdel < 0) ? -1 * tdel : tdel;

                    if (tmdel > adel)
                        adel = tmdel;

                    if ((tmdel * 128 * dst_timing_rate_mhz / 1000000) > dst_timing->tree_margin)
                        dst_timing->current_dram_clktree_c1d1u0 = __MOVAVG_AC(dst_timing, c1d1u0);
                }

                cval = ((1000000 * ActualOscClocks(src_timing->run_clocks)) / (src_timing_rate_mhz * 2 * temp1_1));

                if (dvfs_pt1 || training_pt1)
                    __INCREMENT_PTFV(c1d1u1, cval);
                else if (dvfs_update)
                    __AVERAGE_PTFV(c1d1u1);
                else if (training_update)
                    __AVERAGE_WRITE_PTFV(c1d1u1);
                else if (periodic_training_update)
                    __WEIGHTED_UPDATE_PTFV(c1d1u1, cval);

                if (dvfs_update || training_update || periodic_training_update) {
                    tdel = dst_timing->current_dram_clktree_c1d1u1 - __MOVAVG_AC(dst_timing, c1d1u1);
                    tmdel = (tdel < 0) ? -1 * tdel : tdel;

                    if (tmdel > adel)
                        adel = tmdel;

                    if ((tmdel * 128 * dst_timing_rate_mhz / 1000000) > dst_timing->tree_margin)
                        dst_timing->current_dram_clktree_c1d1u1 = __MOVAVG_AC(dst_timing, c1d1u1);
                }
            }

            if (training_update) {
                dst_timing->trained_dram_clktree_c0d0u0 = dst_timing->current_dram_clktree_c0d0u0;
                dst_timing->trained_dram_clktree_c0d0u1 = dst_timing->current_dram_clktree_c0d0u1;
                dst_timing->trained_dram_clktree_c0d1u0 = dst_timing->current_dram_clktree_c0d1u0;
                dst_timing->trained_dram_clktree_c0d1u1 = dst_timing->current_dram_clktree_c0d1u1;
                dst_timing->trained_dram_clktree_c1d0u0 = dst_timing->current_dram_clktree_c1d0u0;
                dst_timing->trained_dram_clktree_c1d0u1 = dst_timing->current_dram_clktree_c1d0u1;
                dst_timing->trained_dram_clktree_c1d1u0 = dst_timing->current_dram_clktree_c1d1u0;
                dst_timing->trained_dram_clktree_c1d1u1 = dst_timing->current_dram_clktree_c1d1u1;
            }

            return adel;

        }

        u32 PeriodicCompensationHandler(int type, u32 dram_dev_num, u32 fbio_cfg7, EmcDvfsTimingTable *src_timing, EmcDvfsTimingTable *dst_timing) {
            #define __COPY_EMA(nt, lt, dev)                     \
                ({ __MOVAVG(nt, dev) = __MOVAVG(lt, dev) *          \
                   (nt)->ptfv_dvfs_samples; })

            uint32_t adel = 0;
            uint32_t samples = dst_timing->ptfv_dvfs_samples;
            uint32_t samples_write = dst_timing->ptfv_write_samples;
            uint32_t delay = 2 + (1000 * ActualOscClocks(src_timing->run_clocks) / src_timing->rate_khz);

            if (!dst_timing->periodic_training)
                return 0;

            if (type == DVFS_SEQUENCE) {
                if (src_timing->periodic_training && (dst_timing->ptfv_config_ctrl & 1)) {
                    /*
                     * If the previous frequency was using periodic
                     * calibration then we can reuse the previous
                     * frequencies EMA data.
                     */
                    __COPY_EMA(dst_timing, src_timing, c0d0u0);
                    __COPY_EMA(dst_timing, src_timing, c0d0u1);
                    __COPY_EMA(dst_timing, src_timing, c1d0u0);
                    __COPY_EMA(dst_timing, src_timing, c1d0u1);
                    __COPY_EMA(dst_timing, src_timing, c0d1u0);
                    __COPY_EMA(dst_timing, src_timing, c0d1u1);
                    __COPY_EMA(dst_timing, src_timing, c1d1u0);
                    __COPY_EMA(dst_timing, src_timing, c1d1u1);
                } else {
                    /* Reset the EMA.*/
                    __MOVAVG(dst_timing, c0d0u0) = 0;
                    __MOVAVG(dst_timing, c0d0u1) = 0;
                    __MOVAVG(dst_timing, c1d0u0) = 0;
                    __MOVAVG(dst_timing, c1d0u1) = 0;
                    __MOVAVG(dst_timing, c0d1u0) = 0;
                    __MOVAVG(dst_timing, c0d1u1) = 0;
                    __MOVAVG(dst_timing, c1d1u0) = 0;
                    __MOVAVG(dst_timing, c1d1u1) = 0;

                    for (uint32_t i = 0; i < samples; i++) {
                        StartPeriodicCompensation();
                        util::WaitMicroSeconds(delay);

                        /* Generate next sample of data. */
                        adel = UpdateClockTreeDelay(src_timing, dst_timing, dram_dev_num, fbio_cfg7, DVFS_PT1);
                    }
                }

                adel = UpdateClockTreeDelay(src_timing, dst_timing, dram_dev_num, fbio_cfg7, DVFS_UPDATE);
            } else if (type == WRITE_TRAINING_SEQUENCE) {
                /* Reset the EMA.*/
                __MOVAVG(dst_timing, c0d0u0) = 0;
                __MOVAVG(dst_timing, c0d0u1) = 0;
                __MOVAVG(dst_timing, c1d0u0) = 0;
                __MOVAVG(dst_timing, c1d0u1) = 0;
                __MOVAVG(dst_timing, c0d1u0) = 0;
                __MOVAVG(dst_timing, c0d1u1) = 0;
                __MOVAVG(dst_timing, c1d1u0) = 0;
                __MOVAVG(dst_timing, c1d1u1) = 0;

                for (uint32_t i = 0; i < samples_write; i++) {
                    StartPeriodicCompensation();
                    util::WaitMicroSeconds(delay);

                    /* Generate next sample of data. */
                    UpdateClockTreeDelay(src_timing, dst_timing, dram_dev_num, fbio_cfg7, TRAINING_PT1);
                }

                adel = UpdateClockTreeDelay(src_timing, dst_timing, dram_dev_num, fbio_cfg7, TRAINING_UPDATE);
            } else if (type == PERIODIC_TRAINING_SEQUENCE) {
                StartPeriodicCompensation();
                util::WaitMicroSeconds(delay);

                adel = UpdateClockTreeDelay(src_timing, dst_timing, dram_dev_num, fbio_cfg7, PERIODIC_TRAINING_UPDATE);
            }

            return adel;
        }

        uint32_t ApplyPeriodicCompensationTrimmer(EmcDvfsTimingTable *dst_timing, uint32_t offset) {
            #define TRIM_REG(chan, rank, reg, byte)                 \
                ((EMC_PMACRO_OB_DDLL_LONG_DQ_RANK ## rank ## _ ## reg ##    \
                  _OB_DDLL_LONG_DQ_RANK ## rank ## _BYTE ## byte ## _MASK & \
                  dst_timing->trim_regs.emc_pmacro_ob_ddll_long_dq_rank ## rank ## _ ## reg ) >>    \
                 EMC_PMACRO_OB_DDLL_LONG_DQ_RANK ## rank ## _ ## reg ##     \
                 _OB_DDLL_LONG_DQ_RANK ## rank ## _BYTE ## byte ## _SHIFT)  \
                +                               \
                (((EMC_DATA_BRLSHFT_ ## rank ## _RANK ## rank ## _BYTE ##   \
                   byte ## _DATA_BRLSHFT_MASK &                 \
                   dst_timing->trim_perch_regs.emc ## chan ## _data_brlshft_ ## rank ) >> \
                  EMC_DATA_BRLSHFT_ ## rank ## _RANK ## rank ## _BYTE ##    \
                  byte ## _DATA_BRLSHFT_SHIFT) * 64)

            #define CALC_TEMP(rank, reg, byte1, byte2, n)               \
                ((adj[n] << EMC_PMACRO_OB_DDLL_LONG_DQ_RANK ## rank ## _ ## \
                  reg ## _OB_DDLL_LONG_DQ_RANK ## rank ## _BYTE ## byte1 ## _SHIFT) & \
                 EMC_PMACRO_OB_DDLL_LONG_DQ_RANK ## rank ## _ ## reg ##     \
                 _OB_DDLL_LONG_DQ_RANK ## rank ## _BYTE ## byte1 ## _MASK)  \
                |                               \
                ((adj[n + 1] << EMC_PMACRO_OB_DDLL_LONG_DQ_RANK ## rank ## _ ## \
                  reg ## _OB_DDLL_LONG_DQ_RANK ## rank ## _BYTE ## byte2 ## _SHIFT) & \
                 EMC_PMACRO_OB_DDLL_LONG_DQ_RANK ## rank ## _ ## reg ##     \
                 _OB_DDLL_LONG_DQ_RANK ## rank ## _BYTE ## byte2 ## _MASK)  \


            uint32_t temp = 0;
            uint32_t dst_rate_mhz = dst_timing->rate_khz / 1000;
            int tree_delta[4] = {0};
            u32 tree_delta_taps[4] = {0};
            int adj[] = {
                static_cast<int>(TRIM_REG(0, 0, 0, 0)),
                static_cast<int>(TRIM_REG(0, 0, 0, 1)),
                static_cast<int>(TRIM_REG(0, 0, 1, 2)),
                static_cast<int>(TRIM_REG(0, 0, 1, 3)),

                static_cast<int>(TRIM_REG(1, 0, 2, 4)),
                static_cast<int>(TRIM_REG(1, 0, 2, 5)),
                static_cast<int>(TRIM_REG(1, 0, 3, 6)),
                static_cast<int>(TRIM_REG(1, 0, 3, 7)),

                static_cast<int>(TRIM_REG(0, 1, 0, 0)),
                static_cast<int>(TRIM_REG(0, 1, 0, 1)),
                static_cast<int>(TRIM_REG(0, 1, 1, 2)),
                static_cast<int>(TRIM_REG(0, 1, 1, 3)),

                static_cast<int>(TRIM_REG(1, 1, 2, 4)),
                static_cast<int>(TRIM_REG(1, 1, 2, 5)),
                static_cast<int>(TRIM_REG(1, 1, 3, 6)),
                static_cast<int>(TRIM_REG(1, 1, 3, 7))
            };

            switch (offset) {
                case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_0:
                case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_1:
                case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_2:
                case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_3:
                case EMC_DATA_BRLSHFT_0:
                    tree_delta[0] = 128 * (dst_timing->current_dram_clktree_c0d0u0 - dst_timing->trained_dram_clktree_c0d0u0);
                    tree_delta[1] = 128 * (dst_timing->current_dram_clktree_c0d0u1 - dst_timing->trained_dram_clktree_c0d0u1);
                    tree_delta[2] = 128 * (dst_timing->current_dram_clktree_c1d0u0 - dst_timing->trained_dram_clktree_c1d0u0);
                    tree_delta[3] = 128 * (dst_timing->current_dram_clktree_c1d0u1 - dst_timing->trained_dram_clktree_c1d0u1);
                    tree_delta_taps[0] = (tree_delta[0] * static_cast<int>(dst_rate_mhz)) / 1000000;
                    tree_delta_taps[1] = (tree_delta[1] * static_cast<int>(dst_rate_mhz)) / 1000000;
                    tree_delta_taps[2] = (tree_delta[2] * static_cast<int>(dst_rate_mhz)) / 1000000;
                    tree_delta_taps[3] = (tree_delta[3] * static_cast<int>(dst_rate_mhz)) / 1000000;

                    for (int i = 0; i < 4; i++) {
                        if ((tree_delta_taps[i] > dst_timing->tree_margin) || (tree_delta_taps[i] < (-1 * dst_timing->tree_margin))) {
                            adj[i * 2] = adj[i * 2] + tree_delta_taps[i];
                            adj[i * 2 + 1] = adj[i * 2 + 1] + tree_delta_taps[i];
                        }
                    }

                    if (offset == EMC_DATA_BRLSHFT_0) {
                        for (int i = 0; i < 8; i++) {
                            adj[i] = adj[i] / 64;
                        }
                    } else {
                        for (int i = 0; i < 8; i++) {
                            adj[i] = adj[i] % 64;
                        }
                    }
                    break;
                case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_0:
                case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_1:
                case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_2:
                case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_3:
                case EMC_DATA_BRLSHFT_1:
                    tree_delta[0] = 128 * (dst_timing->current_dram_clktree_c0d1u0 - dst_timing->trained_dram_clktree_c0d1u0);
                    tree_delta[1] = 128 * (dst_timing->current_dram_clktree_c0d1u1 - dst_timing->trained_dram_clktree_c0d1u1);
                    tree_delta[2] = 128 * (dst_timing->current_dram_clktree_c1d1u0 - dst_timing->trained_dram_clktree_c1d1u0);
                    tree_delta[3] = 128 * (dst_timing->current_dram_clktree_c1d1u1 - dst_timing->trained_dram_clktree_c1d1u1);
                    tree_delta_taps[0] = (tree_delta[0] * static_cast<int>(dst_rate_mhz)) / 1000000;
                    tree_delta_taps[1] = (tree_delta[1] * static_cast<int>(dst_rate_mhz)) / 1000000;
                    tree_delta_taps[2] = (tree_delta[2] * static_cast<int>(dst_rate_mhz)) / 1000000;
                    tree_delta_taps[3] = (tree_delta[3] * static_cast<int>(dst_rate_mhz)) / 1000000;

                    for (int i = 0; i < 4; i++) {
                        if ((tree_delta_taps[i] > dst_timing->tree_margin) || (tree_delta_taps[i] < (-1 * dst_timing->tree_margin))) {
                            adj[8 + i * 2] = adj[8 + i * 2] + tree_delta_taps[i];
                            adj[8 + i * 2 + 1] = adj[8 + i * 2 + 1] + tree_delta_taps[i];
                        }
                    }

                    if (offset == EMC_DATA_BRLSHFT_1) {
                        for (int i = 0; i < 8; i++) {
                            adj[i + 8] = adj[i + 8] / 64;
                        }
                    } else {
                        for (int i = 0; i < 8; i++) {
                            adj[i + 8] = adj[i + 8] % 64;
                        }
                    }
                    break;
            }

            switch (offset) {
                case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_0:
                    temp = CALC_TEMP(0, 0, 0, 1, 0);
                    break;
                case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_1:
                    temp = CALC_TEMP(0, 1, 2, 3, 2);
                    break;
                case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_2:
                    temp = CALC_TEMP(0, 2, 4, 5, 4);
                    break;
                case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_3:
                    temp = CALC_TEMP(0, 3, 6, 7, 6);
                    break;
                case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_0:
                    temp = CALC_TEMP(1, 0, 0, 1, 8);
                    break;
                case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_1:
                    temp = CALC_TEMP(1, 1, 2, 3, 10);
                    break;
                case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_2:
                    temp = CALC_TEMP(1, 2, 4, 5, 12);
                    break;
                case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_3:
                    temp = CALC_TEMP(1, 3, 6, 7, 14);
                    break;
                case EMC_DATA_BRLSHFT_0:
                    temp = ((adj[0] <<
                         EMC_DATA_BRLSHFT_0_RANK0_BYTE0_DATA_BRLSHFT_SHIFT) &
                         EMC_DATA_BRLSHFT_0_RANK0_BYTE0_DATA_BRLSHFT_MASK) |
                           ((adj[1] <<
                         EMC_DATA_BRLSHFT_0_RANK0_BYTE1_DATA_BRLSHFT_SHIFT) &
                         EMC_DATA_BRLSHFT_0_RANK0_BYTE1_DATA_BRLSHFT_MASK) |
                           ((adj[2] <<
                         EMC_DATA_BRLSHFT_0_RANK0_BYTE2_DATA_BRLSHFT_SHIFT) &
                         EMC_DATA_BRLSHFT_0_RANK0_BYTE2_DATA_BRLSHFT_MASK) |
                           ((adj[3] <<
                         EMC_DATA_BRLSHFT_0_RANK0_BYTE3_DATA_BRLSHFT_SHIFT) &
                         EMC_DATA_BRLSHFT_0_RANK0_BYTE3_DATA_BRLSHFT_MASK) |
                           ((adj[4] <<
                         EMC_DATA_BRLSHFT_0_RANK0_BYTE4_DATA_BRLSHFT_SHIFT) &
                         EMC_DATA_BRLSHFT_0_RANK0_BYTE4_DATA_BRLSHFT_MASK) |
                           ((adj[5] <<
                         EMC_DATA_BRLSHFT_0_RANK0_BYTE5_DATA_BRLSHFT_SHIFT) &
                         EMC_DATA_BRLSHFT_0_RANK0_BYTE5_DATA_BRLSHFT_MASK) |
                           ((adj[6] <<
                         EMC_DATA_BRLSHFT_0_RANK0_BYTE6_DATA_BRLSHFT_SHIFT) &
                         EMC_DATA_BRLSHFT_0_RANK0_BYTE6_DATA_BRLSHFT_MASK) |
                           ((adj[7] <<
                         EMC_DATA_BRLSHFT_0_RANK0_BYTE7_DATA_BRLSHFT_SHIFT) &
                         EMC_DATA_BRLSHFT_0_RANK0_BYTE7_DATA_BRLSHFT_MASK);
                    break;
                case EMC_DATA_BRLSHFT_1:
                    temp = ((adj[8] <<
                         EMC_DATA_BRLSHFT_1_RANK1_BYTE0_DATA_BRLSHFT_SHIFT) &
                         EMC_DATA_BRLSHFT_1_RANK1_BYTE0_DATA_BRLSHFT_MASK) |
                           ((adj[9] <<
                         EMC_DATA_BRLSHFT_1_RANK1_BYTE1_DATA_BRLSHFT_SHIFT) &
                         EMC_DATA_BRLSHFT_1_RANK1_BYTE1_DATA_BRLSHFT_MASK) |
                           ((adj[10] <<
                         EMC_DATA_BRLSHFT_1_RANK1_BYTE2_DATA_BRLSHFT_SHIFT) &
                         EMC_DATA_BRLSHFT_1_RANK1_BYTE2_DATA_BRLSHFT_MASK) |
                           ((adj[11] <<
                         EMC_DATA_BRLSHFT_1_RANK1_BYTE3_DATA_BRLSHFT_SHIFT) &
                         EMC_DATA_BRLSHFT_1_RANK1_BYTE3_DATA_BRLSHFT_MASK) |
                           ((adj[12] <<
                         EMC_DATA_BRLSHFT_1_RANK1_BYTE4_DATA_BRLSHFT_SHIFT) &
                         EMC_DATA_BRLSHFT_1_RANK1_BYTE4_DATA_BRLSHFT_MASK) |
                           ((adj[13] <<
                         EMC_DATA_BRLSHFT_1_RANK1_BYTE5_DATA_BRLSHFT_SHIFT) &
                         EMC_DATA_BRLSHFT_1_RANK1_BYTE5_DATA_BRLSHFT_MASK) |
                           ((adj[14] <<
                         EMC_DATA_BRLSHFT_1_RANK1_BYTE6_DATA_BRLSHFT_SHIFT) &
                         EMC_DATA_BRLSHFT_1_RANK1_BYTE6_DATA_BRLSHFT_MASK) |
                           ((adj[15] <<
                         EMC_DATA_BRLSHFT_1_RANK1_BYTE7_DATA_BRLSHFT_SHIFT) &
                         EMC_DATA_BRLSHFT_1_RANK1_BYTE7_DATA_BRLSHFT_MASK);
                    break;
                default:
                    break;
            }

            #undef TRIM_REG
            #undef CALC_TEMP

            return temp;
        }

        uint32_t DvfsPowerRampDown(bool flip_backward, EmcDvfsTimingTable *src_timing, EmcDvfsTimingTable *dst_timing, uint32_t clk) {
            uint32_t ramp_down_wait = 0;
            uint32_t seq_wait = 0;
            uint32_t pmacro_cmd_pad = 0;
            uint32_t pmacro_dq_pad = 0;
            uint32_t pmacro_cfg5 = 0;
            uint32_t pmacro_rfu1 = 0;
            uint32_t pmacro_common_tx = 0;

            if (flip_backward) {
                pmacro_cmd_pad = dst_timing->burst_regs.emc_pmacro_cmd_pad_tx_ctrl;
                pmacro_dq_pad = dst_timing->burst_regs.emc_pmacro_data_pad_tx_ctrl;
                pmacro_rfu1 = dst_timing->burst_regs.emc_pmacro_brick_ctrl_rfu1;
                pmacro_cfg5 = dst_timing->burst_regs.emc_fbio_cfg5;
                pmacro_common_tx = dst_timing->burst_regs.emc_pmacro_common_pad_tx_ctrl;
            } else {
                pmacro_cmd_pad = src_timing->burst_regs.emc_pmacro_cmd_pad_tx_ctrl;
                pmacro_dq_pad = ((dst_timing->burst_regs.emc_pmacro_data_pad_tx_ctrl & 0x101) | src_timing->burst_regs.emc_pmacro_data_pad_tx_ctrl);
                pmacro_rfu1 = src_timing->burst_regs.emc_pmacro_brick_ctrl_rfu1;
                pmacro_cfg5 = src_timing->burst_regs.emc_fbio_cfg5;
                pmacro_common_tx = src_timing->burst_regs.emc_pmacro_common_pad_tx_ctrl;
            }

            pmacro_cmd_pad |= (1 << 26);

            CcfifoWrite(EMC_PMACRO_CMD_PAD_TX_CTRL, pmacro_cmd_pad, 0);
            CcfifoWrite(EMC_FBIO_CFG5, pmacro_cfg5 | (1 << 8), 12);

            ramp_down_wait = clk * 12;
            seq_wait = (100000 / clk) + 1;

            if (clk < (1000000 / 1000)) {
                if (clk < (1000000 / 2400)) {
                    pmacro_cmd_pad &= ~((1 << 1) | (1 << 24));
                    pmacro_cmd_pad |= (1 << 9) | (1 << 16);

                    CcfifoWrite(EMC_PMACRO_CMD_PAD_TX_CTRL, pmacro_cmd_pad, seq_wait);
                    ramp_down_wait += 100000;

                    pmacro_dq_pad &= ~((1 << 1) | (1 << 24));
                    pmacro_dq_pad |= (1 << 9) | (1 << 16);

                    CcfifoWrite(EMC_PMACRO_DATA_PAD_TX_CTRL, pmacro_dq_pad, 0);
                    CcfifoWrite(EMC_PMACRO_BRICK_CTRL_RFU1, pmacro_rfu1 & ~0x01120112, 0);
                } else {
                    CcfifoWrite(EMC_PMACRO_BRICK_CTRL_RFU1, pmacro_rfu1 & ~0x01120112, seq_wait);
                    ramp_down_wait += 100000;
                }

                CcfifoWrite(EMC_PMACRO_BRICK_CTRL_RFU1, pmacro_rfu1 & ~0x01bf01bf, seq_wait);
                ramp_down_wait += 100000;

                if (clk < (1000000 / 2400)) {
                    pmacro_cmd_pad &= ~((1 << 1) | (1 << 24) | (1 << 9) | (1 << 16));

                    CcfifoWrite(EMC_PMACRO_CMD_PAD_TX_CTRL, pmacro_cmd_pad, seq_wait);
                    ramp_down_wait += 100000;

                    pmacro_dq_pad &= ~((1 << 1) | (1 << 24) | (1 << 9) | (1 << 16));

                    CcfifoWrite(EMC_PMACRO_DATA_PAD_TX_CTRL, pmacro_dq_pad, 0);
                    CcfifoWrite(EMC_PMACRO_BRICK_CTRL_RFU1, pmacro_rfu1 & ~0x07ff07ff, 0);
                } else {
                    CcfifoWrite(EMC_PMACRO_BRICK_CTRL_RFU1, pmacro_rfu1 & ~0x07ff07ff, seq_wait);
                    ramp_down_wait += 100000;
                }
            } else {
                CcfifoWrite(EMC_PMACRO_BRICK_CTRL_RFU1, pmacro_rfu1 & ~0x07ff07ff, seq_wait + 19);
                ramp_down_wait += (100000 + (20 * clk));
            }

            if (clk < (1000000 / 600)) {
                ramp_down_wait += 100000;
                CcfifoWrite(EMC_PMACRO_COMMON_PAD_TX_CTRL, pmacro_common_tx & ~0x5, seq_wait);
                ramp_down_wait += 100000;
                CcfifoWrite(EMC_PMACRO_COMMON_PAD_TX_CTRL, pmacro_common_tx & ~0xf, seq_wait);
                ramp_down_wait += 100000;
                CcfifoWrite(0, 0, seq_wait);
                ramp_down_wait += 100000;
            } else {
                CcfifoWrite(EMC_PMACRO_COMMON_PAD_TX_CTRL, pmacro_common_tx & ~0xf, seq_wait);
            }

            return ramp_down_wait;
        }

        uint32_t DvfsPowerRampUp(bool flip_backward, EmcDvfsTimingTable *src_timing, EmcDvfsTimingTable *dst_timing, uint32_t training, uint32_t clk) {
            uint32_t ramp_up_wait = 0;
            uint32_t pmacro_cmd_pad = 0;
            uint32_t pmacro_dq_pad = 0;
            uint32_t pmacro_cfg5 = 0;
            uint32_t pmacro_rfu1 = 0;
            uint32_t pmacro_common_tx = 0;

            if (flip_backward) {
                pmacro_cmd_pad = src_timing->burst_regs.emc_pmacro_cmd_pad_tx_ctrl;
                pmacro_dq_pad = src_timing->burst_regs.emc_pmacro_data_pad_tx_ctrl;
                pmacro_rfu1 = src_timing->burst_regs.emc_pmacro_brick_ctrl_rfu1;
                pmacro_cfg5 = src_timing->burst_regs.emc_fbio_cfg5;
                pmacro_common_tx = src_timing->burst_regs.emc_pmacro_common_pad_tx_ctrl;
            } else if (training & 3) {
                pmacro_cmd_pad = dst_timing->shadow_regs_ca_train.emc_pmacro_cmd_pad_tx_ctrl;
                pmacro_dq_pad = dst_timing->shadow_regs_ca_train.emc_pmacro_data_pad_tx_ctrl;
                pmacro_rfu1 = dst_timing->shadow_regs_ca_train.emc_pmacro_brick_ctrl_rfu1;
                pmacro_cfg5 = dst_timing->shadow_regs_ca_train.emc_fbio_cfg5;
                pmacro_common_tx = dst_timing->shadow_regs_ca_train.emc_pmacro_common_pad_tx_ctrl;
            } else if (training & 0xC) {
                pmacro_cmd_pad = dst_timing->shadow_regs_quse_train.emc_pmacro_cmd_pad_tx_ctrl;
                pmacro_dq_pad = dst_timing->shadow_regs_quse_train.emc_pmacro_data_pad_tx_ctrl;
                pmacro_rfu1 = dst_timing->shadow_regs_quse_train.emc_pmacro_brick_ctrl_rfu1;
                pmacro_cfg5 = dst_timing->shadow_regs_quse_train.emc_fbio_cfg5;
                pmacro_common_tx = dst_timing->shadow_regs_quse_train.emc_pmacro_common_pad_tx_ctrl;
            } else if (training & 0xF0) {
                pmacro_cmd_pad = dst_timing->shadow_regs_rdwr_train.emc_pmacro_cmd_pad_tx_ctrl;
                pmacro_dq_pad = dst_timing->shadow_regs_rdwr_train.emc_pmacro_data_pad_tx_ctrl;
                pmacro_rfu1 = dst_timing->shadow_regs_rdwr_train.emc_pmacro_brick_ctrl_rfu1;
                pmacro_cfg5 = dst_timing->shadow_regs_rdwr_train.emc_fbio_cfg5;
                pmacro_common_tx = dst_timing->shadow_regs_rdwr_train.emc_pmacro_common_pad_tx_ctrl;
            } else {
                pmacro_cmd_pad = dst_timing->burst_regs.emc_pmacro_cmd_pad_tx_ctrl;
                pmacro_dq_pad = dst_timing->burst_regs.emc_pmacro_data_pad_tx_ctrl;
                pmacro_rfu1 = dst_timing->burst_regs.emc_pmacro_brick_ctrl_rfu1;
                pmacro_cfg5 = dst_timing->burst_regs.emc_fbio_cfg5;
                pmacro_common_tx = dst_timing->burst_regs.emc_pmacro_common_pad_tx_ctrl;
            }

            pmacro_cmd_pad |= (1 << 26);

            if (clk < 1000000 / 600) {
                CcfifoWrite(EMC_PMACRO_COMMON_PAD_TX_CTRL, pmacro_common_tx & 0xa, 0);
                CcfifoWrite(EMC_PMACRO_COMMON_PAD_TX_CTRL, pmacro_common_tx & 0xf, (100000 / clk) + 1);
                ramp_up_wait += 100000;
            } else {
                CcfifoWrite(EMC_PMACRO_COMMON_PAD_TX_CTRL, pmacro_common_tx | 0x8, 0);
            }

            if (clk < 1000000 / 1000) {
                if (clk < 1000000 / 2400) {
                    pmacro_cmd_pad |= ((1 << 9) | (1 << 16));
                    pmacro_cmd_pad &= ~((1 << 1) | (1 << 24));

                    CcfifoWrite(EMC_PMACRO_CMD_PAD_TX_CTRL, pmacro_cmd_pad, (100000 / clk) + 1);
                    ramp_up_wait += 100000;

                    pmacro_dq_pad |= ((1 << 9) | (1 << 16));
                    pmacro_dq_pad &= ~((1 << 1) | (1 << 24));

                    CcfifoWrite(EMC_PMACRO_DATA_PAD_TX_CTRL, pmacro_dq_pad, 0);
                    CcfifoWrite(EMC_PMACRO_BRICK_CTRL_RFU1, pmacro_rfu1 & 0xfe40fe40, 0);
                } else {
                    CcfifoWrite(EMC_PMACRO_BRICK_CTRL_RFU1, pmacro_rfu1 & 0xfe40fe40, (100000 / clk) + 1);
                    ramp_up_wait += 100000;
                }

                CcfifoWrite(EMC_PMACRO_BRICK_CTRL_RFU1, pmacro_rfu1 & 0xfeedfeed, (100000 / clk) + 1);
                ramp_up_wait += 100000;

                if (clk < 1000000 / 2400) {
                    pmacro_cmd_pad |= ((1 << 9) | (1 << 16) | (1 << 1) | (1 << 24));

                    CcfifoWrite(EMC_PMACRO_CMD_PAD_TX_CTRL, pmacro_cmd_pad, (100000 / clk) + 1);
                    ramp_up_wait += 100000;

                    pmacro_dq_pad |= ((1 << 9) | (1 << 16) | (1 << 1) | (1 << 24));

                    CcfifoWrite(EMC_PMACRO_DATA_PAD_TX_CTRL, pmacro_dq_pad, 0);
                    CcfifoWrite(EMC_PMACRO_BRICK_CTRL_RFU1, pmacro_rfu1, 0);
                } else {
                    CcfifoWrite(EMC_PMACRO_BRICK_CTRL_RFU1, pmacro_rfu1, (100000 / clk) + 1);
                    ramp_up_wait += 100000;
                }

                CcfifoWrite(EMC_FBIO_CFG5, pmacro_cfg5 & ~(1 << 8), (100000 / clk) + 10);
                ramp_up_wait += (100000 + (10 * clk));
            } else if (clk < 1000000 / 600) {
                CcfifoWrite(EMC_PMACRO_BRICK_CTRL_RFU1, pmacro_rfu1 | 0x06000600, (100000 / clk) + 1);
                CcfifoWrite(EMC_FBIO_CFG5, pmacro_cfg5 & ~(1 << 8), (100000 / clk) + 10);
                ramp_up_wait += (100000 + 10 * clk);
            } else {
                CcfifoWrite(EMC_PMACRO_BRICK_CTRL_RFU1, pmacro_rfu1 | 0x00000600, 0);
                CcfifoWrite(EMC_FBIO_CFG5, pmacro_cfg5 & ~(1 << 8), 12);
                ramp_up_wait += (12 * clk);
            }

            pmacro_cmd_pad &= ~(1 << 26);
            CcfifoWrite(EMC_PMACRO_CMD_PAD_TX_CTRL, pmacro_cmd_pad, 5);

            return ramp_up_wait;
        }

        void FreqChange(EmcDvfsTimingTable *src_timing, EmcDvfsTimingTable *dst_timing, u32 training, u32 next_clk_src) {
            /* Extract training values. */
            const bool train_ca           = (training & CA_TRAINING);
            const bool train_ca_vref      = (training & CA_VREF_TRAINING);
            const bool train_quse         = (training & QUSE_TRAINING);
            const bool train_quse_vref    = (training & QUSE_VREF_TRAINING);
            const bool train_wr           = (training & WRITE_TRAINING);
            const bool train_wr_vref      = (training & WRITE_VREF_TRAINING);
            const bool train_rd           = (training & READ_TRAINING);
            const bool train_rd_vref      = (training & READ_VREF_TRAINING);
            const bool train_second_rank  = (training & TRAIN_SECOND_RANK);
            const bool train_bit_level    = (training & BIT_LEVEL_TRAINING);

            /* Check if we should do training. */
            const bool training_enabled = (training & (CA_TRAINING | CA_VREF_TRAINING | QUSE_TRAINING | WRITE_TRAINING | WRITE_VREF_TRAINING | READ_TRAINING | READ_VREF_TRAINING));

            /* Declare variables. */
            bool skip_zqcal = false;
            bool compensate_trimmer_applicable = false;
            uint32_t zqcal_before_cc_cutoff = 2400; /* In picoseconds */
            int zq_latch_dvfs_wait_time;

            uint32_t mr13_catr_enable;
            uint32_t mr13_flip_fspwr;
            uint32_t mr13_flip_fspop;

            int next_push, next_dq_e_ivref, next_dqs_e_ivref;

            uint32_t zq_wait_long;
            uint32_t zq_wait_short;

            uint32_t tRTM;
            uint32_t RP_war;
            uint32_t R2P_war;
            uint32_t TRPab_war;
            int nRTP;
            uint32_t deltaTWATM;
            uint32_t W2P_war;
            uint32_t tRPST;

            uint32_t mrw_req;
            uint32_t adel = 0;
            uint32_t dst_rate_mhz = dst_timing->rate_khz / 1000;

            /* Set some common values needed. */
            const int dram_type    = reg::GetValue(EMC + EMC_FBIO_CFG5, EMC_REG_BITS_MASK(FBIO_CFG5_DRAM_TYPE));
            const int dram_dev_num = (reg::Read(MC + MC_EMEM_ADR_CFG) & 1) + 1;
            const bool shared_zq_resistor = ((src_timing->burst_regs.emc_zcal_wait_cnt >> 31) & 1);
            const u32 fbio_cfg7 = src_timing->burst_regs.emc_fbio_cfg7;
            const bool is_lpddr3 = (dram_type == DRAM_TYPE_LPDDR2) && ((dst_timing->burst_regs.emc_fbio_cfg5 >> 25) & 1);
            bool opt_zcal_en_cc = ((dst_timing->burst_regs.emc_zcal_interval && !src_timing->burst_regs.emc_zcal_interval) || (dram_type == DRAM_TYPE_LPDDR4));
            bool opt_war_200024907 = (dram_type == DRAM_TYPE_LPDDR4);
            bool opt_do_sw_qrst = false;
            bool opt_cc_short_zcal = true;
            bool opt_short_zcal = true;
            bool save_restore_clkstop_pd = true;
            uint32_t opt_dll_mode = (dram_type == DRAM_TYPE_DDR4) ? GetDllState(dst_timing) : DLL_OFF;
            uint32_t opt_dvfs_mode = MAN_SR;
            uint32_t emc_auto_cal_config = reg::Read(EMC + EMC_AUTO_CAL_CONFIG);

            /* In picoseconds. */
            uint32_t source_clock_period = 1000000000 / src_timing->rate_khz;
            uint32_t destination_clock_period = 1000000000 / dst_timing->rate_khz;

            uint32_t tFC_lpddr4 = 1000 * dst_timing->dram_timings.t_fc_lpddr4;
            uint32_t tZQCAL_lpddr4 = 1000000;
            int tZQCAL_lpddr4_fc_adj = (source_clock_period > zqcal_before_cc_cutoff) ? tZQCAL_lpddr4 / destination_clock_period : (tZQCAL_lpddr4 - tFC_lpddr4) / destination_clock_period;

            g_fsp_for_next_freq = !g_fsp_for_next_freq;

            uint32_t emc_dbg_o = reg::Read(EMC + EMC_DBG);
            uint32_t emc_pin_o = reg::Read(EMC + EMC_PIN);
            uint32_t emc_cfg_pipe_clk_o = reg::Read(EMC + EMC_CFG_PIPE_CLK);
            uint32_t emc_dbg = emc_dbg_o;

            uint32_t emc_cfg = dst_timing->burst_regs.emc_cfg & 0x0FFFFFFF;
            uint32_t emc_sel_dpd_ctrl = dst_timing->emc_sel_dpd_ctrl & 0xFFFFFEC3;

            /* Step 1.1: Disable DLL. */
            DllDisable(fbio_cfg7);

            /* Step 1.2: Disable AUTOCAL. */
            emc_auto_cal_config = dst_timing->emc_auto_cal_config;
            u32 auto_cal_en = (emc_auto_cal_config & (1 << 29));
            emc_auto_cal_config &= 0x7FFFF9FF;
            emc_auto_cal_config |= 0x600;
            reg::Write(EMC + EMC_AUTO_CAL_CONFIG, emc_auto_cal_config);

            /* Step 1.3: Disable other power features. */
            SetShadowBypass(ACTIVE);
            reg::Write(EMC + EMC_CFG, emc_cfg);
            reg::Write(EMC + EMC_SEL_DPD_CTRL, emc_sel_dpd_ctrl);
            SetShadowBypass(ASSEMBLY);

            /* Skip this if training_enabled is set. */
            if (!training_enabled && dst_timing->periodic_training) {
                /* Wait for DRAM to get out of power down. */
                if (dram_dev_num == TWO_RANK) {
                    WaitForUpdate(EMC_EMC_STATUS, 0x30, false, fbio_cfg7);
                } else {
                    WaitForUpdate(EMC_EMC_STATUS, 0x10, false, fbio_cfg7);
                }

                /* Wait for DRAM to get out of self refresh. */
                WaitForUpdate(EMC_EMC_STATUS, 0x300, false, fbio_cfg7);

                /* Reset all clock tree values. */
                dst_timing->current_dram_clktree_c0d0u0 = dst_timing->trained_dram_clktree_c0d0u0;
                dst_timing->current_dram_clktree_c0d0u1 = dst_timing->trained_dram_clktree_c0d0u1;
                dst_timing->current_dram_clktree_c0d1u0 = dst_timing->trained_dram_clktree_c0d1u0;
                dst_timing->current_dram_clktree_c0d1u1 = dst_timing->trained_dram_clktree_c0d1u1;
                dst_timing->current_dram_clktree_c1d0u0 = dst_timing->trained_dram_clktree_c1d0u0;
                dst_timing->current_dram_clktree_c1d0u1 = dst_timing->trained_dram_clktree_c1d0u1;
                dst_timing->current_dram_clktree_c1d1u0 = dst_timing->trained_dram_clktree_c1d1u0;
                dst_timing->current_dram_clktree_c1d1u1 = dst_timing->trained_dram_clktree_c1d1u1;

                /* Do DVFS_SEQUENCE. */
                adel = PeriodicCompensationHandler(DVFS_SEQUENCE, dram_dev_num, fbio_cfg7, src_timing, dst_timing);

                /* Check if we should use compensate trimmer. */
                compensate_trimmer_applicable = dst_timing->periodic_training && ((adel * 128 * dst_rate_mhz) / 1000000) > dst_timing->tree_margin;
            }

            reg::Write(EMC + EMC_INTSTATUS, 0x10);
            SetShadowBypass(ACTIVE);
            reg::Write(EMC + EMC_CFG, emc_cfg);
            reg::Write(EMC + EMC_SEL_DPD_CTRL, emc_sel_dpd_ctrl);
            reg::Write(EMC + EMC_CFG_PIPE_CLK, emc_cfg_pipe_clk_o | 0x1);
            reg::Write(EMC + EMC_FDPD_CTRL_CMD_NO_RAMP, dst_timing->emc_fdpd_ctrl_cmd_no_ramp & ~0x1);

            uint32_t bg_regulator_mode_change = ((dst_timing->burst_regs.emc_pmacro_bg_bias_ctrl_0 & (1 << 2)) ^ (src_timing->burst_regs.emc_pmacro_bg_bias_ctrl_0 & (1 << 2))) || ((dst_timing->burst_regs.emc_pmacro_bg_bias_ctrl_0 & (1 << 0)) ^ (src_timing->burst_regs.emc_pmacro_bg_bias_ctrl_0 & (1 << 0)));

            uint32_t enable_bg_regulator = (dst_timing->burst_regs.emc_pmacro_bg_bias_ctrl_0 & (1 << 0)) == 0;

            /* Check if we need to change BG the regulator. */
            if (bg_regulator_mode_change) {
                if (enable_bg_regulator) {
                    reg::Write(EMC + EMC_PMACRO_BG_BIAS_CTRL_0, src_timing->burst_regs.emc_pmacro_bg_bias_ctrl_0 & ~(1 << 0));
                } else {
                    reg::Write(EMC + EMC_PMACRO_BG_BIAS_CTRL_0, src_timing->burst_regs.emc_pmacro_bg_bias_ctrl_0 & ~(1 << 2));
                }
            }

            /* Check if we need to turn on VREF generator. */
            if ((((!(src_timing->burst_regs.emc_pmacro_data_pad_tx_ctrl & (1 << 0)))) &&
                 ((dst_timing->burst_regs.emc_pmacro_data_pad_tx_ctrl & (1 << 0)))) ||
                ((!(src_timing->burst_regs.emc_pmacro_data_pad_tx_ctrl & (1 << 8))) &&
                 ((dst_timing->burst_regs.emc_pmacro_data_pad_tx_ctrl & (1 << 8)))))
            {
                uint32_t pad_tx_ctrl = dst_timing->burst_regs.emc_pmacro_data_pad_tx_ctrl;
                uint32_t last_pad_tx_ctrl = src_timing->burst_regs.emc_pmacro_data_pad_tx_ctrl;

                next_dqs_e_ivref = pad_tx_ctrl & (1 << 8);
                next_dq_e_ivref = pad_tx_ctrl & (1 << 0);
                next_push = (last_pad_tx_ctrl & ~(1 << 0) & ~(1 << 8)) | next_dq_e_ivref | next_dqs_e_ivref;
                reg::Write(EMC + EMC_PMACRO_DATA_PAD_TX_CTRL, next_push);
                util::WaitMicroSeconds(1);
            } else if (bg_regulator_mode_change) {
                util::WaitMicroSeconds(1);
            }

            SetShadowBypass(ASSEMBLY);

            /* Step 2:
             *   Prelock the DLL.
             */
            if (dst_timing->burst_regs.emc_cfg_dig_dll & 0x1) {
                DllPrelock(dst_timing, training_enabled, next_clk_src);
            } else {
                ChangeDllSrc(dst_timing, next_clk_src);
                DllDisable(fbio_cfg7);
            }

            /* Step 3:
             *   Prepare autocal for the clock change.
             */
            SetShadowBypass(ACTIVE);
            reg::Write(EMC + EMC_AUTO_CAL_CONFIG2, dst_timing->emc_auto_cal_config2);
            reg::Write(EMC + EMC_AUTO_CAL_CONFIG3, dst_timing->emc_auto_cal_config3);
            reg::Write(EMC + EMC_AUTO_CAL_CONFIG4, dst_timing->emc_auto_cal_config4);
            reg::Write(EMC + EMC_AUTO_CAL_CONFIG5, dst_timing->emc_auto_cal_config5);
            reg::Write(EMC + EMC_AUTO_CAL_CONFIG6, dst_timing->emc_auto_cal_config6);
            reg::Write(EMC + EMC_AUTO_CAL_CONFIG7, dst_timing->emc_auto_cal_config7);
            reg::Write(EMC + EMC_AUTO_CAL_CONFIG8, dst_timing->emc_auto_cal_config8);
            SetShadowBypass(ASSEMBLY);

            emc_auto_cal_config |= (0x1 | auto_cal_en);
            reg::Write(EMC + EMC_AUTO_CAL_CONFIG, emc_auto_cal_config);

            /* Step 4:
             *   Update EMC_CFG.
             */
            if ((source_clock_period > 50000) && (dram_type == DRAM_TYPE_LPDDR4)) {
                CcfifoWrite(EMC_SELF_REF, 1, 0);
            } else {
                reg::Write(EMC + EMC_CFG_2, dst_timing->emc_cfg_2);
            }

            /* Step 5:
             *   Prepare reference variables for ZQCAL regs.
             */
            uint32_t emc_zcal_interval = src_timing->burst_regs.emc_zcal_interval;
            emc_zcal_interval &= 0xFF000000;
            uint32_t emc_zcal_wait_cnt_old = src_timing->burst_regs.emc_zcal_wait_cnt;
            uint32_t emc_zcal_wait_cnt_new = dst_timing->burst_regs.emc_zcal_wait_cnt;
            emc_zcal_wait_cnt_old &= ~0x7ff;
            emc_zcal_wait_cnt_new &= ~0x7ff;

            if (dram_type == DRAM_TYPE_LPDDR4) {
                zq_wait_long = std::max<u32>(1, util::DivideUp(1000000, destination_clock_period));
            } else if (dram_type == DRAM_TYPE_LPDDR2 || is_lpddr3) {
                zq_wait_long = std::max<u32>(dst_timing->min_mrs_wait, util::DivideUp(360000, destination_clock_period)) + 4;
            } else if (dram_type == DRAM_TYPE_DDR4) {
                zq_wait_long = std::max<u32>(256, util::DivideUp(320000, destination_clock_period) + 2);
            } else {
                zq_wait_long = 0;
            }


            if (dram_type == DRAM_TYPE_LPDDR2 || is_lpddr3) {
                zq_wait_short = std::max<u32>(std::max<u32>(dst_timing->min_mrs_wait, 6), util::DivideUp(90000, destination_clock_period)) + 4;
            } else if (dram_type == DRAM_TYPE_DDR4) {
                zq_wait_short = std::max<u32>(64, util::DivideUp(80000, destination_clock_period)) + 2;
            } else {
                zq_wait_short = 0;
            }

            /* TODO: Actually use the reference variables. */
            AMS_UNUSED(zq_wait_long, zq_wait_short);

            /* Step 6:
             *   Training code.
             */
            if ((train_ca || train_ca_vref) && (dram_dev_num == TWO_RANK)) {
                reg::Write(EMC + EMC_PIN, 0x107);
            }

            /* Step 7:
             *   Program FSP reference registers and send MRWs to new FSPWR.
             */

            /* Step 7.1: Bug 200024907 - Patch RP R2P */
            if (opt_war_200024907) {
                nRTP = 16;
                if (source_clock_period >= 1000000/1866) /* 535.91 ps */
                    nRTP = 14;
                if (source_clock_period >= 1000000/1600) /* 625.00 ps */
                    nRTP = 12;
                if (source_clock_period >= 1000000/1333) /* 750.19 ps */
                    nRTP = 10;
                if (source_clock_period >= 1000000/1066) /* 938.09 ps */
                    nRTP = 8;

                deltaTWATM = std::max<u32>(util::DivideUp(7500, source_clock_period), 8);

                /*
                 * Originally there was a + .5 in the tRPST calculation.
                 * However since we can't do FP in the kernel and the tRTM
                 * computation was in a floating point ceiling function, adding
                 * one to tRTP should be ok. There is no other source of non
                 * integer values, so the result was always going to be
                 * something for the form: f_ceil(N + .5) = N + 1;
                 */
                tRPST = ((src_timing->emc_mrw & 0x80) >> 7);
                tRTM = src_timing->dram_timings.rl + util::DivideUp(3600, source_clock_period) + std::max<u32>(util::DivideUp(7500, source_clock_period), 8) + tRPST + 1 + nRTP;

                if (src_timing->burst_regs.emc_rp < tRTM) {
                    if (tRTM > (src_timing->burst_regs.emc_r2p + src_timing->burst_regs.emc_rp)) {
                        R2P_war = tRTM - src_timing->burst_regs.emc_rp;
                        RP_war = src_timing->burst_regs.emc_rp;
                        TRPab_war = src_timing->burst_regs.emc_trpab;
                        if (R2P_war > 63) {
                            RP_war = R2P_war + src_timing->burst_regs.emc_rp - 63;
                            if (TRPab_war < RP_war)
                                TRPab_war = RP_war;
                            R2P_war = 63;
                        }
                    } else {
                        R2P_war = src_timing-> burst_regs.emc_r2p;
                        RP_war = src_timing->burst_regs.emc_rp;
                        TRPab_war = src_timing->burst_regs.emc_trpab;
                    }

                    if (RP_war < deltaTWATM) {
                        W2P_war = src_timing->burst_regs.emc_w2p + deltaTWATM - RP_war;
                        if (W2P_war > 63) {
                            RP_war = RP_war + W2P_war - 63;
                            if (TRPab_war < RP_war)
                                TRPab_war = RP_war;
                            W2P_war = 63;
                        }
                    } else {
                        W2P_war = src_timing->burst_regs.emc_w2p;
                    }

                    if ((src_timing->burst_regs.emc_w2p != W2P_war)
                        || (src_timing->burst_regs.emc_r2p != R2P_war)
                        || (src_timing->burst_regs.emc_rp != RP_war)
                        || (src_timing->burst_regs.emc_trpab != TRPab_war))
                    {
                        SetShadowBypass(ACTIVE);
                        reg::Write(EMC + EMC_RP, RP_war);
                        reg::Write(EMC + EMC_R2P, R2P_war);
                        reg::Write(EMC + EMC_W2P, W2P_war);
                        reg::Write(EMC + EMC_TRPAB, TRPab_war);
                        SetShadowBypass(ASSEMBLY);
                        util::WaitMicroSeconds(1);
                    }
                }
            }

            if (!g_fsp_for_next_freq) {
                mr13_flip_fspwr = (dst_timing->emc_mrw3 & 0xffffff3f) | 0x80;
                mr13_flip_fspop = (dst_timing->emc_mrw3 & 0xffffff3f) | 0x00;
            } else {
                mr13_flip_fspwr = (dst_timing->emc_mrw3 & 0xffffff3f) | 0x40;
                mr13_flip_fspop = (dst_timing->emc_mrw3 & 0xffffff3f) | 0xc0;
            }

            mr13_catr_enable = (mr13_flip_fspwr & 0xFFFFFFFE) | 0x01;

            if (dram_dev_num == TWO_RANK) {
                if (train_ca || train_ca_vref) {
                    if (train_second_rank) {
                        mr13_flip_fspop = (mr13_flip_fspop & 0x3FFFFFFF) | 0x80000000;
                        mr13_catr_enable = (mr13_catr_enable & 0x3FFFFFFF)| 0x40000000;
                    } else {
                        mr13_flip_fspop = (mr13_flip_fspop & 0x3FFFFFFF) | 0x40000000;
                        mr13_catr_enable = (mr13_catr_enable & 0x3FFFFFFF) | 0x80000000;
                    }
                } else {
                    if (train_second_rank) {
                        mr13_catr_enable = (mr13_catr_enable & 0x3FFFFFFF) | 0x40000000;
                    } else {
                        mr13_catr_enable = (mr13_catr_enable & 0x3FFFFFFF) | 0x80000000;
                    }
                }
            }

            if (dram_type == DRAM_TYPE_LPDDR4) {
                reg::Write(EMC + EMC_MRW3, mr13_flip_fspwr);
                reg::Write(EMC + EMC_MRW, dst_timing->emc_mrw);
                reg::Write(EMC + EMC_MRW2, dst_timing->emc_mrw2);
            }

            /* Step 8:
             *   Program the shadow registers.
             */

            /* Set burst registers. */
            for (u32 i = 0; i < dst_timing->num_burst; i++) {
                uint32_t var = 0;
                uint32_t wval = 0;

                if (!BurstRegistersOffsets[i]) {
                    continue;
                }

                var = BurstRegistersOffsets[i];

                if (train_ca || train_ca_vref) {
                    wval = dst_timing->shadow_regs_ca_train_arr[i];
                } else if (train_quse || train_quse_vref) {
                    wval = dst_timing->shadow_regs_quse_train_arr[i];
                } else if (train_wr || train_wr_vref || train_rd || train_rd_vref) {
                    wval = dst_timing->shadow_regs_rdwr_train_arr[i];
                } else {
                    wval = dst_timing->burst_regs_arr[i];
                }


                if (dram_type != DRAM_TYPE_LPDDR4 &&
                    (var == EMC_MRW6      || var == EMC_MRW7 ||
                     var == EMC_MRW8      || var == EMC_MRW9 ||
                     var == EMC_MRW10     || var == EMC_MRW11 ||
                     var == EMC_MRW12     || var == EMC_MRW13 ||
                     var == EMC_MRW14     || var == EMC_MRW15 ||
                     var == EMC_TRAINING_CTRL))
                {
                    continue;
                }

                if (var == EMC_CFG) {
                    wval &= (dram_type == DRAM_TYPE_LPDDR4) ? 0x0FFFFFFF : 0xCFFFFFFF;
                } else if ((var == EMC_ZCAL_INTERVAL) && opt_zcal_en_cc) {
                    wval = 0; /* EMC_ZCAL_INTERVAL reset value. */
                } else if (var == EMC_PMACRO_AUTOCAL_CFG_COMMON) {
                    wval |= (1 << 16);
                } else if (var == EMC_PMACRO_DATA_PAD_TX_CTRL) {
                    wval &= 0xFEFEFDFD;
                } else if (var == EMC_PMACRO_CMD_PAD_TX_CTRL) {
                    wval &= 0xFAFEFDFD;
                    wval |= 0x04000000;
                } else if (var == EMC_PMACRO_BRICK_CTRL_RFU1) {
                    wval &= 0xf800f800;
                } else if (var == EMC_PMACRO_COMMON_PAD_TX_CTRL) {
                    wval &= 0xfffffff0;
                } else if (var == EMC_TRAINING_CTRL) {
                    wval |= train_second_rank ? (1 << 14) : 0;
                }

                reg::Write(EMC + var, wval);
            }

            if (dram_type == DRAM_TYPE_LPDDR4) {
                /* Use the current timing when training. */
                if (training_enabled) {
                    mrw_req = (23 << 16) | (src_timing->run_clocks & 0xFF);
                } else {
                    mrw_req = (23 << 16) | (dst_timing->run_clocks & 0xFF);
                }

                reg::Write(EMC + EMC_MRW, mrw_req);
            }

            /* Per channel burst registers. */
            const bool dual_channel = reg::GetField(fbio_cfg7, EMC_REG_BITS_MASK(FBIO_CFG7_CH1_ENABLE)) == EMC_FBIO_CFG7_CH1_ENABLE_ENABLE;
            for (u32 i = 0; i < dst_timing->num_burst_per_ch; i++) {
                if (!PerChannelBurstRegisters[i]) {
                    continue;
                }

                const u32 addr = PerChannelBurstRegisters[i];
                const u32 base = addr & ~0xFFF;
                const u32 off  = addr &  0xFFF;

                if (dram_type != DRAM_TYPE_LPDDR4 &&
                    (off == EMC_MRW6  ||
                     off == EMC_MRW7  ||
                     off == EMC_MRW8  ||
                     off == EMC_MRW9  ||
                     off == EMC_MRW10 ||
                     off == EMC_MRW11 ||
                     off == EMC_MRW12 ||
                     off == EMC_MRW13 ||
                     off == EMC_MRW14 ||
                     off == EMC_MRW15)
                    )
                {
                    continue;
                }

                /* Filter out second channel if not in DUAL_CHANNEL mode. */
                if (!dual_channel && base == EMC1) {
                    continue;
                }

                /* Write the value. */
                reg::Write(addr, dst_timing->burst_perch_regs_arr[i]);
            }

            /* Vref regs. */
            for (u32 i = 0; i < dst_timing->vref_num; i++) {
                if (!PerChannelVrefRegisters[i]) {
                    continue;
                }

                const u32 addr = PerChannelVrefRegisters[i];
                const u32 base = addr & ~0xFFF;

                /* Filter out second channel if not in DUAL_CHANNEL mode. */
                if (!dual_channel && base == EMC1) {
                    continue;
                }

                /* Write the value. */
                reg::Write(addr, dst_timing->vref_perch_regs_arr[i]);
            }

            /* Training regs. */
            if (training_enabled) {
                for (u32 i = 0; i < dst_timing->training_mod_num; i++) {
                    if (!PerChannelTrainingModRegisters[i]) {
                        continue;
                    }

                    const u32 addr = PerChannelTrainingModRegisters[i];
                    const u32 base = addr & ~0xFFF;

                    /* Filter out second channel if not in DUAL_CHANNEL mode. */
                    if (!dual_channel && base == EMC1) {
                        continue;
                    }

                    /* Write the value. */
                    reg::Write(addr, dst_timing->training_mod_regs_arr[i]);
                }
            }

            /* Trimmers. */
            for (u32 i = 0; i < dst_timing->num_trim; i++) {
                if (!TrimRegisters[i]) {
                    continue;
                }

                const u32 addr = TrimRegisters[i];
                const u32 ofs  = addr &  0xFFF;

                u32 wval = dst_timing->trim_regs_arr[i];

                if (compensate_trimmer_applicable &&
                    (ofs == EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_0 ||
                     ofs == EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_1 ||
                     ofs == EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_2 ||
                     ofs == EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_3 ||
                     ofs == EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_0 ||
                     ofs == EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_1 ||
                     ofs == EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_2 ||
                     ofs == EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_3 ||
                     ofs == EMC_DATA_BRLSHFT_0 ||
                     ofs == EMC_DATA_BRLSHFT_1))
                {
                    wval = ApplyPeriodicCompensationTrimmer(dst_timing, ofs);
                }

                /* Write the value. */
                reg::Write(addr, wval);
            }

            /* Per-channel trimmers. */
            for (u32 i = 0; i < dst_timing->num_trim_per_ch; i++) {
                if (!PerChannelTrimRegisters[i]) {
                    continue;
                }

                const u32 addr = PerChannelTrimRegisters[i];
                const u32 base = addr & ~0xFFF;
                const u32 ofs  = addr &  0xFFF;

                /* Filter out second channel if not in DUAL_CHANNEL mode. */
                if (!dual_channel && base == EMC1) {
                    continue;
                }

                u32 wval = dst_timing->trim_perch_regs_arr[i];

                if (compensate_trimmer_applicable &&
                    (ofs == EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_0 ||
                     ofs == EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_1 ||
                     ofs == EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_2 ||
                     ofs == EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_3 ||
                     ofs == EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_0 ||
                     ofs == EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_1 ||
                     ofs == EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_2 ||
                     ofs == EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_3 ||
                     ofs == EMC_DATA_BRLSHFT_0 ||
                     ofs == EMC_DATA_BRLSHFT_1))
                {
                    wval = ApplyPeriodicCompensationTrimmer(dst_timing, ofs);
                }

                /* Write the value. */
                reg::Write(addr, wval);
            }

            if (training_enabled) {
                if (train_wr && dst_timing->periodic_training && (dram_type == DRAM_TYPE_LPDDR4)) {
                    PeriodicCompensationHandler(WRITE_TRAINING_SEQUENCE, dram_dev_num, fbio_cfg7, src_timing, dst_timing);
                }
            } else {
                /* Write burst_mc_regs. */
                for (u32 i = 0; i < dst_timing->num_mc_regs; i++) {
                    reg::Write(BurstMcRegisters[i], dst_timing->burst_mc_regs_arr[i]);
                }
            }

            /* Registers to be programmed on the faster clock. */
            if (!training_enabled && (dst_timing->rate_khz < src_timing->rate_khz)) {
                for (u32 i = 0; i < dst_timing->num_up_down; i++) {
                    reg::Write(LaScaleRegisters[i], dst_timing->la_scale_regs_arr[i]);
                }
            }

            /* Step 9:
             *   LPDDR4 section A.
             */
            if (dram_type == DRAM_TYPE_LPDDR4) {
                reg::Write(EMC + EMC_ZCAL_INTERVAL, emc_zcal_interval);
                reg::Write(EMC + EMC_ZCAL_WAIT_CNT, emc_zcal_wait_cnt_new);
                reg::Write(EMC + EMC_DBG, emc_dbg_o | 0x40000002);
                reg::Write(EMC + EMC_ZCAL_INTERVAL, emc_zcal_interval);
                reg::Write(EMC + EMC_DBG, emc_dbg_o);

                if (training_enabled) {
                    SetShadowBypass(ACTIVE);

                    reg::Write(EMC + EMC_PMACRO_AUTOCAL_CFG_COMMON, dst_timing->burst_regs.emc_pmacro_autocal_cfg_common | (1 << 16));

                    if (train_ca || train_ca_vref) {
                        reg::Write(EMC + EMC_FBIO_CFG5, src_timing->burst_regs.emc_fbio_cfg5 | (1 << 27));
                    }

                    SetShadowBypass(ASSEMBLY);

                    if (dual_channel) {
                        CcfifoWrite(EMC_CFG_SYNC, 0, 0);
                    }

                    /* Change CFG_SWAP. */
                    CcfifoWrite(EMC_DBG, ((emc_dbg_o & 0xF3FFFFFF) | 0x4000000), 0);
                }
            }

            /* Step 10:
             *   LPDDR4 and DDR3 common section.
             */
            if (opt_dvfs_mode == MAN_SR || dram_type == DRAM_TYPE_LPDDR4) {
                if (dram_type == DRAM_TYPE_LPDDR4) {
                    CcfifoWrite(EMC_SELF_REF, 0x101, 0);
                } else {
                    CcfifoWrite(EMC_SELF_REF, 0x1, 0);
                }

                if (!(train_ca || train_ca_vref) && (dram_type == DRAM_TYPE_LPDDR4) && (source_clock_period <= zqcal_before_cc_cutoff)) {
                    CcfifoWrite(EMC_MRW3, mr13_flip_fspwr ^ 0x40, 0);
                    CcfifoWrite(EMC_MRW6, (dst_timing->burst_regs.emc_mrw6 & 0xFFFF3F3F) | (src_timing->burst_regs.emc_mrw6 & 0x0000C0C0), 0);
                    CcfifoWrite(EMC_MRW14, (dst_timing->burst_regs.emc_mrw14 & 0xFFFF0707) | (src_timing->burst_regs.emc_mrw14 & 0x00003838), 0);

                    if (dram_dev_num == TWO_RANK) {
                        CcfifoWrite(EMC_MRW7, (dst_timing->burst_regs.emc_mrw7 & 0xFFFF3F3F) | (src_timing->burst_regs.emc_mrw7 & 0x0000C0C0), 0);
                        CcfifoWrite(EMC_MRW15, (dst_timing->burst_regs.emc_mrw15 & 0xFFFF0707) | (src_timing->burst_regs.emc_mrw15 & 0x00003838), 0);
                    }

                    if (opt_zcal_en_cc) {
                        if ((dram_dev_num == ONE_RANK) || shared_zq_resistor) {
                            CcfifoWrite(EMC_ZQ_CAL, 2 << 30 | (1 << 0), 0);
                        } else {
                            CcfifoWrite(EMC_ZQ_CAL, (1 << 0), 0);
                        }
                    }
                }
            }

            emc_dbg = emc_dbg_o;
            if (dram_type == DRAM_TYPE_LPDDR4) {
                if (training_enabled) {
                    /* Change CFG_SWAP. */
                    emc_dbg = ((emc_dbg_o & 0xF3FFFFFF) | 0x4000000 |  (1 << 30));
                    CcfifoWrite(EMC_DBG, emc_dbg, 0);
                }
                if (train_ca || train_ca_vref) {
                    CcfifoWrite(EMC_PMACRO_DATA_RX_TERM_MODE, src_timing->burst_regs.emc_pmacro_data_rx_term_mode & 0xFFFFFCCC, 0);

                    if ((dram_dev_num == TWO_RANK) && train_second_rank) {
                        CcfifoWrite(EMC_MRW3, mr13_flip_fspop | 0x8, (1000 * src_timing->dram_timings.t_rp) / source_clock_period);
                        CcfifoWrite(EMC_MRW3, mr13_catr_enable | 0x8, 0);
                    } else {
                        CcfifoWrite(EMC_MRW3, mr13_catr_enable | 0x8, (1000 * src_timing->dram_timings.t_rp) / source_clock_period);
                    }

                    CcfifoWrite(EMC_TR_CTRL_0, 0x15A, 0);
                    CcfifoWrite(EMC_INTSTATUS, 0, 1000000 / source_clock_period);
                } else {
                    CcfifoWrite(EMC_MRW3, mr13_flip_fspop | 0x8, (1000 * src_timing->dram_timings.t_rp) / source_clock_period);
                    CcfifoWrite(EMC_INTSTATUS, 0, tFC_lpddr4 / source_clock_period);
                }
            }

            bool ref_b4_sref_en = false;
            bool cya_issue_pc_ref = false;
            bool cya_allow_ref_cc = false;

            if ((dram_type == DRAM_TYPE_LPDDR4) || (opt_dvfs_mode != MAN_SR)) {
                uint32_t t = 30 + (cya_allow_ref_cc ? (4000 * src_timing->dram_timings.t_rfc) + ((1000 * src_timing->dram_timings.t_rp) / source_clock_period) : 0);
                CcfifoWrite(EMC_PIN, emc_pin_o & 0xFFFFFFF8, t);
            }

            uint32_t ref_delay_mult = 1;
            ref_delay_mult += ref_b4_sref_en   ? 1 : 0;
            ref_delay_mult += cya_allow_ref_cc ? 1 : 0;
            ref_delay_mult += cya_issue_pc_ref ? 1 : 0;
            uint32_t ref_delay = ref_delay_mult * ((1000 * src_timing->dram_timings.t_rp / source_clock_period) + (1000 * src_timing->dram_timings.t_rfc / source_clock_period)) + 20;

            /* Step 11:
             *   Ramp down.
             */
            CcfifoWrite(EMC_CFG_SYNC, 0, (dram_type == DRAM_TYPE_LPDDR4) ? 0 : ref_delay);
            CcfifoWrite(EMC_DBG, emc_dbg | ((1 << 1) | (1 << 30)), 0);
            uint32_t ramp_down_wait = DvfsPowerRampDown(false, src_timing, dst_timing, source_clock_period);

            /* Step 12:
             *   Trigger the clock change.
             */
            CcfifoWrite(EMC_STALL_THEN_EXE_AFTER_CLKCHANGE, 1, 0);
            if (!training_enabled) {
                CcfifoWrite(EMC_DBG, (emc_dbg & ~(1 << 30)) | (1 << 1), 0);
            }

            /* Step 13:
             *   Ramp up.
             */
            uint32_t ramp_up_wait = DvfsPowerRampUp(false, src_timing, dst_timing, training, destination_clock_period);
            CcfifoWrite(EMC_DBG, emc_dbg, 0);

            /* Step 14:
             *   Bringup CKE pins.
             */
            if ((dram_type == DRAM_TYPE_LPDDR4)) {
                uint32_t r = emc_pin_o & 0xFFFFFFF8;
                if (train_ca || train_ca_vref) {
                    if (dram_dev_num == TWO_RANK) {
                        if (train_second_rank) {
                            CcfifoWrite(EMC_PIN, r | 5, 0);
                        } else {
                            CcfifoWrite(EMC_PIN, r | 6, 0);
                        }
                    } else {
                        CcfifoWrite(EMC_PIN, r, 0);
                    }
                } else if (dram_dev_num == TWO_RANK) {
                    CcfifoWrite(EMC_PIN, r | 7, 0);
                } else {
                    CcfifoWrite(EMC_PIN, r | 1, 0);
                }
            }

            /* Step 15:
             *   Calculate zqlatch wait time; has dependency on ramping times.
             */
            if (source_clock_period <= zqcal_before_cc_cutoff) {
                int t = (int)(ramp_up_wait + ramp_down_wait) / (int)destination_clock_period;
                zq_latch_dvfs_wait_time = (int)tZQCAL_lpddr4_fc_adj - t;
            } else {
                zq_latch_dvfs_wait_time = tZQCAL_lpddr4_fc_adj - util::DivideUp(1000 * dst_timing->dram_timings.t_pdex, destination_clock_period);
            }

            if (!(train_ca || train_ca_vref) && (dram_type == DRAM_TYPE_LPDDR4) && opt_zcal_en_cc) {
                if (dram_dev_num == ONE_RANK) {
                    if (source_clock_period > zqcal_before_cc_cutoff) {
                        CcfifoWrite(EMC_ZQ_CAL, (2 << 30) | (1 << 0), util::DivideUp(1000 * dst_timing->dram_timings.t_pdex, destination_clock_period));
                    }

                    if (!training_enabled) {
                        CcfifoWrite(EMC_MRW3, (mr13_flip_fspop & 0xF3FFFFF7) | 0xC000000, util::DivideUp(1000 * dst_timing->dram_timings.t_pdex, destination_clock_period));
                        CcfifoWrite(EMC_SELF_REF, 0x100, 0);
                        CcfifoWrite(EMC_REF, 0, 0);
                    }

                    CcfifoWrite(EMC_ZQ_CAL, (2 << 30) | (1 << 1), std::max<int>(0, zq_latch_dvfs_wait_time));
                } else if (shared_zq_resistor) {
                    if (source_clock_period > zqcal_before_cc_cutoff) {
                        CcfifoWrite(EMC_ZQ_CAL, (2 << 30) | (1 << 0), util::DivideUp(1000 * dst_timing->dram_timings.t_pdex, destination_clock_period));
                    }

                    CcfifoWrite(EMC_ZQ_CAL, (2 << 30) | (1 << 1), std::max<int>(0, zq_latch_dvfs_wait_time) + util::DivideUp(1000 * dst_timing->dram_timings.t_pdex, destination_clock_period));
                    CcfifoWrite(EMC_ZQ_CAL, (1 << 30) | (1 << 1), 0);

                    if (!training_enabled) {
                        CcfifoWrite(EMC_MRW3, (mr13_flip_fspop & 0xF3FFFFF7) | 0xC000000, 0);
                        CcfifoWrite(EMC_SELF_REF, 0x100, 0);
                        CcfifoWrite(EMC_REF, 0, 0);
                    }

                    CcfifoWrite(EMC_ZQ_CAL, (1 << 30) | (1 << 1), tZQCAL_lpddr4 / destination_clock_period);
                } else {
                    if (source_clock_period > zqcal_before_cc_cutoff) {
                        CcfifoWrite(EMC_ZQ_CAL, (1 << 0), util::DivideUp(1000 * dst_timing->dram_timings.t_pdex, destination_clock_period));
                    }

                    if (!training_enabled) {
                        CcfifoWrite(EMC_MRW3, (mr13_flip_fspop & 0xF3FFFFF7) | 0xC000000, util::DivideUp(1000 * dst_timing->dram_timings.t_pdex, destination_clock_period));
                        CcfifoWrite(EMC_SELF_REF, 0x100, 0);
                        CcfifoWrite(EMC_REF, 0, 0);
                    }

                    CcfifoWrite(EMC_ZQ_CAL, (1 << 1), std::max<int>(0, zq_latch_dvfs_wait_time));
                }
            }

            /* WAR: delay for zqlatch */
            CcfifoWrite(EMC_INTSTATUS, 0, 10);

            /* Step 16:
             *   LPDDR4 Conditional Training Kickoff.
             */
            if (training_enabled && (dram_type == DRAM_TYPE_LPDDR4)) {
                CcfifoWrite(EMC_INTSTATUS, 0, (1020000 / destination_clock_period));

                uint32_t train_cmd = 0;

                if (train_ca)
                    train_cmd |= (1 << 1);  /* CA */
                if (train_ca_vref)
                    train_cmd |= (1 << 5);  /* CA_VREF */
                if (train_quse)
                    train_cmd |= (1 << 4);  /* QUSE */
                if (train_quse_vref)
                    train_cmd |= (1 << 8);  /* QUSE_VREF */
                if (train_wr)
                    train_cmd |= (1 << 3);  /* WR */
                if (train_wr_vref)
                    train_cmd |= (1 << 6);  /* WR_VREF */
                if (train_rd)
                    train_cmd |= (1 << 2);  /* RD */
                if (train_rd_vref)
                    train_cmd |= (1 << 7);  /* RD_VREF */

                train_cmd |= (1 << 31);     /* GO */

                CcfifoWrite(EMC_TRAINING_CMD, train_cmd, 0);

                if (bg_regulator_mode_change) {
                    if (enable_bg_regulator)
                        CcfifoWrite(EMC_PMACRO_BG_BIAS_CTRL_0, src_timing->burst_regs.emc_pmacro_bg_bias_ctrl_0 & ~(1 << 0), 0);
                    else
                        CcfifoWrite(EMC_PMACRO_BG_BIAS_CTRL_0, src_timing->burst_regs.emc_pmacro_bg_bias_ctrl_0 & ~(1 << 2), 0);
                }

                CcfifoWrite(EMC_SWITCH_BACK_CTRL, 1, 0);

                if (!(train_ca || train_ca_vref) || train_second_rank) {
                    CcfifoWrite(EMC_MRW3, mr13_flip_fspop ^ 0xC0, 0);
                    CcfifoWrite(EMC_INTSTATUS, 0, (1000000 / destination_clock_period));
                }

                CcfifoWrite(EMC_PIN, emc_pin_o & 0xFFFFFFF8, 0);
                CcfifoWrite(EMC_CFG_SYNC, 0, 0);
                CcfifoWrite(EMC_DBG, emc_dbg | ((1 << 30) | (1 << 1)), 0);

                DvfsPowerRampDown(true, src_timing, dst_timing, destination_clock_period);

                CcfifoWrite(EMC_STALL_THEN_EXE_AFTER_CLKCHANGE, 1, 0);
                CcfifoWrite(EMC_DBG, (emc_dbg & ~(1 << 30)) | (1 << 1), 0);

                DvfsPowerRampUp(true, src_timing, dst_timing, training, source_clock_period);

                CcfifoWrite(EMC_DBG, emc_dbg, 0);

                if (dram_dev_num == TWO_RANK) {
                    CcfifoWrite(EMC_PIN, emc_pin_o | 7, 0);
                } else {
                    CcfifoWrite(EMC_PIN, ((emc_pin_o & 0xFFFFFFF8) | 1), 0);
                }

                if (train_ca || train_ca_vref) {
                    CcfifoWrite(EMC_TR_CTRL_0, 0x4A, (200000 / source_clock_period));
                    CcfifoWrite(EMC_TR_CTRL_0, 0x40, (1000000 / source_clock_period));
                    CcfifoWrite(EMC_MRW3, mr13_catr_enable & 0xFFFFFFFE, 0);
                    CcfifoWrite(EMC_INTSTATUS, 0, (1000000 / source_clock_period));
                    CcfifoWrite(EMC_PMACRO_DATA_RX_TERM_MODE, src_timing->burst_regs.emc_pmacro_data_rx_term_mode, 0);
                }

                CcfifoWrite(EMC_DBG, emc_dbg_o, 0);

                if (opt_zcal_en_cc) {
                    CcfifoWrite(EMC_ZQ_CAL, (2 << 30) | (1 << 0), 0);
                    CcfifoWrite(EMC_ZQ_CAL, (2 << 30) | (1 << 1), (1000000 / source_clock_period));

                    if (dram_dev_num == TWO_RANK) {
                        if (shared_zq_resistor) {
                            if (!(train_ca || train_ca_vref) || train_second_rank) {
                                CcfifoWrite(EMC_ZQ_CAL, (1 << 30) | (1 << 0), 0);
                                CcfifoWrite(EMC_ZQ_CAL, (1 << 30) | (1 << 1), (1000000 / source_clock_period));

                                if (!(train_ca || train_ca_vref))
                                    CcfifoWrite(EMC_MRW3, ((mr13_flip_fspop ^ 0xC0) & 0xF3FFFFF7) | 0xC000000, 0);
                            }

                            CcfifoWrite(EMC_SELF_REF, 0x100, 0);
                            skip_zqcal = true;
                        } else {
                            if ((train_ca || train_ca_vref) && !train_second_rank) {
                                CcfifoWrite(EMC_SELF_REF, 0x100, 0);
                                skip_zqcal = true;
                            } else {
                                CcfifoWrite(EMC_ZQ_CAL, (1 << 30) | (1 << 0), 0);
                                CcfifoWrite(EMC_ZQ_CAL, (1 << 30) | (1 << 1), (1000000 / source_clock_period));
                            }
                        }
                    }
                }

                if (!skip_zqcal) {
                    if (!(train_ca || train_ca_vref))
                        CcfifoWrite(EMC_MRW3, ((mr13_flip_fspop ^ 0xC0) & 0xF3FFFFF7) | 0xC000000, 0);

                    CcfifoWrite(EMC_SELF_REF, 0x100, 0);
                }
            }

            if (!skip_zqcal) {
                /* Step 17:
                 *   MANSR exit self refresh.
                 */

                if ((opt_dvfs_mode == MAN_SR) && (dram_type != DRAM_TYPE_LPDDR4))
                    CcfifoWrite(EMC_SELF_REF, 0, 0);

                /* Step 18:
                 *   Send MRWs to LPDDR3/DDR3.
                 */

                if (dram_type == DRAM_TYPE_LPDDR2) {
                    CcfifoWrite(EMC_MRW2, dst_timing->emc_mrw2, 0);
                    CcfifoWrite(EMC_MRW, dst_timing->emc_mrw, 0);

                    if (is_lpddr3) {
                        CcfifoWrite(EMC_MRW4, dst_timing->emc_mrw4, 0);
                    }
                } else if (dram_type == DRAM_TYPE_DDR4) {
                    if (opt_dll_mode == DLL_ON) {
                        CcfifoWrite(EMC_EMRS, dst_timing->emc_emrs & ~(1 << 26), 0);
                    }
                    CcfifoWrite(EMC_EMRS2, dst_timing->emc_emrs2 & ~(1 << 26), 0);
                    CcfifoWrite(EMC_MRS, dst_timing->emc_mrs | (1 << 26), 0);
                }

                /* Step 19:
                 *   ZQCAL for LPDDR3/DDR3
                 */

                if (opt_zcal_en_cc) {
                    if (dram_type == DRAM_TYPE_LPDDR2) {
                        uint32_t r;
                        uint32_t zq_op = opt_cc_short_zcal ? 0x56 : 0xAB;
                        uint32_t zcal_wait_time_ps = opt_cc_short_zcal ? 90000 : 360000;
                        uint32_t zcal_wait_time_clocks = util::DivideUp(zcal_wait_time_ps, destination_clock_period);
                        r = (zcal_wait_time_clocks << 16) | (zcal_wait_time_clocks << 0);

                        CcfifoWrite(EMC_MRS_WAIT_CNT2, r, 0);
                        CcfifoWrite(EMC_MRW, (2 << 30) | (1 << 27) | (10 << 16) | (zq_op << 0), 0);

                        if (dram_dev_num == TWO_RANK) {
                            r = (1 << 30) | (1 << 27) | (10 << 16) | (zq_op << 0);
                            CcfifoWrite(EMC_MRW, r, 0);
                        }
                    } else if (dram_type == DRAM_TYPE_DDR4) {
                        uint32_t zq_op = opt_cc_short_zcal ? 0 : (1 << 4);
                        CcfifoWrite(EMC_ZQ_CAL, zq_op | (2 << 30) | (1 << 0), 0);

                        if (dram_dev_num == TWO_RANK) {
                            CcfifoWrite(EMC_ZQ_CAL, zq_op | (1 << 30) | (1 << 0), 0);
                        }
                    }
                }
            }

            if (bg_regulator_mode_change) {
                SetShadowBypass(ACTIVE);

                uint32_t bg_regulator_switch_complete_wait_clks = ramp_up_wait > 1250000 ? 0 : (1250000 - ramp_up_wait) / destination_clock_period;

                if (training_enabled) {
                    bg_regulator_switch_complete_wait_clks = (1250000 / source_clock_period);
                    CcfifoWrite(EMC_PMACRO_BG_BIAS_CTRL_0, src_timing->burst_regs.emc_pmacro_bg_bias_ctrl_0, bg_regulator_switch_complete_wait_clks);
                } else {
                    CcfifoWrite(EMC_PMACRO_BG_BIAS_CTRL_0, dst_timing->burst_regs.emc_pmacro_bg_bias_ctrl_0, bg_regulator_switch_complete_wait_clks);
                }

                SetShadowBypass(ASSEMBLY);
            }

            /* Step 20:
             *   Issue ref and optional QRST.
             */
            if (training_enabled || (dram_type != DRAM_TYPE_LPDDR4)) {
                CcfifoWrite(EMC_REF, 0, 0);
            }

            if (opt_do_sw_qrst) {
                CcfifoWrite(EMC_ISSUE_QRST, 1, 0);
                CcfifoWrite(EMC_ISSUE_QRST, 0, 2);
            }

            /* Step 21:
             *   Restore ZCAL and ZCAL interval.
             */
            if (save_restore_clkstop_pd || opt_zcal_en_cc) {
                SetShadowBypass(ACTIVE);

                if (opt_zcal_en_cc) {
                    if (training_enabled) {
                        CcfifoWrite(EMC_ZCAL_INTERVAL, src_timing->burst_regs.emc_zcal_interval, 0);
                    } else if (dram_type != DRAM_TYPE_LPDDR4) {
                        CcfifoWrite(EMC_ZCAL_INTERVAL, dst_timing->burst_regs.emc_zcal_interval, 0);
                    }
                }

                if (save_restore_clkstop_pd) {
                    CcfifoWrite(EMC_CFG, dst_timing->burst_regs.emc_cfg & ~(1 << 28), 0);
                }

                if (training_enabled && (dram_type == DRAM_TYPE_LPDDR4)) {
                    CcfifoWrite(EMC_SEL_DPD_CTRL, src_timing->emc_sel_dpd_ctrl, 0);
                }

                SetShadowBypass(ASSEMBLY);
            }

            /* Step 22:
             *   Restore EMC_CFG_PIPE_CLK.
             */
            CcfifoWrite(EMC_CFG_PIPE_CLK, emc_cfg_pipe_clk_o, 0);

            if (bg_regulator_mode_change) {
                if (enable_bg_regulator) {
                    reg::Write(EMC + EMC_PMACRO_BG_BIAS_CTRL_0, dst_timing->burst_regs.emc_pmacro_bg_bias_ctrl_0 & ~(1 << 2));
                } else {
                    reg::Write(EMC + EMC_PMACRO_BG_BIAS_CTRL_0, dst_timing->burst_regs.emc_pmacro_bg_bias_ctrl_0 & ~(1 << 0));
                }
            }

            /* Step 23:
             *   Do clock change.
             */
            if (training_enabled) {
                u32 cur_clk_src = reg::Read(CLKRST + CLK_RST_CONTROLLER_CLK_SOURCE_EMC);
                reg::Write(CLKRST + CLK_RST_CONTROLLER_CLK_SOURCE_EMC_SAFE, cur_clk_src);
                ChangeDllSrc(src_timing, cur_clk_src);
            }

            uint32_t cfg_dig_dll_tmp = (reg::Read(EMC + EMC_CFG_DIG_DLL) & 0xFFFFFF24) | 0x88;
            reg::Write(EMC + EMC_CFG_DIG_DLL, cfg_dig_dll_tmp);

            reg::Write(CLKRST + CLK_RST_CONTROLLER_CLK_SOURCE_EMC, next_clk_src);
            WaitForUpdate(EMC_INTSTATUS, 0x10, true, fbio_cfg7);

            /* Step 24:
             *   Save training results.
             */
            if (training_enabled) {
                uint32_t emc_dbg_tmp = reg::Read(EMC + EMC_DBG);
                reg::Write(EMC + EMC_DBG, emc_dbg_tmp | 1);       /* Set READ_MUX to ASSEMBLY. */

                /* Save CA results. */
                if (train_ca) {
                    dst_timing->trim_perch_regs.emc0_cmd_brlshft_0 = reg::Read(EMC0 + EMC_CMD_BRLSHFT_0);
                    dst_timing->trim_perch_regs.emc1_cmd_brlshft_1 = dual_channel ? reg::Read(EMC1 + EMC_CMD_BRLSHFT_1): 0;
                    dst_timing->trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_4 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_4);
                    dst_timing->trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_5 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_5) : 0;

                    if (train_bit_level) {
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd0_0 = reg::Read(EMC + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD0_0);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd0_1 = reg::Read(EMC + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD0_1);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd0_2 = reg::Read(EMC + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD0_2);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd1_0 = reg::Read(EMC + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD1_0);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd1_1 = reg::Read(EMC + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD1_1);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd1_2 = reg::Read(EMC + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD1_2);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd2_0 = reg::Read(EMC + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD2_0);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd2_1 = reg::Read(EMC + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD2_1);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd2_2 = reg::Read(EMC + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD2_2);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd3_0 = reg::Read(EMC + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD3_0);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd3_1 = reg::Read(EMC + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD3_1);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd3_2 = reg::Read(EMC + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD3_2);
                    }
                }

                /* Save CA_VREF results. */
                if (train_ca_vref) {
                    dst_timing->burst_perch_regs.emc0_mrw10 = (reg::Read(EMC0 + EMC_TRAINING_OPT_CA_VREF) & 0xFFFF) | 0x880C0000;
                    dst_timing->burst_perch_regs.emc1_mrw10 = (dual_channel ? reg::Read(EMC1 + EMC_TRAINING_OPT_CA_VREF) & 0xFFFF : 0) | 0x880C0000;

                    if (dram_dev_num == TWO_RANK) {
                        dst_timing->burst_perch_regs.emc0_mrw11 = ((reg::Read(EMC0 + EMC_TRAINING_OPT_CA_VREF) >> 16) & 0xFF) | (reg::Read(EMC0 + EMC_TRAINING_OPT_CA_VREF) >> 24 << 8) | (0x480C0000 & 0xFFFFFF00);
                        dst_timing->burst_perch_regs.emc1_mrw11 = (((dual_channel ? reg::Read(EMC1 + EMC_TRAINING_OPT_CA_VREF) : 0) >> 16) & 0xFF) | ((dual_channel ? reg::Read(EMC1 + EMC_TRAINING_OPT_CA_VREF) : 0) >> 24 << 8) | (0x480C0000 & 0xFFFFFF00);
                    } else {
                        dst_timing->burst_perch_regs.emc0_mrw11 = ((reg::Read(EMC0 + EMC_TRAINING_OPT_CA_VREF) >> 16) & 0xFF) | (reg::Read(EMC0 + EMC_TRAINING_OPT_CA_VREF) >> 24 << 8) | (0xC80C0000 & 0xFFFFFF00);
                        dst_timing->burst_perch_regs.emc1_mrw11 = (((dual_channel ? reg::Read(EMC1 + EMC_TRAINING_OPT_CA_VREF) : 0) >> 16) & 0xFF) | ((dual_channel ? reg::Read(EMC1 + EMC_TRAINING_OPT_CA_VREF) : 0) >> 24 << 8) | (0xC80C0000 & 0xFFFFFF00);
                    }
                }

                /* Save QUSE results. */
                if (train_quse || train_rd) {
                    dst_timing->trim_perch_regs.emc0_quse_brlshft_0 = reg::Read(EMC0 + EMC_QUSE_BRLSHFT_0);
                    dst_timing->trim_perch_regs.emc1_quse_brlshft_1 = dual_channel ? reg::Read(EMC1 + EMC_QUSE_BRLSHFT_1) : 0;

                    dst_timing->trim_regs.emc_pmacro_quse_ddll_rank0_0 = reg::Read(EMC0 + EMC_PMACRO_QUSE_DDLL_RANK0_0);
                    dst_timing->trim_regs.emc_pmacro_quse_ddll_rank0_1= reg::Read(EMC0 + EMC_PMACRO_QUSE_DDLL_RANK0_1);
                    dst_timing->trim_regs.emc_pmacro_quse_ddll_rank0_2 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_QUSE_DDLL_RANK0_2) : 0;
                    dst_timing->trim_regs.emc_pmacro_quse_ddll_rank0_3 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_QUSE_DDLL_RANK0_3) : 0;

                    if (dram_dev_num == TWO_RANK) {
                        dst_timing->trim_perch_regs.emc0_quse_brlshft_2 = reg::Read(EMC0 + EMC_QUSE_BRLSHFT_2);
                        dst_timing->trim_perch_regs.emc1_quse_brlshft_3 = dual_channel ? reg::Read(EMC1 + EMC_QUSE_BRLSHFT_3) : 0;

                        dst_timing->trim_regs.emc_pmacro_quse_ddll_rank1_0 = reg::Read(EMC0 + EMC_PMACRO_QUSE_DDLL_RANK1_0);
                        dst_timing->trim_regs.emc_pmacro_quse_ddll_rank1_1 = reg::Read(EMC0 + EMC_PMACRO_QUSE_DDLL_RANK1_1);
                        dst_timing->trim_regs.emc_pmacro_quse_ddll_rank1_2 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_QUSE_DDLL_RANK1_2) : 0;
                        dst_timing->trim_regs.emc_pmacro_quse_ddll_rank1_3 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_QUSE_DDLL_RANK1_3) : 0;
                    }
                }

                /* Save QUSE_VREF results. */
                if (train_quse_vref) {
                    if (dram_dev_num == TWO_RANK) {
                        uint32_t emc0_opt_dqs_array[4] = {0};
                        uint32_t emc1_opt_dqs_array[4] = {0};
                        uint32_t emc1_training_opt_dqs_ib_vref_rank0_val = dual_channel ? reg::Read(EMC1 + EMC_TRAINING_OPT_DQS_IB_VREF_RANK0) : 0;
                        uint32_t emc1_training_opt_dqs_ib_vref_rank1_val = dual_channel ? reg::Read(EMC1 + EMC_TRAINING_OPT_DQS_IB_VREF_RANK1) : 0;

                        for (int i = 0; i < 4; i++) {
                            emc0_opt_dqs_array[i] = (reg::Read(EMC0 + EMC_TRAINING_OPT_DQS_IB_VREF_RANK0) >> (8 * i)) & 0xFF;
                            emc1_opt_dqs_array[i] = (emc1_training_opt_dqs_ib_vref_rank0_val >> (8 * i)) & 0xFF;
                        }

                        uint32_t ib_vref_dqs_0 = 0;
                        uint32_t ib_vref_dqs_1 = 0;
                        for (int i = 0; i < 4; i++)
                        {
                            ib_vref_dqs_0 |= (emc0_opt_dqs_array[i] + ((reg::Read(EMC0 + EMC_TRAINING_OPT_DQS_IB_VREF_RANK1) >> (8 * i)) & 0xFF)) >> 1 << (8 * i);
                            ib_vref_dqs_1 |= (emc1_opt_dqs_array[i] + ((emc1_training_opt_dqs_ib_vref_rank1_val >> (8 * i)) & 0xFF)) >> 1 << (8 * i);
                        }

                        dst_timing->trim_regs.emc_pmacro_ib_vref_dqs_0 = ib_vref_dqs_0;
                        dst_timing->trim_regs.emc_pmacro_ib_vref_dqs_1 = ib_vref_dqs_1;
                    }
                    else
                    {
                        dst_timing->trim_regs.emc_pmacro_ib_vref_dqs_0 = reg::Read(EMC + EMC_PMACRO_IB_VREF_DQS_0);
                        dst_timing->trim_regs.emc_pmacro_ib_vref_dqs_1 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_IB_VREF_DQS_1) : 0;
                    }
                }

                /* Save RD results. */
                if (train_rd) {
                    dst_timing->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank0_0 = reg::Read(EMC0 + EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_0);
                    dst_timing->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank0_1 = reg::Read(EMC0 + EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_1);
                    dst_timing->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank0_2 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_2) : 0;
                    dst_timing->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank0_3 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_3) : 0;

                    if (dram_dev_num == TWO_RANK) {
                        dst_timing->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank1_0 = reg::Read(EMC0 + EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_0);
                        dst_timing->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank1_1 = reg::Read(EMC0 + EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_1);
                        dst_timing->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank1_2 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_2) : 0;
                        dst_timing->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank1_3 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_3) : 0;
                    }

                    if (train_bit_level) {
                        dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte0_0 = reg::Read(EMC0 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE0_0);
                        dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte0_1 = reg::Read(EMC0 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE0_1);
                        dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte0_2 = reg::Read(EMC0 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE0_2);
                        dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte1_0 = reg::Read(EMC0 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE1_0);
                        dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte1_1 = reg::Read(EMC0 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE1_1);
                        dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte1_2 = reg::Read(EMC0 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE1_2);
                        dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte2_0 = reg::Read(EMC0 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE2_0);
                        dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte2_1 = reg::Read(EMC0 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE2_1);
                        dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte2_2 = reg::Read(EMC0 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE2_2);
                        dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte3_0 = reg::Read(EMC0 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE3_0);
                        dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte3_1 = reg::Read(EMC0 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE3_1);
                        dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte3_2 = reg::Read(EMC0 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE3_2);
                        dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte4_0 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE4_0) : 0;
                        dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte4_1 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE4_1) : 0;
                        dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte4_2 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE4_2) : 0;
                        dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte5_0 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE5_0) : 0;
                        dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte5_1 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE5_1) : 0;
                        dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte5_2 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE5_2) : 0;
                        dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte6_0 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE6_0) : 0;
                        dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte6_1 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE6_1) : 0;
                        dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte6_2 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE6_2) : 0;
                        dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte7_0 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE7_0) : 0;
                        dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte7_1 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE7_1) : 0;
                        dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte7_2 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE7_2) : 0;

                        if (dram_dev_num == TWO_RANK) {
                            dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte0_0 = reg::Read(EMC0 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE0_0);
                            dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte0_1 = reg::Read(EMC0 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE0_1);
                            dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte0_2 = reg::Read(EMC0 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE0_2);
                            dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte1_0 = reg::Read(EMC0 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE1_0);
                            dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte1_1 = reg::Read(EMC0 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE1_1);
                            dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte1_2 = reg::Read(EMC0 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE1_2);
                            dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte2_0 = reg::Read(EMC0 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE2_0);
                            dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte2_1 = reg::Read(EMC0 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE2_1);
                            dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte2_2 = reg::Read(EMC0 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE2_2);
                            dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte3_0 = reg::Read(EMC0 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE3_0);
                            dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte3_1 = reg::Read(EMC0 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE3_1);
                            dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte3_2 = reg::Read(EMC0 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE3_2);
                            dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte4_0 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE4_0) : 0;
                            dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte4_1 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE4_1) : 0;
                            dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte4_2 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE4_2) : 0;
                            dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte5_0 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE5_0) : 0;
                            dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte5_1 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE5_1) : 0;
                            dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte5_2 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE5_2) : 0;
                            dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte6_0 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE6_0) : 0;
                            dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte6_1 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE6_1) : 0;
                            dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte6_2 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE6_2) : 0;
                            dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte7_0 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE7_0) : 0;
                            dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte7_1 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE7_1) : 0;
                            dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte7_2 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE7_2) : 0;
                        }
                    }

                    /* Save RD_VREF results. */
                    if (train_rd_vref) {
                        uint8_t ib_vref_dq_byte0_icr = (reg::Read(EMC + EMC_PMACRO_IB_VREF_DQ_0) & 0x7F) + (dst_timing->save_restore_mod_regs[0] & 0x7F);
                        if (dst_timing->save_restore_mod_regs[0] & 0x80000000)
                            ib_vref_dq_byte0_icr = (reg::Read(EMC + EMC_PMACRO_IB_VREF_DQ_0) & 0x7F) - (dst_timing->save_restore_mod_regs[0] & 0x7F);

                        uint8_t ib_vref_dq_byte1_icr = ((reg::Read(EMC + EMC_PMACRO_IB_VREF_DQ_0) >> 8) & 0x7F) + (dst_timing->save_restore_mod_regs[1] & 0x7F);
                        if (dst_timing->save_restore_mod_regs[1] & 0x80000000)
                            ib_vref_dq_byte1_icr = ((reg::Read(EMC + EMC_PMACRO_IB_VREF_DQ_0) >> 8) & 0x7F) - (dst_timing->save_restore_mod_regs[1] & 0x7F);

                        uint8_t ib_vref_dq_byte2_icr = ((reg::Read(EMC + EMC_PMACRO_IB_VREF_DQ_0) >> 16) & 0x7F) + (dst_timing->save_restore_mod_regs[2] & 0x7F);
                        if (dst_timing->save_restore_mod_regs[2] & 0x80000000)
                            ib_vref_dq_byte2_icr = ((reg::Read(EMC + EMC_PMACRO_IB_VREF_DQ_0) >> 16) & 0x7F) - (dst_timing->save_restore_mod_regs[2] & 0x7F);

                        uint8_t ib_vref_dq_byte3_icr = ((reg::Read(EMC + EMC_PMACRO_IB_VREF_DQ_0) >> 24) & 0x7F) + (dst_timing->save_restore_mod_regs[3] & 0x7F);
                        if (dst_timing->save_restore_mod_regs[3] & 0x80000000)
                            ib_vref_dq_byte3_icr = ((reg::Read(EMC + EMC_PMACRO_IB_VREF_DQ_0) >> 24) & 0x7F) - (dst_timing->save_restore_mod_regs[3] & 0x7F);

                        dst_timing->trim_regs.emc_pmacro_ib_vref_dq_0 = ((ib_vref_dq_byte0_icr & 0x7F) | (ib_vref_dq_byte1_icr & 0x7F) << 8) | ((ib_vref_dq_byte2_icr & 0x7F) << 16) | ((ib_vref_dq_byte3_icr & 0x7F) << 24);

                        uint8_t ib_vref_dq_byte4_icr = (reg::Read(EMC + EMC_PMACRO_IB_VREF_DQ_1) & 0x7F) + (dst_timing->save_restore_mod_regs[4] & 0x7F);
                        if (dst_timing->save_restore_mod_regs[4] & 0x80000000)
                            ib_vref_dq_byte4_icr = (reg::Read(EMC + EMC_PMACRO_IB_VREF_DQ_1) & 0x7F) - (dst_timing->save_restore_mod_regs[4] & 0x7F);

                        uint8_t ib_vref_dq_byte5_icr = ((reg::Read(EMC + EMC_PMACRO_IB_VREF_DQ_1) >> 8) & 0x7F) + (dst_timing->save_restore_mod_regs[5] & 0x7F);
                        if (dst_timing->save_restore_mod_regs[5] & 0x80000000)
                            ib_vref_dq_byte5_icr = ((reg::Read(EMC + EMC_PMACRO_IB_VREF_DQ_1) >> 8) & 0x7F) - (dst_timing->save_restore_mod_regs[5] & 0x7F);

                        uint8_t ib_vref_dq_byte6_icr = ((reg::Read(EMC + EMC_PMACRO_IB_VREF_DQ_1) >> 16) & 0x7F) + (dst_timing->save_restore_mod_regs[6] & 0x7F);
                        if (dst_timing->save_restore_mod_regs[6] & 0x80000000)
                            ib_vref_dq_byte6_icr = ((reg::Read(EMC + EMC_PMACRO_IB_VREF_DQ_1) >> 16) & 0x7F) - (dst_timing->save_restore_mod_regs[6] & 0x7F);

                        uint8_t ib_vref_dq_byte7_icr = ((reg::Read(EMC + EMC_PMACRO_IB_VREF_DQ_1) >> 24) & 0x7F) + (dst_timing->save_restore_mod_regs[7] & 0x7F);
                        if (dst_timing->save_restore_mod_regs[7] & 0x80000000)
                            ib_vref_dq_byte7_icr = ((reg::Read(EMC + EMC_PMACRO_IB_VREF_DQ_1) >> 24) & 0x7F) - (dst_timing->save_restore_mod_regs[7] & 0x7F);

                        dst_timing->trim_regs.emc_pmacro_ib_vref_dq_1 = ((ib_vref_dq_byte4_icr & 0x7F) | (ib_vref_dq_byte5_icr & 0x7F) << 8) | ((ib_vref_dq_byte6_icr & 0x7F) << 16) | ((ib_vref_dq_byte7_icr & 0x7F) << 24);
                    }
                }

                /* Save WR results. */
                if (train_wr) {
                    dst_timing->trim_perch_regs.emc0_data_brlshft_0 = reg::Read(EMC0 + EMC_DATA_BRLSHFT_0);
                    dst_timing->trim_perch_regs.emc1_data_brlshft_0 = dual_channel ? reg::Read(EMC1 + EMC_DATA_BRLSHFT_0) : 0;

                    if (dram_dev_num == TWO_RANK) {
                        dst_timing->trim_perch_regs.emc0_data_brlshft_1 = reg::Read(EMC0 + EMC_DATA_BRLSHFT_1);
                        dst_timing->trim_perch_regs.emc1_data_brlshft_1 = dual_channel ? reg::Read(EMC1 + EMC_DATA_BRLSHFT_1) : 0;
                    }

                    dst_timing->trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_0 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_0);
                    dst_timing->trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_1 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_1);
                    dst_timing->trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_2 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_2) : 0;
                    dst_timing->trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_3 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_3) : 0;

                    if (dram_dev_num == TWO_RANK) {
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_long_dq_rank1_0 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_0);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_long_dq_rank1_1 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_1);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_long_dq_rank1_2 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_2) : 0;
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_long_dq_rank1_3 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_3) : 0;
                    }

                    if (train_bit_level) {
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte0_0 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE0_0);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte0_1 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE0_1);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte0_2 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE0_2);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte1_0 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE1_0);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte1_1 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE1_1);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte1_2 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE1_2);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte2_0 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE2_0);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte2_1 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE2_1);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte2_2 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE2_2);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte3_0 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE3_0);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte3_1 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE3_1);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte3_2 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE3_2);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte4_0 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE4_0) : 0;
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte4_1 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE4_1) : 0;
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte4_2 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE4_2) : 0;
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte5_0 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE5_0) : 0;
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte5_1 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE5_1) : 0;
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte5_2 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE5_2) : 0;
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte6_0 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE6_0) : 0;
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte6_1 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE6_1) : 0;
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte6_2 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE6_2) : 0;
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte7_0 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE7_0) : 0;
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte7_1 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE7_1) : 0;
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte7_2 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE7_2) : 0;

                        if (dram_dev_num == TWO_RANK) {
                            dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte0_0 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE0_0);
                            dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte0_1 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE0_1);
                            dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte0_2 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE0_2);
                            dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte1_0 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE1_0);
                            dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte1_1 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE1_1);
                            dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte1_2 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE1_2);
                            dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte2_0 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE2_0);
                            dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte2_1 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE2_1);
                            dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte2_2 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE2_2);
                            dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte3_0 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE3_0);
                            dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte3_1 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE3_1);
                            dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte3_2 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE3_2);
                            dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte4_0 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE4_0) : 0;
                            dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte4_1 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE4_1) : 0;
                            dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte4_2 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE4_2) : 0;
                            dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte5_0 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE5_0) : 0;
                            dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte5_1 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE5_1) : 0;
                            dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte5_2 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE5_2) : 0;
                            dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte6_0 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE6_0) : 0;
                            dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte6_1 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE6_1) : 0;
                            dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte6_2 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE6_2) : 0;
                            dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte7_0 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE7_0) : 0;
                            dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte7_1 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE7_1) : 0;
                            dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte7_2 = dual_channel ? reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE7_2) : 0;
                        }
                    }

                    /* Save WR_VREF results. */
                    if (train_wr_vref) {
                        uint32_t emc1_ranks_sub_partitions = dual_channel ? reg::Read(EMC1 + EMC_TRAINING_OPT_DQ_OB_VREF) : 0;

                        uint8_t emc0_ib_vref_dq_byte8_modded_plus = dst_timing->save_restore_mod_regs[8] + reg::Read(EMC0 + EMC_TRAINING_OPT_DQ_OB_VREF);
                        if (dst_timing->save_restore_mod_regs[8] & 0x80000000)
                            emc0_ib_vref_dq_byte8_modded_plus = reg::Read(EMC0 + EMC_TRAINING_OPT_DQ_OB_VREF) - dst_timing->save_restore_mod_regs[8];

                        uint8_t emc0_mrw12_op_sp1 = ((reg::Read(EMC0 + EMC_TRAINING_OPT_DQ_OB_VREF) & 0xFFFF) >> 8) + dst_timing->save_restore_mod_regs[9];
                        if (dst_timing->save_restore_mod_regs[9] & 0x80000000)
                            emc0_mrw12_op_sp1 = ((reg::Read(EMC0 + EMC_TRAINING_OPT_DQ_OB_VREF) & 0xFFFF) >> 8) - dst_timing->save_restore_mod_regs[9];

                        uint8_t emc0_mrw13_op_sp0 = ((reg::Read(EMC0 + EMC_TRAINING_OPT_DQ_OB_VREF) >> 16) & 0xFF) + dst_timing->save_restore_mod_regs[8];
                        if (dst_timing->save_restore_mod_regs[8] & 0x80000000)
                            emc0_mrw13_op_sp0 = ((reg::Read(EMC0 + EMC_TRAINING_OPT_DQ_OB_VREF) >> 16) & 0xFF) - dst_timing->save_restore_mod_regs[8];

                        uint8_t emc0_ib_vref_dq_byte9_modded_a_plus = dst_timing->save_restore_mod_regs[9] + (reg::Read(EMC0 + EMC_TRAINING_OPT_DQ_OB_VREF) >> 24);
                        if (dst_timing->save_restore_mod_regs[9] & 0x80000000)
                            emc0_ib_vref_dq_byte9_modded_a_plus = (reg::Read(EMC0 + EMC_TRAINING_OPT_DQ_OB_VREF) >> 24) - (uint8_t)dst_timing->save_restore_mod_regs[9];

                        uint8_t emc0_ib_vref_dq_byte10_modded_plus = emc1_ranks_sub_partitions + dst_timing->save_restore_mod_regs[10];
                        if (dst_timing->save_restore_mod_regs[10] & 0x80000000)
                            emc0_ib_vref_dq_byte10_modded_plus = emc1_ranks_sub_partitions - dst_timing->save_restore_mod_regs[10];

                        uint8_t emc0_ib_vref_dq_byte11_modded_plus = ((emc1_ranks_sub_partitions & 0xFFFF) >> 8) + dst_timing->save_restore_mod_regs[11];
                        if (dst_timing->save_restore_mod_regs[11] & 0x80000000)
                            emc0_ib_vref_dq_byte11_modded_plus = ((emc1_ranks_sub_partitions & 0xFFFF) >> 8) - dst_timing->save_restore_mod_regs[11];

                        uint8_t emc1_mrw13_op_sp0 = ((emc1_ranks_sub_partitions >> 16) & 0xFF) + dst_timing->save_restore_mod_regs[10];
                        if (dst_timing->save_restore_mod_regs[10] & 0x80000000)
                            emc1_mrw13_op_sp0 = ((emc1_ranks_sub_partitions >> 16) & 0xFF) - dst_timing->save_restore_mod_regs[10];

                        uint8_t emc1_mrw13_op_sp1 = (emc1_ranks_sub_partitions >> 24) + dst_timing->save_restore_mod_regs[11];
                        if (dst_timing->save_restore_mod_regs[11] & 0x80000000)
                            emc1_mrw13_op_sp1 = (emc1_ranks_sub_partitions >> 24) - dst_timing->save_restore_mod_regs[11];

                        dst_timing->burst_perch_regs.emc1_mrw12 = (uint8_t)emc0_ib_vref_dq_byte10_modded_plus | 0x880E0000 | (emc0_ib_vref_dq_byte11_modded_plus << 8);
                        dst_timing->burst_perch_regs.emc0_mrw12  = emc0_ib_vref_dq_byte8_modded_plus | 0x880E0000 | (emc0_mrw12_op_sp1 << 8);

                        if (dram_dev_num == TWO_RANK) {
                            dst_timing->burst_perch_regs.emc0_mrw13 = emc0_ib_vref_dq_byte9_modded_a_plus << 8 | emc0_mrw13_op_sp0 | 0x480E0000;
                            dst_timing->burst_perch_regs.emc1_mrw13 = (emc1_mrw13_op_sp1 << 8) | emc1_mrw13_op_sp0 | 0x480E0000;
                        } else {
                            dst_timing->burst_perch_regs.emc0_mrw13 = emc0_ib_vref_dq_byte9_modded_a_plus << 8 | emc0_mrw13_op_sp0 | 0xC80E0000;
                            dst_timing->burst_perch_regs.emc1_mrw13 = (emc1_mrw13_op_sp1 << 8) | emc1_mrw13_op_sp0 | 0xC80E0000;
                        }
                    }
                }

                reg::Write(EMC + EMC_DBG, emc_dbg_tmp);
            }

            /* Step 25:
             *   Program MC updown registers.
             */
            if ((dst_timing->rate_khz > src_timing->rate_khz) && !training_enabled) {
                for (u32 i = 0; i < dst_timing->num_up_down; i++) {
                    reg::Write(LaScaleRegisters[i], dst_timing->la_scale_regs_arr[i]);
                }

                /* Request a timing update. */
                TimingUpdate(fbio_cfg7);
            }

            /* Step 26:
             *   Restore ZCAL registers.
             */
            if (dram_type == DRAM_TYPE_LPDDR4) {
                SetShadowBypass(ACTIVE);
                reg::Write(EMC + EMC_ZCAL_WAIT_CNT, dst_timing->burst_regs.emc_zcal_wait_cnt);
                reg::Write(EMC + EMC_ZCAL_INTERVAL, dst_timing->burst_regs.emc_zcal_interval);
                SetShadowBypass(ASSEMBLY);
            }

            if ((dram_type != DRAM_TYPE_LPDDR4)
                && opt_zcal_en_cc
                && !opt_short_zcal
                && opt_cc_short_zcal)
            {
                util::WaitMicroSeconds(2);

                SetShadowBypass(ACTIVE);
                if (dram_type == DRAM_TYPE_LPDDR2) {
                    reg::Write(EMC + EMC_MRS_WAIT_CNT, dst_timing->burst_regs.emc_mrs_wait_cnt);
                } else if (dram_type == DRAM_TYPE_DDR4) {
                    reg::Write(EMC + EMC_ZCAL_WAIT_CNT, dst_timing->burst_regs.emc_zcal_wait_cnt);
                }
                SetShadowBypass(ASSEMBLY);
            }

            /* Step 27:
             *   Restore EMC_CFG, FDPD registers.
             */
            SetShadowBypass(ACTIVE);
            reg::Write(EMC + EMC_CFG, dst_timing->burst_regs.emc_cfg);
            SetShadowBypass(ASSEMBLY);
            reg::Write(EMC + EMC_FDPD_CTRL_CMD_NO_RAMP, dst_timing->emc_fdpd_ctrl_cmd_no_ramp);
            reg::Write(EMC + EMC_SEL_DPD_CTRL, dst_timing->emc_sel_dpd_ctrl);

            /* Step 28:
             *   Training recover.
             */
            if (training_enabled && (dram_type == DRAM_TYPE_LPDDR4)) {
                SetShadowBypass(ACTIVE);
                reg::Write(EMC + EMC_CFG, dst_timing->burst_regs.emc_cfg);
                reg::Write(EMC + EMC_SEL_DPD_CTRL, dst_timing->emc_sel_dpd_ctrl);
                reg::Write(EMC + EMC_ZCAL_WAIT_CNT, src_timing->burst_regs.emc_zcal_wait_cnt);
                reg::Write(EMC + EMC_ZCAL_INTERVAL, src_timing->burst_regs.emc_zcal_interval);
                reg::Write(EMC + EMC_AUTO_CAL_CONFIG2, src_timing->emc_auto_cal_config2);
                reg::Write(EMC + EMC_AUTO_CAL_CONFIG3, src_timing->emc_auto_cal_config3);
                reg::Write(EMC + EMC_AUTO_CAL_CONFIG4, src_timing->emc_auto_cal_config4);
                reg::Write(EMC + EMC_AUTO_CAL_CONFIG5, src_timing->emc_auto_cal_config5);
                reg::Write(EMC + EMC_AUTO_CAL_CONFIG6, src_timing->emc_auto_cal_config6);
                reg::Write(EMC + EMC_AUTO_CAL_CONFIG7, src_timing->emc_auto_cal_config7);
                reg::Write(EMC + EMC_AUTO_CAL_CONFIG8, src_timing->emc_auto_cal_config8);
                SetShadowBypass(ASSEMBLY);
                reg::Write(EMC + EMC_TR_DVFS, dst_timing->burst_regs.emc_tr_dvfs & ~(1 << 0));
            }

            SetShadowBypass(ACTIVE);
            reg::Write(EMC + EMC_PMACRO_AUTOCAL_CFG_COMMON, dst_timing->burst_regs.emc_pmacro_autocal_cfg_common);
            SetShadowBypass(ASSEMBLY);

            /* Step 29:
             *   Power fix WAR.
             */
            reg::Write(EMC + EMC_PMACRO_CFG_PM_GLOBAL_0, 0xFF0000);
            reg::Write(EMC + EMC_PMACRO_TRAINING_CTRL_0, 0x8);
            reg::Write(EMC + EMC_PMACRO_TRAINING_CTRL_1, 0x8);
            reg::Write(EMC + EMC_PMACRO_CFG_PM_GLOBAL_0, 0);

            /* Step 30:
             *   Re-enable autocal.
             */
            if (training_enabled) {
                emc_auto_cal_config = src_timing->emc_auto_cal_config;

                /* Restore FSP to account for switch back. Only needed in training. */
                g_fsp_for_next_freq = !g_fsp_for_next_freq;
            } else {
                emc_auto_cal_config = dst_timing->emc_auto_cal_config;

                if (dst_timing->burst_regs.emc_cfg_dig_dll & 0x1) {
                    DllEnableStall(fbio_cfg7);
                }
            }

            reg::Write(EMC + EMC_AUTO_CAL_CONFIG, emc_auto_cal_config);
        }

        void CleanupActiveShadowCopy(EmcDvfsTimingTable *src_timing, EmcDvfsTimingTable *dst_timing) {
            const int dram_type = reg::GetValue(EMC + EMC_FBIO_CFG5, EMC_REG_BITS_MASK(FBIO_CFG5_DRAM_TYPE));
            const u32 fbio_cfg7 = reg::Read(EMC + EMC_FBIO_CFG7);

            /* Change CFG_SWAP to ASSEMBLY_ONLY */
            uint32_t emc_dbg = reg::Read(EMC + EMC_DBG);
            emc_dbg = ((emc_dbg & 0xF3FFFFFF) | 0x8000000);
            reg::Write(EMC + EMC_DBG, emc_dbg);

            /* Change UPDATE_AUTO_CAL_IN_UPDATE to ALWAYS */
            uint32_t emc_cfg_update = reg::Read(EMC + EMC_CFG_UPDATE);
            emc_cfg_update = ((emc_cfg_update & 0xFFFFFFF9) | 0x04);
            reg::Write(EMC + EMC_CFG_UPDATE, emc_cfg_update);

            /* Request a timing update event */
            TimingUpdate(fbio_cfg7);

            /* Change UPDATE_AUTO_CAL_IN_UPDATE to NEVER */
            emc_cfg_update = reg::Read(EMC + EMC_CFG_UPDATE);
            emc_cfg_update &= 0xFFFFFFF9;
            reg::Write(EMC + EMC_CFG_UPDATE, emc_cfg_update);

            /* Change CFG_SWAP to ACTIVE_ONLY */
            emc_dbg = reg::Read(EMC + EMC_DBG);
            emc_dbg &= 0xF3FFFFFF;
            reg::Write(EMC + EMC_DBG, emc_dbg);

            /* Disable DLL and change CFG_DLL_MODE to RUN_PERIODIC */
            uint32_t emc_cfg_dig_dll = reg::Read(EMC + EMC_CFG_DIG_DLL);
            emc_cfg_dig_dll = ((emc_cfg_dig_dll & 0xFFFFFF3E) | 0x80);
            reg::Write(EMC + EMC_CFG_DIG_DLL, emc_cfg_dig_dll);

            /* Request a timing update event */
            TimingUpdate(fbio_cfg7);

            /* Disable or enable DLL */
            emc_cfg_dig_dll = reg::Read(EMC + EMC_CFG_DIG_DLL);
            if (dst_timing->burst_regs.emc_cfg_dig_dll == 0x01) {
                emc_cfg_dig_dll |= 0x01;
            } else {
                emc_cfg_dig_dll &= 0xFFFFFFFE;
            }

            /* Change CFG_DLL_MODE to RUN_PERIODIC */
            emc_cfg_dig_dll = ((emc_cfg_dig_dll & 0xFFFFFF3F) | 0x80);
            reg::Write(EMC + EMC_CFG_DIG_DLL, emc_cfg_dig_dll);

            /* Request a timing update event */
            TimingUpdate(fbio_cfg7);

            /* Wait for DLL_LOCK to be set */
            uint32_t emc_dig_dll_status = 0;
            do {
                emc_dig_dll_status = reg::Read(EMC + EMC_DIG_DLL_STATUS);
            } while (!(emc_dig_dll_status & (1 << 15)));

            /* Check if DRAM is LPDDR4 */
            if (dram_type == DRAM_TYPE_LPDDR4) {
                reg::Write(EMC + EMC_RP, src_timing->burst_regs.emc_rp);
                reg::Write(EMC + EMC_R2P, src_timing->burst_regs.emc_r2p);
                reg::Write(EMC + EMC_W2P, src_timing->burst_regs.emc_w2p);
                reg::Write(EMC + EMC_TRPAB, src_timing->burst_regs.emc_trpab);
            }

            /* Request a timing update event */
            TimingUpdate(fbio_cfg7);
        }

        void TrainFreq(EmcDvfsTimingTable *src_timing, EmcDvfsTimingTable *dst_timing, u32 next_clk_src) {
            /* Get dram dev num. */
            const u32 dram_dev_num = (reg::Read(MC + MC_EMEM_ADR_CFG) & 1) + 1;

            /* Write RAM patterns, if first training. */
            if (!g_did_first_training) {
                const auto * const pattern = GetEmcRamTrainingPattern();
                for (u32 i = 0; i < 0x100; ++i) {
                    reg::Write(EMC + EMC_TRAINING_PATRAM_DQ,   pattern[dst_timing->training_pattern].dq[i]);
                    reg::Write(EMC + EMC_TRAINING_PATRAM_DMI,  pattern[dst_timing->training_pattern].dmi[i]);
                    reg::Write(EMC + EMC_TRAINING_PATRAM_CTRL, 0x80000000 | i);
                }

                g_did_first_training = true;
            }

            /* Do training, if we need to. */
            const u32 needed_training = dst_timing->needs_training;
            if (needed_training && !dst_timing->trained) {
                /* Determine what training to do. */
                u32 training_params[8];
                u32 num_params = 0;

                if (needed_training & (CA_TRAINING | CA_VREF_TRAINING)) {
                    training_params[num_params++] = (needed_training & (CA_TRAINING | CA_VREF_TRAINING | BIT_LEVEL_TRAINING));
                }
                if (dram_dev_num == TWO_RANK) {
                    if (needed_training & (CA_TRAINING | CA_VREF_TRAINING)) {
                        training_params[num_params++] = (needed_training & (CA_TRAINING | CA_VREF_TRAINING | TRAIN_SECOND_RANK | BIT_LEVEL_TRAINING));
                    }
                    if (needed_training & (QUSE_TRAINING | QUSE_VREF_TRAINING)) {
                        training_params[num_params++] = (needed_training & (QUSE_TRAINING | QUSE_VREF_TRAINING | BIT_LEVEL_TRAINING));
                        training_params[num_params++] = (needed_training & (QUSE_TRAINING | BIT_LEVEL_TRAINING));
                    }
                } else {
                    if (needed_training & (QUSE_TRAINING | QUSE_VREF_TRAINING)) {
                        training_params[num_params++] = (needed_training & (QUSE_TRAINING | QUSE_VREF_TRAINING | BIT_LEVEL_TRAINING));
                    }
                }
                if (needed_training & (WRITE_TRAINING | WRITE_VREF_TRAINING | READ_TRAINING | READ_VREF_TRAINING)) {
                    training_params[num_params++] = (needed_training & (WRITE_TRAINING | WRITE_VREF_TRAINING | READ_TRAINING | READ_VREF_TRAINING | BIT_LEVEL_TRAINING));
                }

                /* Apply all training. */
                for (u32 i = 0; i < num_params; ++i) {
                    FreqChange(src_timing, dst_timing, training_params[i], next_clk_src);
                    CleanupActiveShadowCopy(src_timing, dst_timing);
                }

                /* Set tables as trained. */
                dst_timing->trained = 1;
            }
        }

        constexpr inline const u16 PeriodicCompensationRegisters[] = {
            EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_0,
            EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_1,
            EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_2,
            EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_3,
            EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_0,
            EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_1,
            EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_2,
            EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_3,
            EMC_DATA_BRLSHFT_0,
            EMC_DATA_BRLSHFT_1
        };

        void PeriodicCompensationRoutine(EmcDvfsTimingTable *timing) {
            if (timing->periodic_training) {
                const int dram_dev_num = (reg::Read(MC + MC_EMEM_ADR_CFG) & 1) + 1;
                const u32 fbio_cfg7 = timing->burst_regs.emc_fbio_cfg7;

                uint32_t emc_cfg_o = reg::Read(EMC + EMC_CFG);
                uint32_t emc_cfg_dig_dll_o = reg::Read(EMC + EMC_CFG_DIG_DLL);
                uint32_t emc_cfg_update_o  = reg::Read(EMC + EMC_CFG_UPDATE);
                /*
                 * 1. Power optimizations should be off.
                 */
                reg::Write(EMC + EMC_CFG_DIG_DLL, emc_cfg_dig_dll_o & 0xFFFFFFFE);
                reg::Write(EMC + EMC_CFG_UPDATE, (emc_cfg_update_o & 0xFFFFF9FF) | 0x400);
                reg::Write(EMC + EMC_CFG, emc_cfg_o & 0x0FFFFFFF);

                /* Do timing update. */
                TimingUpdate(fbio_cfg7);

                if (dram_dev_num == TWO_RANK) {
                    WaitForUpdate(EMC_EMC_STATUS, 0x30, false, fbio_cfg7);
                } else {
                    WaitForUpdate(EMC_EMC_STATUS, 0x10, false, fbio_cfg7);
                }

                WaitForUpdate(EMC_EMC_STATUS, 0x300, false, fbio_cfg7);

                WaitForUpdate(EMC_EMC_STATUS, 0x01, false, fbio_cfg7);

                /*
                 * 2. osc kick off - this assumes training and dvfs have set
                 *    correct MR23.
                 */
                StartPeriodicCompensation();

                /*
                 * 3. Let dram capture its clock tree delays.
                 */
                util::WaitMicroSeconds(2 + ((ActualOscClocks(timing->run_clocks) * 1000) / timing->rate_khz));

                /*
                 * 4. Check delta wrt previous values (save value if margin
                 *    exceeds what is set in table).
                 */
                uint32_t del = UpdateClockTreeDelay(timing, timing, dram_dev_num, fbio_cfg7, PERIODIC_TRAINING_UPDATE);

                /*
                 * 5. Apply compensation w.r.t. trained values (if clock tree
                 *    has drifted more than the set margin).
                 */
                if (timing->tree_margin < ((del * 128 * (timing->rate_khz / 1000)) / 1000000)) {
                    for (u32 i = 0; i < util::size(PeriodicCompensationRegisters); ++i) {
                        reg::Write(EMC + PeriodicCompensationRegisters[i], ApplyPeriodicCompensationTrimmer(timing, PeriodicCompensationRegisters[i]));
                    }
                }

                /* Restore register values. */
                reg::Write(EMC + EMC_CFG, emc_cfg_o);
                reg::Write(EMC + EMC_CFG_DIG_DLL, emc_cfg_dig_dll_o);
                reg::Write(EMC + EMC_TIMING_CONTROL, 1);
                reg::Write(EMC + EMC_CFG_UPDATE, emc_cfg_update_o);
            }
        }

        void Dvfs(EmcDvfsTimingTable *dst_timing, EmcDvfsTimingTable *src_timing, bool train) {
            /* Get the old 2x clock source. */
            const u32 prev_2x_clk_src = reg::GetValue(CLKRST + CLK_RST_CONTROLLER_CLK_SOURCE_EMC, CLK_RST_REG_BITS_MASK(CLK_SOURCE_EMC_EMC_2X_CLK_SRC));

            /* Set g_next_pll. */
            g_next_pll = prev_2x_clk_src == PLLMB_UD || prev_2x_clk_src == PLLMB_OUT0;

            /* Reprogram pll. */
            u32 next_clk_src;
            if (PllReprogram(dst_timing->rate_khz, dst_timing->clk_src_emc, src_timing->rate_khz, src_timing->clk_src_emc)) {
                if (prev_2x_clk_src == PLLMB_UD || prev_2x_clk_src == PLLMB_OUT0) {
                    g_next_pll = 0;
                } else if (prev_2x_clk_src == PLLM_UD || prev_2x_clk_src == PLLM_OUT0) {
                    g_next_pll = !g_next_pll;
                }

                next_clk_src = ProgramPllm(dst_timing->rate_khz, dst_timing->clk_src_emc, g_next_pll);
            } else {
                next_clk_src = dst_timing->clk_src_emc;

                const u32 next_2x_clk_src = reg::GetField(next_clk_src, CLK_RST_REG_BITS_MASK(CLK_SOURCE_EMC_EMC_2X_CLK_SRC));
                if (next_2x_clk_src == PLLM_UD || next_2x_clk_src == PLLMB_UD) {
                    if (g_next_pll) {
                        reg::SetField(next_clk_src, CLK_RST_REG_BITS_VALUE(CLK_SOURCE_EMC_EMC_2X_CLK_SRC, PLLMB_UD));
                    }
                } else if (next_2x_clk_src == PLLM_OUT0 || next_2x_clk_src == PLLMB_OUT0) {
                    if (g_next_pll) {
                        reg::SetField(next_clk_src, CLK_RST_REG_BITS_VALUE(CLK_SOURCE_EMC_EMC_2X_CLK_SRC, PLLMB_OUT0));
                    }
                }
            }

            if (train) {
                TrainFreq(src_timing, dst_timing, next_clk_src);
                if (PllReprogram(dst_timing->rate_khz, dst_timing->clk_src_emc, src_timing->rate_khz, src_timing->clk_src_emc)) {
                    g_next_pll = !g_next_pll;
                }
            } else {
                FreqChange(src_timing, dst_timing, 0, next_clk_src);
                PeriodicCompensationRoutine(dst_timing);
            }
        }

    }

    void DoMemoryTrainingErista(int index, void *mtc_tables_buffer) {
        /* Get timing tables. */
        auto *timing_tables = GetEmcDvfsTimingTables(index, mtc_tables_buffer);
        auto *timing_204    = timing_tables + 0;
        auto *timing_800    = timing_tables + 1;
        auto *timing_1600   = timing_tables + 2;

        /* Check timing tables. */
        if (timing_204->rate_khz != 204000 || timing_1600->rate_khz != 1600000) {
            ShowFatalError("EmcDvfsTimingTables seem corrupted %" PRIu32 " %" PRIu32  " %" PRIu32 "?\n", timing_204->rate_khz, timing_800->rate_khz, timing_1600->rate_khz);
        }

        /* Check that we should do training. */
        if (timing_204->clk_src_emc != reg::Read(CLKRST + CLK_RST_CONTROLLER_CLK_SOURCE_EMC)) {
            /* Our clock source isn't what's expected, so presumably training has already been done? */
            /* Either way, the safe bet is to skip it. */
            return;
        }

        /* Train 800MHz. */
        Dvfs(timing_800, timing_204, true);

        /* Train 1600MHz. */
        Dvfs(timing_1600, timing_204, true);

        /* Switch to 800MHz. */
        Dvfs(timing_800, timing_204, false);

        /* Switch to 1600MHz. */
        Dvfs(timing_1600, timing_800, false);

        /* Wait 100ms. */
        util::WaitMicroSeconds(100000);

        /* Do Periodic compensation */
        PeriodicCompensationRoutine(timing_1600);
    }

}
