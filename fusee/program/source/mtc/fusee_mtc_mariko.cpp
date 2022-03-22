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
#include "../fusee_uncompress.hpp"
#include "fusee_mtc.hpp"
#include "fusee_mtc_timing_table_mariko.hpp"

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

        #include "fusee_mtc_tables_mariko.inc"
        #include "fusee_mtc_ram_training_pattern.inc"

        #define DECLARE_REGISTER_HANDLER(BASE, REG, NAME) BASE + REG,

        constexpr inline const u32 BurstRegisters[] = {
            FOREACH_BURST_REG(DECLARE_REGISTER_HANDLER)
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

        using EmcDvfsTimingTable = mariko::EmcDvfsTimingTable;

        EmcDvfsTimingTable *GetEmcDvfsTimingTables(int index, void *mtc_tables_buffer) {
            /* Get the compressed table. */
            const u8 *cmp_table;
            size_t cmp_table_size;
            switch (index) {
                #define HANDLE_CASE(N, TABLE)           \
                    case N:                             \
                        cmp_table = TABLE;              \
                        cmp_table_size = sizeof(TABLE); \
                        break;
                HANDLE_CASE(0x00, T210b01SdevEmcDvfsTableS4gb01)
                HANDLE_CASE(0x05, T210b01SdevEmcDvfsTableS4gb03)
                HANDLE_CASE(0x06, T210b01SdevEmcDvfsTableS8gb03)
                HANDLE_CASE(0x07, T210b01SdevEmcDvfsTableH4gb03)
                HANDLE_CASE(0x08, T210b01SdevEmcDvfsTableM4gb03)
                HANDLE_CASE(0x09, T210b01SdevEmcDvfsTableS4gbY01)
                HANDLE_CASE(0x0A, T210b01SdevEmcDvfsTableS1y4gbY01)
                HANDLE_CASE(0x0B, T210b01SdevEmcDvfsTableS1y8gbY01)
                HANDLE_CASE(0x0C, T210b01SdevEmcDvfsTableS1y4gbX03)
                HANDLE_CASE(0x0D, T210b01SdevEmcDvfsTableS1y8gbX03)
                HANDLE_CASE(0x0E, T210b01SdevEmcDvfsTableS1y4gb01)
                HANDLE_CASE(0x0F, T210b01SdevEmcDvfsTableM1y4gb01)
                HANDLE_CASE(0x10, T210b01SdevEmcDvfsTableH1y4gb01)
                default:
                    ShowFatalError("Unknown EmcDvfsTimingTableIndex: %d\n", index);
            }

            /* Uncompress the table. */
            EmcDvfsTimingTable *out_tables = reinterpret_cast<EmcDvfsTimingTable *>(mtc_tables_buffer);
            Uncompress(out_tables, 2 * sizeof(EmcDvfsTimingTable), cmp_table, cmp_table_size);

            return out_tables;
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
            /* Get current divp value. */
            u32 pll_p;
            switch (reg::GetValue(CLKRST + CLK_RST_CONTROLLER_CLK_SOURCE_EMC, CLK_RST_REG_BITS_MASK(CLK_SOURCE_EMC_EMC_2X_CLK_SRC))) {
                case PLLM_UD:
                case PLLM_OUT0:
                    pll_p = reg::GetValue(CLKRST + CLK_RST_CONTROLLER_PLLM_BASE, CLK_RST_REG_BITS_MASK(PLLM_BASE_PLLM_DIVP_B01));
                    break;
                case PLLMB_UD:
                case PLLMB_OUT0:
                    pll_p = reg::GetValue(CLKRST + CLK_RST_CONTROLLER_PLLMB_BASE, CLK_RST_REG_BITS_MASK(PLLMB_BASE_PLLMB_DIVP_B01));
                    break;
                default:
                    pll_p = 0;
                    break;
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

        u32 ProgramPllm(u32 next_rate_khz, u32 next_clk_src, u32 ret_clk_src, bool is_pllmb, EmcDvfsTimingTable *timing) {
            u32 ret = ret_clk_src;

            const uint32_t base = ((timing->pllmb_divm & 0xFF) | ((timing->pllmb_divn & 0xFF) << 8) | ((timing->pllmb_divp & 1) << 20));
            if (is_pllmb) {
                reg::Write(CLKRST + CLK_RST_CONTROLLER_PLLMB_BASE, base);
                reg::Read(CLKRST + CLK_RST_CONTROLLER_PLLMB_BASE);

                reg::SetBits(CLKRST + CLK_RST_CONTROLLER_PLLMB_MISC1, 0x10000000);

                if (timing->pll_en_ssc & 1) {
                    reg::Write(CLKRST + CLK_RST_CONTROLLER_PLLMB_SS_CFG,   timing->pllmb_ss_cfg);
                    reg::Write(CLKRST + CLK_RST_CONTROLLER_PLLMB_SS_CTRL1, timing->pllmb_ss_ctrl1);
                    reg::Write(CLKRST + CLK_RST_CONTROLLER_PLLMB_SS_CTRL2, timing->pllmb_ss_ctrl2);
                } else {
                    reg::Write(CLKRST + CLK_RST_CONTROLLER_PLLMB_SS_CFG,   timing->pllmb_ss_cfg   & 0xBFFFFFFF);
                    reg::Write(CLKRST + CLK_RST_CONTROLLER_PLLMB_SS_CTRL2, timing->pllmb_ss_ctrl2 & 0x0000FFFF);
                }

                reg::SetBits(CLKRST + CLK_RST_CONTROLLER_PLLMB_BASE, 0x40000000);

                switch (reg::GetField(ret, CLK_RST_REG_BITS_MASK(CLK_SOURCE_EMC_EMC_2X_CLK_SRC))) {
                    case PLLM_OUT0:
                        reg::SetField(ret, CLK_RST_REG_BITS_VALUE(CLK_SOURCE_EMC_EMC_2X_CLK_SRC, PLLMB_OUT0));
                        break;
                    case PLLM_UD:
                        reg::SetField(ret, CLK_RST_REG_BITS_VALUE(CLK_SOURCE_EMC_EMC_2X_CLK_SRC, PLLMB_UD));
                        break;
                }

                while ((reg::Read(CLKRST + CLK_RST_CONTROLLER_PLLMB_BASE) & 0x8000000) == 0) { /* ... */ }

                return ret;
            } else {
                reg::Write(CLKRST + CLK_RST_CONTROLLER_PLLM_BASE, base);
                reg::Read(CLKRST + CLK_RST_CONTROLLER_PLLM_BASE);

                reg::SetBits(CLKRST + CLK_RST_CONTROLLER_PLLM_MISC2, 0x10);

                if (timing->pll_en_ssc & 1) {
                    reg::Write(CLKRST + CLK_RST_CONTROLLER_PLLM_SS_CFG,   timing->pllm_ss_cfg);
                    reg::Write(CLKRST + CLK_RST_CONTROLLER_PLLM_SS_CTRL1, timing->pllm_ss_ctrl1);
                    reg::Write(CLKRST + CLK_RST_CONTROLLER_PLLM_SS_CTRL2, timing->pllm_ss_ctrl2);
                } else {
                    reg::Write(CLKRST + CLK_RST_CONTROLLER_PLLM_SS_CFG,   timing->pllm_ss_cfg   & 0xBFFFFFFF);
                    reg::Write(CLKRST + CLK_RST_CONTROLLER_PLLM_SS_CTRL2, timing->pllm_ss_ctrl2 & 0x0000FFFF);
                }

                reg::SetBits(CLKRST + CLK_RST_CONTROLLER_PLLM_BASE, 0x40000000);

                switch (reg::GetField(ret, CLK_RST_REG_BITS_MASK(CLK_SOURCE_EMC_EMC_2X_CLK_SRC))) {
                    case PLLM_OUT0:
                        reg::SetField(ret, CLK_RST_REG_BITS_VALUE(CLK_SOURCE_EMC_EMC_2X_CLK_SRC, PLLM_OUT0));
                        break;
                    case PLLM_UD:
                        reg::SetField(ret, CLK_RST_REG_BITS_VALUE(CLK_SOURCE_EMC_EMC_2X_CLK_SRC, PLLM_UD));
                        break;
                }

                while ((reg::Read(CLKRST + CLK_RST_CONTROLLER_PLLM_BASE) & 0x8000000) == 0) { /* ... */ }

                return ret;
            }
        }

        u32 GetDllState(EmcDvfsTimingTable *timing) {
            return (!(timing->emc_emrs & 0x1)) ? DLL_ON : DLL_OFF;
        }

        int WaitForUpdate(u32 reg_offset, u32 mask, bool updated, u32 fbio_cfg7) {
            constexpr int StatusUpdateTimeout = 1000;

            int result = 0;

            if (reg::GetField(fbio_cfg7, EMC_REG_BITS_MASK(FBIO_CFG7_CH0_ENABLE)) == EMC_FBIO_CFG7_CH0_ENABLE_ENABLE) {
                bool success = false;
                for (int i = 0; i < StatusUpdateTimeout; ++i) {
                    if (((reg::Read(EMC0 + reg_offset) & mask) != 0) == updated) {
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

        u32 DivideUpFloat(u32 a, u32 b) {
            const float res = a / b;

            const u32 floor = static_cast<u32>(res);

            return floor + ((static_cast<float>(floor) + 0.01 < res) ? 1 : 0);
        }

        void StartPeriodicCompensation() {
            reg::Write(EMC + EMC_MPC, 0x4B);
            reg::Read(EMC + EMC_MPC);
        }

        u32 SetShadowBypass(u32 val, u32 emc_dbg) {
            reg::SetField(emc_dbg, EMC_REG_BITS_VALUE(DBG_WRITE_MUX, val));
            return emc_dbg;
        }

        constinit uint32_t g_periodic_timmer_compensation_intermediates[9 * 0x10] = {};

        uint32_t ApplyPeriodicCompensationTrimmer(EmcDvfsTimingTable *timing, uint32_t trim_reg) {
            /* Initialize variables. */
            uint32_t rate_mhz = timing->rate_khz / 1000;
            uint32_t adj[0x10] = { 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8 };

            int tree_delta[4] = {0};
            uint32_t tree_delta_taps[4] = {0};

            /* Generate the intermediate array. */
            #define SET_TRIM_INTERMEDIATE(_arr_, _emc_, _rank_, _byte_)                                                             \
            ({                                                                                                                      \
                const uint32_t shft = timing->trim_perch_regs.emc##  _emc_ ##_data_brlshft_## _rank_;                        \
                const uint32_t base = ((shft >> (3 * _byte_)) & 7) << 6;                                                            \
                const uint32_t val0 = timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank ## _rank_ ## _byte ## _byte_ ## _0; \
                const uint32_t val1 = timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank ## _rank_ ## _byte ## _byte_ ## _1; \
                const uint32_t val2 = timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank ## _rank_ ## _byte ## _byte_ ## _2; \
                _arr_[9 * (8 * _rank_ + _byte_) + 0] = base + ((val0 >>  0) & 0xFF);                                                \
                _arr_[9 * (8 * _rank_ + _byte_) + 1] = base + ((val0 >>  8) & 0xFF);                                                \
                _arr_[9 * (8 * _rank_ + _byte_) + 2] = base + ((val0 >> 16) & 0xFF);                                                \
                _arr_[9 * (8 * _rank_ + _byte_) + 3] = base + ((val0 >> 24) & 0xFF);                                                \
                _arr_[9 * (8 * _rank_ + _byte_) + 4] = base + ((val1 >>  0) & 0xFF);                                                \
                _arr_[9 * (8 * _rank_ + _byte_) + 5] = base + ((val1 >>  8) & 0xFF);                                                \
                _arr_[9 * (8 * _rank_ + _byte_) + 6] = base + ((val1 >> 16) & 0xFF);                                                \
                _arr_[9 * (8 * _rank_ + _byte_) + 7] = base + ((val1 >> 24) & 0xFF);                                                \
                _arr_[9 * (8 * _rank_ + _byte_) + 8] = base + ((val2 >>  0) & 0xFF);                                                \
            })
            {
                SET_TRIM_INTERMEDIATE(g_periodic_timmer_compensation_intermediates, 0, 0, 0);
                SET_TRIM_INTERMEDIATE(g_periodic_timmer_compensation_intermediates, 0, 0, 1);
                SET_TRIM_INTERMEDIATE(g_periodic_timmer_compensation_intermediates, 0, 0, 2);
                SET_TRIM_INTERMEDIATE(g_periodic_timmer_compensation_intermediates, 0, 0, 3);
                SET_TRIM_INTERMEDIATE(g_periodic_timmer_compensation_intermediates, 1, 0, 4);
                SET_TRIM_INTERMEDIATE(g_periodic_timmer_compensation_intermediates, 1, 0, 5);
                SET_TRIM_INTERMEDIATE(g_periodic_timmer_compensation_intermediates, 1, 0, 6);
                SET_TRIM_INTERMEDIATE(g_periodic_timmer_compensation_intermediates, 1, 0, 7);
                SET_TRIM_INTERMEDIATE(g_periodic_timmer_compensation_intermediates, 0, 1, 0);
                SET_TRIM_INTERMEDIATE(g_periodic_timmer_compensation_intermediates, 0, 1, 1);
                SET_TRIM_INTERMEDIATE(g_periodic_timmer_compensation_intermediates, 0, 1, 2);
                SET_TRIM_INTERMEDIATE(g_periodic_timmer_compensation_intermediates, 0, 1, 3);
                SET_TRIM_INTERMEDIATE(g_periodic_timmer_compensation_intermediates, 1, 1, 4);
                SET_TRIM_INTERMEDIATE(g_periodic_timmer_compensation_intermediates, 1, 1, 5);
                SET_TRIM_INTERMEDIATE(g_periodic_timmer_compensation_intermediates, 1, 1, 6);
                SET_TRIM_INTERMEDIATE(g_periodic_timmer_compensation_intermediates, 1, 1, 7);
            }
            #undef SET_TRIM_INTERMEDIATE

            switch (trim_reg) {
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE0_0:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE0_1:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE0_2:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE1_0:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE1_1:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE1_2:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE2_0:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE2_1:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE2_2:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE3_0:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE3_1:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE3_2:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE4_0:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE4_1:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE4_2:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE5_0:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE5_1:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE5_2:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE6_0:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE6_1:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE6_2:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE7_0:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE7_1:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE7_2:
                case EMC0_BASE + EMC_DATA_BRLSHFT_0:
                case EMC1_BASE + EMC_DATA_BRLSHFT_0:
                    {
                        tree_delta[0] = 128 * (timing->current_dram_clktree_c0d0u0 - timing->trained_dram_clktree_c0d0u0);
                        tree_delta[1] = 128 * (timing->current_dram_clktree_c0d0u1 - timing->trained_dram_clktree_c0d0u1);
                        tree_delta[2] = 128 * (timing->current_dram_clktree_c1d0u0 - timing->trained_dram_clktree_c1d0u0);
                        tree_delta[3] = 128 * (timing->current_dram_clktree_c1d0u1 - timing->trained_dram_clktree_c1d0u1);
                        tree_delta_taps[0] = (tree_delta[0] * (int)rate_mhz) / 1000000;
                        tree_delta_taps[1] = (tree_delta[1] * (int)rate_mhz) / 1000000;
                        tree_delta_taps[2] = (tree_delta[2] * (int)rate_mhz) / 1000000;
                        tree_delta_taps[3] = (tree_delta[3] * (int)rate_mhz) / 1000000;

                        for (int i = 0; i < 4; ++i) {
                            const uint32_t sum = (tree_delta_taps[i] <= timing->tree_margin) ? 0 : tree_delta_taps[i];

                            for (int j = 0; j < 18; ++j) {
                                const uint32_t v = (g_periodic_timmer_compensation_intermediates[18 * i + j] += sum);
                                if (v < (adj[2 * i + (j < 9)] << 6)) {
                                    adj[2 * i + (j < 9)] = v >> 6;
                                }
                            }
                            for (int j = 0; j < 18; ++j) {
                                g_periodic_timmer_compensation_intermediates[18 * i + j] -= (adj[2 * i + (j < 9)] << 6);
                            }
                        }
                    }
                    break;
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE0_0:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE0_1:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE0_2:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE1_0:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE1_1:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE1_2:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE2_0:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE2_1:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE2_2:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE3_0:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE3_1:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE3_2:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE4_0:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE4_1:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE4_2:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE5_0:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE5_1:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE5_2:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE6_0:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE6_1:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE6_2:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE7_0:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE7_1:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE7_2:
                case EMC0_BASE + EMC_DATA_BRLSHFT_1:
                case EMC1_BASE + EMC_DATA_BRLSHFT_1:
                    {
                        tree_delta[0] = 128 * (timing->current_dram_clktree_c0d1u0 - timing->trained_dram_clktree_c0d1u0);
                        tree_delta[1] = 128 * (timing->current_dram_clktree_c0d1u1 - timing->trained_dram_clktree_c0d1u1);
                        tree_delta[2] = 128 * (timing->current_dram_clktree_c1d1u0 - timing->trained_dram_clktree_c1d1u0);
                        tree_delta[3] = 128 * (timing->current_dram_clktree_c1d1u1 - timing->trained_dram_clktree_c1d1u1);
                        tree_delta_taps[0] = (tree_delta[0] * (int)rate_mhz) / 1000000;
                        tree_delta_taps[1] = (tree_delta[1] * (int)rate_mhz) / 1000000;
                        tree_delta_taps[2] = (tree_delta[2] * (int)rate_mhz) / 1000000;
                        tree_delta_taps[3] = (tree_delta[3] * (int)rate_mhz) / 1000000;

                        for (int i = 0; i < 4; ++i) {
                            const uint32_t sum = (tree_delta_taps[i] <= timing->tree_margin) ? 0 : tree_delta_taps[i];

                            for (int j = 0; j < 18; ++j) {
                                const uint32_t v = (g_periodic_timmer_compensation_intermediates[72 + 18 * i + j] += sum);
                                if (v < (adj[8 + 2 * i + (j < 9)] << 6)) {
                                    adj[8 + 2 * i + (j < 9)] = v >> 6;
                                }
                            }
                            for (int j = 0; j < 18; ++j) {
                                g_periodic_timmer_compensation_intermediates[72 + 18 * i + j] -= (adj[8 + 2 * i + (j < 9)] << 6);
                            }
                        }
                    }
                    break;
            }

            uint32_t result = 0;
            switch (trim_reg) {
                case EMC0_BASE + EMC_DATA_BRLSHFT_0:
                    result = ((adj[ 0] & 7) <<  0) | ((adj[ 1] & 7) <<  3) | ((adj[ 2] & 7) <<  6) | ((adj[ 3] & 7) <<  9);
                    break;
                case EMC1_BASE + EMC_DATA_BRLSHFT_0:
                    result = ((adj[ 4] & 7) << 12) | ((adj[ 5] & 7) << 15) | ((adj[ 6] & 7) << 18) | ((adj[ 7] & 7) << 21);
                    break;
                case EMC0_BASE + EMC_DATA_BRLSHFT_1:
                    result = ((adj[ 8] & 7) <<  0) | ((adj[ 9] & 7) <<  3) | ((adj[10] & 7) <<  6) | ((adj[11] & 7) <<  9);
                    break;
                case EMC1_BASE + EMC_DATA_BRLSHFT_1:
                    result = ((adj[12] & 7) << 12) | ((adj[13] & 7) << 15) | ((adj[14] & 7) << 18) | ((adj[15] & 7) << 21);
                    break;
                #define ADD_TRIM_CASE(_ARR_, _RANK_, _BYTE_)                                                                                                                                                                                                  \
                    case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK ## _RANK_ ## _BYTE ## _BYTE_ ##_0:                                                                                                                                                       \
                        result = ((_ARR_[9 * (8 * _RANK_ + _BYTE_) + 0] & 0xFF) << 0) | ((_ARR_[9 * (8 * _RANK_ + _BYTE_) + 1] & 0xFF) << 8) | ((_ARR_[9 * (8 * _RANK_ + _BYTE_) + 2] & 0xFF) << 16) | ((_ARR_[9 * (8 * _RANK_ + _BYTE_) + 3] & 0xFF) << 24); \
                        break;                                                                                                                                                                                                                                \
                    case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK ## _RANK_ ## _BYTE ## _BYTE_ ##_1:                                                                                                                                                       \
                        result = ((_ARR_[9 * (8 * _RANK_ + _BYTE_) + 4] & 0xFF) << 0) | ((_ARR_[9 * (8 * _RANK_ + _BYTE_) + 5] & 0xFF) << 8) | ((_ARR_[9 * (8 * _RANK_ + _BYTE_) + 6] & 0xFF) << 16) | ((_ARR_[9 * (8 * _RANK_ + _BYTE_) + 7] & 0xFF) << 24); \
                        break;                                                                                                                                                                                                                                \
                    case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK ## _RANK_ ## _BYTE ## _BYTE_ ##_2:                                                                                                                                                       \
                        result = ((_ARR_[9 * (8 * _RANK_ + _BYTE_) + 8] & 0xFF) << 0);                                                                                                                                                                        \
                        break;
                ADD_TRIM_CASE(g_periodic_timmer_compensation_intermediates, 0, 0);
                ADD_TRIM_CASE(g_periodic_timmer_compensation_intermediates, 0, 1);
                ADD_TRIM_CASE(g_periodic_timmer_compensation_intermediates, 0, 2);
                ADD_TRIM_CASE(g_periodic_timmer_compensation_intermediates, 0, 3);
                ADD_TRIM_CASE(g_periodic_timmer_compensation_intermediates, 0, 4);
                ADD_TRIM_CASE(g_periodic_timmer_compensation_intermediates, 0, 5);
                ADD_TRIM_CASE(g_periodic_timmer_compensation_intermediates, 0, 6);
                ADD_TRIM_CASE(g_periodic_timmer_compensation_intermediates, 0, 7);
                ADD_TRIM_CASE(g_periodic_timmer_compensation_intermediates, 1, 0);
                ADD_TRIM_CASE(g_periodic_timmer_compensation_intermediates, 1, 1);
                ADD_TRIM_CASE(g_periodic_timmer_compensation_intermediates, 1, 2);
                ADD_TRIM_CASE(g_periodic_timmer_compensation_intermediates, 1, 3);
                ADD_TRIM_CASE(g_periodic_timmer_compensation_intermediates, 1, 4);
                ADD_TRIM_CASE(g_periodic_timmer_compensation_intermediates, 1, 5);
                ADD_TRIM_CASE(g_periodic_timmer_compensation_intermediates, 1, 6);
                ADD_TRIM_CASE(g_periodic_timmer_compensation_intermediates, 1, 7);
                #undef ADD_TRIM_CASE
            }

            return result;
        }

        u32 UpdateClockTreeDelay(EmcDvfsTimingTable *src_timing, EmcDvfsTimingTable *dst_timing, u32 dram_dev_num, u32 mode, int type) {
            uint32_t mrr_req = 0, mrr_data = 0;
            uint32_t temp0_0 = 0, temp0_1 = 0, temp1_0 = 0, temp1_1 = 0;
            int tdel = 0, tmdel = 0, adel = 0;

            uint32_t current_timing_rate_mhz = src_timing->rate_khz / 1000;
            uint32_t next_timing_rate_mhz    = dst_timing->rate_khz / 1000;
            uint32_t fbio_cfg7               = dst_timing->emc_fbio_cfg7;

            const bool ch0_enable = reg::GetField(fbio_cfg7, EMC_REG_BITS_MASK(FBIO_CFG7_CH0_ENABLE)) == EMC_FBIO_CFG7_CH0_ENABLE_ENABLE;
            const bool ch1_enable = reg::GetField(fbio_cfg7, EMC_REG_BITS_MASK(FBIO_CFG7_CH1_ENABLE)) == EMC_FBIO_CFG7_CH1_ENABLE_ENABLE;

            bool dvfs_pt1                 = (type == DVFS_PT1);
            bool training_pt1             = (type == TRAINING_PT1);
            bool dvfs_update              = (type == DVFS_UPDATE);
            bool training_update          = (type == TRAINING_UPDATE);
            bool periodic_training_update = (type == PERIODIC_TRAINING_UPDATE);

            /* Dev0 MSB. */
            if (dvfs_pt1 || training_pt1 || periodic_training_update) {
                mrr_req = ((2 << 30) | (19 << 16));
                reg::Write(EMC + EMC_MRR, mrr_req);

                WaitForUpdate(EMC_EMC_STATUS, (1 << 20), true, fbio_cfg7);

                if (ch0_enable) {
                    mrr_data = (reg::Read(EMC0 + EMC_MRR) & 0xFFFF);

                    temp0_0 = ((mrr_data & 0xff) << 8);
                    temp0_1 = (mrr_data & 0xff00);
                } else {
                    temp0_0 = temp0_1 = 0;
                }
                if (ch1_enable) {
                    mrr_data = (reg::Read(EMC1 + EMC_MRR) & 0xFFFF);

                    temp1_0 = ((mrr_data & 0xff) << 8);
                    temp1_1 = (mrr_data & 0xff00);
                } else {
                    temp1_0 = temp1_1 = 0;
                }

                /* Dev0 LSB. */
                mrr_req = ((mrr_req & ~(0xFF << 16)) | (18 << 16));
                reg::Write(EMC + EMC_MRR, mrr_req);

                WaitForUpdate(EMC_EMC_STATUS, (1 << 20), true, fbio_cfg7);

                if (ch0_enable) {
                    mrr_data = (reg::Read(EMC0 + EMC_MRR) & 0xFFFF);

                    temp0_0 |= (mrr_data & 0xff);
                    temp0_1 |= (mrr_data & 0xff00) >> 8;
                }
                if (ch1_enable) {
                    mrr_data = (reg::Read(EMC1 + EMC_MRR) & 0xFFFF);

                    temp1_0 |= (mrr_data & 0xff);
                    temp1_1 |= (mrr_data & 0xff00) >> 8;
                }
            }

            #define CVAL(v) ((uint32_t)((1000 * ((1000 * ActualOscClocks(src_timing->run_clocks)) / current_timing_rate_mhz)) / (2 * v)))

            if (ch0_enable) {
                if (dvfs_pt1 || training_pt1)
                    __INCREMENT_PTFV(c0d0u0, CVAL(temp0_0));
                else if (dvfs_update)
                    __AVERAGE_PTFV(c0d0u0);
                else if (training_update)
                    __AVERAGE_WRITE_PTFV(c0d0u0);
                else if (periodic_training_update)
                    __WEIGHTED_UPDATE_PTFV(c0d0u0, CVAL(temp0_0));

                if (dvfs_update || training_update || periodic_training_update) {
                    tdel = (dst_timing->current_dram_clktree_c0d0u0 - __MOVAVG_AC(dst_timing, c0d0u0));
                    tmdel = (tdel < 0) ? ~tdel : tdel;
                    adel = tmdel;
                    if (mode == 1 || ((adel * 128 * next_timing_rate_mhz) / 1000000) > dst_timing->tree_margin)
                        dst_timing->current_dram_clktree_c0d0u0 = __MOVAVG_AC(dst_timing, c0d0u0);
                }

                if (dvfs_pt1 || training_pt1)
                    __INCREMENT_PTFV(c0d0u1, CVAL(temp0_1));
                else if (dvfs_update)
                    __AVERAGE_PTFV(c0d0u1);
                else if (training_update)
                    __AVERAGE_WRITE_PTFV(c0d0u1);
                else if (periodic_training_update)
                    __WEIGHTED_UPDATE_PTFV(c0d0u1, CVAL(temp0_1));

                if (dvfs_update || training_update || periodic_training_update) {
                    tdel = (dst_timing->current_dram_clktree_c0d0u1 - __MOVAVG_AC(dst_timing, c0d0u1));
                    tmdel = (tdel < 0) ? -1 * tdel : tdel;

                    if (tmdel > adel)
                        adel = tmdel;

                    if (mode == 1 || (tmdel * 128 * next_timing_rate_mhz / 1000000) > dst_timing->tree_margin)
                        dst_timing->current_dram_clktree_c0d0u1 = __MOVAVG_AC(dst_timing, c0d0u1);
                }
            } else {
                adel = 0;
            }

            if (ch1_enable) {
                if (dvfs_pt1 || training_pt1)
                    __INCREMENT_PTFV(c1d0u0, CVAL(temp1_0));
                else if (dvfs_update)
                    __AVERAGE_PTFV(c1d0u0);
                else if (training_update)
                    __AVERAGE_WRITE_PTFV(c1d0u0);
                else if (periodic_training_update)
                    __WEIGHTED_UPDATE_PTFV(c1d0u0, CVAL(temp1_0));

                if (dvfs_update || training_update || periodic_training_update) {
                    tdel = (dst_timing->current_dram_clktree_c1d0u0 - __MOVAVG_AC(dst_timing, c1d0u0));
                    tmdel = (tdel < 0) ? -1 * tdel : tdel;

                    if (tmdel > adel)
                        adel = tmdel;

                    if (mode == 1 || (tmdel * 128 * next_timing_rate_mhz / 1000000) > dst_timing->tree_margin)
                        dst_timing->current_dram_clktree_c1d0u0 = __MOVAVG_AC(dst_timing, c1d0u0);
                }

                if (dvfs_pt1 || training_pt1)
                    __INCREMENT_PTFV(c1d0u1, CVAL(temp1_1));
                else if (dvfs_update)
                    __AVERAGE_PTFV(c1d0u1);
                else if (training_update)
                    __AVERAGE_WRITE_PTFV(c1d0u1);
                else if (periodic_training_update)
                    __WEIGHTED_UPDATE_PTFV(c1d0u1, CVAL(temp1_1));

                if (dvfs_update || training_update || periodic_training_update) {
                    tdel = (dst_timing->current_dram_clktree_c1d0u1 - __MOVAVG_AC(dst_timing, c1d0u1));
                    tmdel = (tdel < 0) ? -1 * tdel : tdel;

                    if (tmdel > adel)
                        adel = tmdel;

                    if (mode == 1 || (tmdel * 128 * next_timing_rate_mhz / 1000000) > dst_timing->tree_margin)
                        dst_timing->current_dram_clktree_c1d0u1 = __MOVAVG_AC(dst_timing, c1d0u1);
                }
            }

            if (dram_dev_num == TWO_RANK) {
                /* Dev1 MSB. */
                if (dvfs_pt1 || training_pt1 || periodic_training_update) {
                    mrr_req = ((1 << 30) | (19 << 16));
                    reg::Write(EMC + EMC_MRR, mrr_req);

                    WaitForUpdate(EMC_EMC_STATUS, (1 << 20), true, fbio_cfg7);

                    if (ch0_enable) {
                        mrr_data = (reg::Read(EMC0 + EMC_MRR) & 0xFFFF);

                        temp0_0 = ((mrr_data & 0xff) << 8);
                        temp0_1 = (mrr_data & 0xff00);
                    }
                    if (ch1_enable) {
                        mrr_data = (reg::Read(EMC1 + EMC_MRR) & 0xFFFF);

                        temp1_0 = ((mrr_data & 0xff) << 8);
                        temp1_1 = (mrr_data & 0xff00);
                    }

                    /* Dev1 LSB. */
                    mrr_req = ((mrr_req & ~(0xFF << 16)) | (18 << 16));
                    reg::Write(EMC + EMC_MRR, mrr_req);

                    WaitForUpdate(EMC_EMC_STATUS, (1 << 20), true, fbio_cfg7);

                    if (ch0_enable) {
                        mrr_data = (reg::Read(EMC0 + EMC_MRR) & 0xFFFF);

                        temp0_0 |= ((mrr_data & 0xff) << 8);
                        temp0_1 |= (mrr_data & 0xff00);
                    }
                    if (ch1_enable) {
                        mrr_data = (reg::Read(EMC1 + EMC_MRR) & 0xFFFF);

                        temp1_0 |= ((mrr_data & 0xff) << 8);
                        temp1_1 |= (mrr_data & 0xff00);
                    }
                }

                if (ch0_enable) {
                    if (dvfs_pt1 || training_pt1)
                        __INCREMENT_PTFV(c0d1u0, CVAL(temp0_0));
                    else if (dvfs_update)
                        __AVERAGE_PTFV(c0d1u0);
                    else if (training_update)
                        __AVERAGE_WRITE_PTFV(c0d1u0);
                    else if (periodic_training_update)
                        __WEIGHTED_UPDATE_PTFV(c0d1u0, CVAL(temp0_0));

                    if (dvfs_update || training_update || periodic_training_update) {
                        tdel = (dst_timing->current_dram_clktree_c0d1u0 - __MOVAVG_AC(dst_timing, c0d1u0));
                        tmdel = (tdel < 0) ? -1 * tdel : tdel;

                        if (tmdel > adel)
                            adel = tmdel;

                        if (mode == 1 || (tmdel * 128 * next_timing_rate_mhz / 1000000) > dst_timing->tree_margin)
                            dst_timing->current_dram_clktree_c0d1u0 = __MOVAVG_AC(dst_timing, c0d1u0);
                    }

                    if (dvfs_pt1 || training_pt1)
                        __INCREMENT_PTFV(c0d1u1, CVAL(temp0_1));
                    else if (dvfs_update)
                        __AVERAGE_PTFV(c0d1u1);
                    else if (training_update)
                        __AVERAGE_WRITE_PTFV(c0d1u1);
                    else if (periodic_training_update)
                        __WEIGHTED_UPDATE_PTFV(c0d1u1, CVAL(temp0_1));

                    if (dvfs_update || training_update || periodic_training_update) {
                        tdel = (dst_timing->current_dram_clktree_c0d1u1 - __MOVAVG_AC(dst_timing, c0d1u1));
                        tmdel = (tdel < 0) ? -1 * tdel : tdel;

                        if (tmdel > adel)
                            adel = tmdel;

                        if (mode == 1 || (tmdel * 128 * next_timing_rate_mhz / 1000000) > dst_timing->tree_margin)
                            dst_timing->current_dram_clktree_c0d1u1 = __MOVAVG_AC(dst_timing, c0d1u1);
                    }
                }

                if (ch1_enable) {
                    if (dvfs_pt1 || training_pt1)
                        __INCREMENT_PTFV(c1d1u0, CVAL(temp1_0));
                    else if (dvfs_update)
                        __AVERAGE_PTFV(c1d1u0);
                    else if (training_update)
                        __AVERAGE_WRITE_PTFV(c1d1u0);
                    else if (periodic_training_update)
                        __WEIGHTED_UPDATE_PTFV(c1d1u0, CVAL(temp1_0));

                    if (dvfs_update || training_update || periodic_training_update) {
                        tdel = (dst_timing->current_dram_clktree_c1d1u0 - __MOVAVG_AC(dst_timing, c1d1u0));
                        tmdel = (tdel < 0) ? -1 * tdel : tdel;

                        if (tmdel > adel)
                            adel = tmdel;

                        if (mode == 1 || (tmdel * 128 * next_timing_rate_mhz / 1000000) > dst_timing->tree_margin)
                            dst_timing->current_dram_clktree_c1d1u0 = __MOVAVG_AC(dst_timing, c1d1u0);
                    }

                    if (dvfs_pt1 || training_pt1)
                        __INCREMENT_PTFV(c1d1u1, CVAL(temp1_1));
                    else if (dvfs_update)
                        __AVERAGE_PTFV(c1d1u1);
                    else if (training_update)
                        __AVERAGE_WRITE_PTFV(c1d1u1);
                    else if (periodic_training_update)
                        __WEIGHTED_UPDATE_PTFV(c1d1u1, CVAL(temp1_1));

                    if (dvfs_update || training_update || periodic_training_update) {
                        tdel = (dst_timing->current_dram_clktree_c1d1u1 - __MOVAVG_AC(dst_timing, c1d1u1));
                        tmdel = (tdel < 0) ? -1 * tdel : tdel;

                        if (tmdel > adel)
                            adel = tmdel;

                        if (mode == 1 || (tmdel * 128 * next_timing_rate_mhz / 1000000) > dst_timing->tree_margin)
                            dst_timing->current_dram_clktree_c1d1u1 = __MOVAVG_AC(dst_timing, c1d1u1);
                    }
                }
            }

            #undef CVAL

            if (mode == 1) {
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

        u32 PeriodicCompensationHandler(int type, u32 dram_dev_num, EmcDvfsTimingTable *src_timing, EmcDvfsTimingTable *dst_timing) {
            if (!dst_timing->periodic_training) {
                return 0;
            }

            uint32_t adel          = 0;
            uint32_t samples       = dst_timing->ptfv_dvfs_samples;
            uint32_t samples_write = dst_timing->ptfv_write_samples;
            uint32_t delay         = 2 + (1000 * ActualOscClocks(src_timing->run_clocks) / src_timing->rate_khz);

            if (type == DVFS_SEQUENCE) {
                if (src_timing->periodic_training && (dst_timing->ptfv_config_ctrl & 1)) {
                    /* If the previous frequency was using periodic calibration then we can reuse the previous frequencies EMA data. */
                    dst_timing->ptfv_dqsosc_movavg_c0d0u0 = src_timing->ptfv_dqsosc_movavg_c0d0u0 * samples;
                    dst_timing->ptfv_dqsosc_movavg_c0d0u1 = src_timing->ptfv_dqsosc_movavg_c0d0u1 * samples;
                    dst_timing->ptfv_dqsosc_movavg_c1d0u0 = src_timing->ptfv_dqsosc_movavg_c1d0u0 * samples;
                    dst_timing->ptfv_dqsosc_movavg_c1d0u1 = src_timing->ptfv_dqsosc_movavg_c1d0u1 * samples;
                    dst_timing->ptfv_dqsosc_movavg_c0d1u0 = src_timing->ptfv_dqsosc_movavg_c0d1u0 * samples;
                    dst_timing->ptfv_dqsosc_movavg_c0d1u1 = src_timing->ptfv_dqsosc_movavg_c0d1u1 * samples;
                    dst_timing->ptfv_dqsosc_movavg_c1d1u0 = src_timing->ptfv_dqsosc_movavg_c1d1u0 * samples;
                    dst_timing->ptfv_dqsosc_movavg_c1d1u1 = src_timing->ptfv_dqsosc_movavg_c1d1u1 * samples;
                } else {
                    /* Reset the EMA. */
                    dst_timing->ptfv_dqsosc_movavg_c0d0u0 = 0;
                    dst_timing->ptfv_dqsosc_movavg_c0d0u1 = 0;
                    dst_timing->ptfv_dqsosc_movavg_c0d1u0 = 0;
                    dst_timing->ptfv_dqsosc_movavg_c0d1u1 = 0;
                    dst_timing->ptfv_dqsosc_movavg_c1d0u0 = 0;
                    dst_timing->ptfv_dqsosc_movavg_c1d0u1 = 0;
                    dst_timing->ptfv_dqsosc_movavg_c1d1u0 = 0;
                    dst_timing->ptfv_dqsosc_movavg_c1d1u1 = 0;

                    for (uint32_t i = 0; i < samples; ++i) {
                        StartPeriodicCompensation();

                        util::WaitMicroSeconds(delay);

                        /* Generate next sample of data. */
                        adel = UpdateClockTreeDelay(src_timing, dst_timing, dram_dev_num, 0, DVFS_PT1);
                    }
                }

                adel = UpdateClockTreeDelay(src_timing, dst_timing, dram_dev_num, 0, DVFS_UPDATE);
            } else if (type == WRITE_TRAINING_SEQUENCE) {
                /* Reset the EMA. */
                dst_timing->ptfv_dqsosc_movavg_c0d0u0 = 0;
                dst_timing->ptfv_dqsosc_movavg_c0d0u1 = 0;
                dst_timing->ptfv_dqsosc_movavg_c0d1u0 = 0;
                dst_timing->ptfv_dqsosc_movavg_c0d1u1 = 0;
                dst_timing->ptfv_dqsosc_movavg_c1d0u0 = 0;
                dst_timing->ptfv_dqsosc_movavg_c1d0u1 = 0;
                dst_timing->ptfv_dqsosc_movavg_c1d1u0 = 0;
                dst_timing->ptfv_dqsosc_movavg_c1d1u1 = 0;

                for (uint32_t i = 0; i < samples_write; ++i) {
                    StartPeriodicCompensation();

                    util::WaitMicroSeconds(delay);

                    /* Generate next sample of data. */
                    adel = UpdateClockTreeDelay(src_timing, dst_timing, dram_dev_num, 1, TRAINING_PT1);
                }

                adel = UpdateClockTreeDelay(src_timing, dst_timing, dram_dev_num, 1, TRAINING_UPDATE);
            } else if (type == PERIODIC_TRAINING_SEQUENCE) {
                StartPeriodicCompensation();

                util::WaitMicroSeconds(delay);

                adel = UpdateClockTreeDelay(src_timing, dst_timing, dram_dev_num, 0, PERIODIC_TRAINING_UPDATE);
            }

            return adel;
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

            reg::ReadWrite(CLKRST + CLK_RST_CONTROLLER_CLK_OUT_ENB_X, CLK_RST_REG_BITS_ENUM_SEL(CLK_ENB_X_CLK_ENB_EMC_DLL, (dst_timing->clk_out_enb_x_0_clk_enb_emc_dll & 1), ENABLE, DISABLE));
            reg::Read(CLKRST + CLK_RST_CONTROLLER_CLK_OUT_ENB_X);
        }

        void DllPrelock(EmcDvfsTimingTable *dst_timing, EmcDvfsTimingTable *src_timing, bool training_enabled, u32 next_clk_src) {
            /* Update EMC_CFG_DIG_DLL */
            reg::Write(EMC + EMC_CFG_DIG_DLL, (reg::Read(EMC + EMC_CFG_DIG_DLL) & 0xFFFFFFE4) | 0x00000008);

            /* Request a timing update event */
            TimingUpdate(dst_timing->emc_fbio_cfg7);

            /* Update EMC_CFG_DIG_DLL */
            reg::Write(EMC + EMC_CFG_DIG_DLL, (reg::Read(EMC + EMC_CFG_DIG_DLL) & 0xFFFFF824) | 0x000003C8);

            /* Request a timing update event */
            TimingUpdate(dst_timing->emc_fbio_cfg7);

            /* Wait until CFG_DLL_EN is cleared. */
            WaitForUpdate(EMC_CFG_DIG_DLL, (1 << 0), false, dst_timing->emc_fbio_cfg7);

            /* Configure PMACRO_DLL_CFG */
            reg::Write(EMC + EMC_PMACRO_DLL_CFG_0, dst_timing->burst_regs.emc_pmacro_dll_cfg_0);
            reg::Read(EMC + EMC_PMACRO_DLL_CFG_1);

            reg::Write(EMC + EMC_PMACRO_DLL_CFG_1, (dst_timing->burst_regs.emc_pmacro_dll_cfg_1 & 0xFFFFDFFF) | (reg::Read(EMC + EMC_PMACRO_DLL_CFG_1) & 0x00002000));

            /* Request a timing update event */
            TimingUpdate(dst_timing->emc_fbio_cfg7);

            /* Change the dll clock source. */
            ChangeDllSrc(dst_timing, next_clk_src);

            /* Wait 2 us. */
            util::WaitMicroSeconds(2);

            /* Enable dll. */
            reg::SetBits(EMC + EMC_CFG_DIG_DLL, 0x1);

            /* Request a timing update event */
            TimingUpdate(dst_timing->emc_fbio_cfg7);

            /* Wait until CFG_DLL_EN is set. */
            WaitForUpdate(EMC_CFG_DIG_DLL, (1 << 0), true, dst_timing->emc_fbio_cfg7);

            /* Wait for DLL_LOCK to be set */
            WaitForUpdate(EMC_DIG_DLL_STATUS, (1 << 2), true, dst_timing->emc_fbio_cfg7);

            if (training_enabled) {
                /* Disable dll. */
                reg::SetBits(EMC + EMC_DBG, 0x2);
                reg::ClearBits(EMC + EMC_CFG_DIG_DLL, 0x1);
                reg::ClearBits(EMC + EMC_DBG, 0x2);

                /* Wait until CFG_DLL_EN is cleared. */
                WaitForUpdate(EMC_CFG_DIG_DLL, (1 << 0), false, dst_timing->emc_fbio_cfg7);
            }

            reg::Read(EMC + EMC_PMACRO_DIG_DLL_STATUS_0);
        }

        void DllDisable(u32 fbio_cfg7) {
            /* Disable dll. */
            reg::ClearBits(EMC + EMC_CFG_DIG_DLL, 0x1);

            /* Request a timing update event */
            TimingUpdate(fbio_cfg7);

            /* Wait until CFG_DLL_EN is cleared. */
            WaitForUpdate(EMC_CFG_DIG_DLL, (1 << 0), false, fbio_cfg7);
        }

        void PllDisable(u32 dst_clk_src) {
            switch (reg::GetField(dst_clk_src, CLK_RST_REG_BITS_MASK(CLK_SOURCE_EMC_EMC_2X_CLK_SRC))) {
                case PLLM_OUT0:
                case PLLM_UD:
                    reg::ClearBits(CLKRST + CLK_RST_CONTROLLER_PLLMB_BASE, 0x40000000);
                    break;
                case PLLMB_OUT0:
                case PLLMB_UD:
                    reg::ClearBits(CLKRST + CLK_RST_CONTROLLER_PLLM_BASE,  0x40000000);
                    break;
                default:
                    reg::ClearBits(CLKRST + CLK_RST_CONTROLLER_PLLMB_BASE, 0x40000000);
                    reg::ClearBits(CLKRST + CLK_RST_CONTROLLER_PLLM_BASE,  0x40000000);
                    break;
            }
        }

        void DvfsPowerRampDown(EmcDvfsTimingTable *src_timing, EmcDvfsTimingTable *dst_timing, bool flip_backward, u32 vtt_vdda_channel) {
            auto *from_table = (flip_backward ? dst_timing : src_timing);
            auto *to_table   = (flip_backward ? src_timing : dst_timing);

            uint32_t from_rate_khz = from_table->rate_khz;
            uint32_t from_period   = 1000000000 / from_rate_khz;

            uint32_t to_rate_khz   = to_table->rate_khz;
            uint32_t clk_div       = 1000 * dst_timing->src_clock_div;

            uint32_t delay = util::DivideUp(clk_div, from_period);

            if (from_rate_khz >= 407997 || to_rate_khz <= 407996) {
                if (from_rate_khz >= 407997 && to_rate_khz <= 407996) {
                    uint32_t pmacro_vttgen_ctrl_1 = reg::Read(EMC + EMC_PMACRO_VTTGEN_CTRL_1);

                    if (dst_timing->vtt_vdda_dual_channel) {
                        if (vtt_vdda_channel != 1) {
                            return;
                        }

                        CcfifoWrite(EMC_PMACRO_VTTGEN_CTRL_1, (pmacro_vttgen_ctrl_1 & 0xFFFF03FF) | ((to_table->vtt_vdda_ctrl_4 & 0x3F) << 10), delay);
                        CcfifoWrite(EMC_PMACRO_VTTGEN_CTRL_1, (pmacro_vttgen_ctrl_1 & 0xFFFF03FF) | ((to_table->vtt_vdda_ctrl_0 & 0x3F) << 10), delay * 2);
                    } else {
                        CcfifoWrite(EMC_PMACRO_VTTGEN_CTRL_1, (pmacro_vttgen_ctrl_1 & 0xFFFF03FF) | ((dst_timing->vtt_vdda_ctrl_0 & 0x3F) << 10), 0);
                    }
                }
            } else {
                uint32_t pmacro_vttgen_ctrl_1 = reg::Read(EMC + EMC_PMACRO_VTTGEN_CTRL_1);

                if (dst_timing->vtt_vdda_dual_channel) {
                    if (vtt_vdda_channel == 1) {
                        CcfifoWrite(EMC_PMACRO_VTTGEN_CTRL_1, (pmacro_vttgen_ctrl_1 & 0xFFFF03FF) | ((dst_timing->vtt_vdda_ctrl_3 & 0x3F) << 10), delay);
                        CcfifoWrite(EMC_PMACRO_VTTGEN_CTRL_1, (pmacro_vttgen_ctrl_1 & 0xFFFF03FF) | ((dst_timing->vtt_vdda_ctrl_0 & 0x3F) << 10), delay);
                    } else if (vtt_vdda_channel == 0) {
                        CcfifoWrite(EMC_PMACRO_VTTGEN_CTRL_1, (pmacro_vttgen_ctrl_1 & 0xFFFF03FF) | ((dst_timing->vtt_vdda_ctrl_1 & 0x3F) << 10), delay);
                        CcfifoWrite(EMC_PMACRO_VTTGEN_CTRL_1, (pmacro_vttgen_ctrl_1 & 0xFFFF03FF) | ((dst_timing->vtt_vdda_ctrl_2 & 0x3F) << 10), delay);
                    }
                } else {
                    CcfifoWrite(EMC_PMACRO_VTTGEN_CTRL_1, (pmacro_vttgen_ctrl_1 & 0xFFFF03FF) | ((dst_timing->vtt_vdda_ctrl_0 & 0x3F) << 10), 0);
                }
            }
        }

        u32 DvfsPowerRampUp(u32 dst_clock_period, bool flip_backward, EmcDvfsTimingTable *src_timing, EmcDvfsTimingTable *dst_timing, u32 training) {
            uint32_t misc_cfg_1 = flip_backward ? src_timing->misc_cfg_1 : dst_timing->misc_cfg_1;

            uint32_t emc_pmacro_cmd_pad_tx_ctrl, emc_pmacro_brick_ctrl_rfu1, emc_fbio_cfg5;
            if (flip_backward) {
                emc_pmacro_cmd_pad_tx_ctrl = src_timing->burst_regs.emc_pmacro_cmd_pad_tx_ctrl;
                emc_pmacro_brick_ctrl_rfu1 = src_timing->burst_regs.emc_pmacro_brick_ctrl_rfu1;
                emc_fbio_cfg5              = src_timing->burst_regs.emc_fbio_cfg5;
            } else if (training & (CA_TRAINING | CA_VREF_TRAINING)) {
                emc_pmacro_cmd_pad_tx_ctrl = dst_timing->shadow_regs_ca_train.emc_pmacro_cmd_pad_tx_ctrl;
                emc_pmacro_brick_ctrl_rfu1 = dst_timing->shadow_regs_ca_train.emc_pmacro_brick_ctrl_rfu1;
                emc_fbio_cfg5              = dst_timing->shadow_regs_ca_train.emc_fbio_cfg5;
            } else if (training & (WRITE_TRAINING | WRITE_VREF_TRAINING | READ_TRAINING | READ_VREF_TRAINING)) {
                emc_pmacro_cmd_pad_tx_ctrl = dst_timing->shadow_regs_rdwr_train.emc_pmacro_cmd_pad_tx_ctrl;
                emc_pmacro_brick_ctrl_rfu1 = dst_timing->shadow_regs_rdwr_train.emc_pmacro_brick_ctrl_rfu1;
                emc_fbio_cfg5              = dst_timing->shadow_regs_rdwr_train.emc_fbio_cfg5;
            } else {
                emc_pmacro_cmd_pad_tx_ctrl = dst_timing->burst_regs.emc_pmacro_cmd_pad_tx_ctrl;
                emc_pmacro_brick_ctrl_rfu1 = dst_timing->burst_regs.emc_pmacro_brick_ctrl_rfu1;
                emc_fbio_cfg5              = dst_timing->burst_regs.emc_fbio_cfg5;
            }

            bool misc_flag = (misc_cfg_1 & 3) == 3;
            uint32_t timescale = 100000 << ((misc_cfg_1 >> 2) & 7);
            uint32_t delay     = (timescale / dst_clock_period);

            if (dst_clock_period < 869 || misc_flag) {
                CcfifoWrite(EMC_PMACRO_BRICK_CTRL_RFU1, emc_pmacro_brick_ctrl_rfu1 & 0xFE40FE40, delay + 1);
                CcfifoWrite(EMC_PMACRO_BRICK_CTRL_RFU1, emc_pmacro_brick_ctrl_rfu1 & 0xFEEDFEED, delay + 1);
                CcfifoWrite(EMC_PMACRO_BRICK_CTRL_RFU1, emc_pmacro_brick_ctrl_rfu1 & 0xFFFFFFFF, delay + 1);

                CcfifoWrite(EMC_FBIO_CFG5, emc_fbio_cfg5 & 0xFFFFFEFF, delay + 10);

                CcfifoWrite(EMC_PMACRO_CMD_PAD_TX_CTRL, emc_pmacro_cmd_pad_tx_ctrl & 0xFBFFFFFF, 5);

                return timescale + 10 * dst_clock_period + 3 * timescale;
            } else if (dst_clock_period > 1665) {
                CcfifoWrite(EMC_PMACRO_BRICK_CTRL_RFU1, emc_pmacro_brick_ctrl_rfu1 | 0x00000600, 0);

                CcfifoWrite(EMC_FBIO_CFG5, emc_fbio_cfg5 & 0xFFFFFEFF, 12);

                CcfifoWrite(EMC_PMACRO_CMD_PAD_TX_CTRL, emc_pmacro_cmd_pad_tx_ctrl & 0xFBFFFFFF, 5);

                return 12 * dst_clock_period;
            } else {
                CcfifoWrite(EMC_PMACRO_BRICK_CTRL_RFU1, emc_pmacro_brick_ctrl_rfu1 | 0x06000600, delay + 1);

                CcfifoWrite(EMC_FBIO_CFG5, emc_fbio_cfg5 & 0xFFFFFEFF, delay + 10);

                CcfifoWrite(EMC_PMACRO_CMD_PAD_TX_CTRL, emc_pmacro_cmd_pad_tx_ctrl & 0xFBFFFFFF, 5);

                return timescale + 10 * dst_clock_period;
            }
        }

        void FreqChange(EmcDvfsTimingTable *src_timing, EmcDvfsTimingTable *dst_timing, u32 training, u32 dst_clk_src) {
            /* Extract training values */
            const bool train_ca           = (training & CA_TRAINING);
            const bool train_ca_vref      = (training & CA_VREF_TRAINING);
            //const bool train_quse         = (training & QUSE_TRAINING);
            //const bool train_quse_vref    = (training & QUSE_VREF_TRAINING);
            const bool train_wr           = (training & WRITE_TRAINING);
            const bool train_wr_vref      = (training & WRITE_VREF_TRAINING);
            const bool train_rd           = (training & READ_TRAINING);
            const bool train_rd_vref      = (training & READ_VREF_TRAINING);
            const bool train_second_rank  = (training & TRAIN_SECOND_RANK);
            const bool train_bit_level    = (training & BIT_LEVEL_TRAINING);

            /* Check if we should do training. */
            const bool training_enabled = (training & (CA_TRAINING | CA_VREF_TRAINING | WRITE_TRAINING | WRITE_VREF_TRAINING | READ_TRAINING | READ_VREF_TRAINING));

            uint32_t dst_emc_fbio_cfg7 = dst_timing->emc_fbio_cfg7;
            uint32_t dst_misc_cfg_0    = dst_timing->misc_cfg_0;
            uint32_t dst_misc_cfg_1    = dst_timing->misc_cfg_1;
            uint32_t dst_misc_cfg_2    = dst_timing->misc_cfg_2;
            uint32_t src_misc_cfg_0    = src_timing->misc_cfg_0;
            uint32_t src_misc_cfg_1    = src_timing->misc_cfg_1;
            uint32_t src_t_rp          = src_timing->dram_timings.t_rp;
            uint32_t src_t_rfc         = src_timing->dram_timings.t_rfc;
            uint32_t dst_t_pdex        = dst_timing->dram_timings.t_pdex;
            uint32_t dst_t_fc_lpddr4   = dst_timing->dram_timings.t_fc_lpddr4;

            const bool ch0_enable = reg::GetField(dst_emc_fbio_cfg7, EMC_REG_BITS_MASK(FBIO_CFG7_CH0_ENABLE)) == EMC_FBIO_CFG7_CH0_ENABLE_ENABLE;
            const bool ch1_enable = reg::GetField(dst_emc_fbio_cfg7, EMC_REG_BITS_MASK(FBIO_CFG7_CH1_ENABLE)) == EMC_FBIO_CFG7_CH1_ENABLE_ENABLE;

            g_fsp_for_next_freq = !g_fsp_for_next_freq;

            const int dram_type = reg::GetValue(EMC + EMC_FBIO_CFG5, EMC_REG_BITS_MASK(FBIO_CFG5_DRAM_TYPE));
            uint32_t src_emc_zcal_wait_cnt = src_timing->burst_regs.emc_zcal_wait_cnt;

            bool shared_zq_resistor = (src_emc_zcal_wait_cnt >> 31) & 1;

            bool opt_zcal_en_cc = (dst_timing->burst_regs.emc_zcal_interval && !src_timing->burst_regs.emc_zcal_interval) || (dram_type == DRAM_TYPE_LPDDR4);

            uint32_t dst_t_fc_lpddr4_hz = 1000 * dst_t_fc_lpddr4;

            bool is_lpddr2 = (dram_type == DRAM_TYPE_LPDDR2);
            bool is_lpddr3 = is_lpddr2 && ((dst_timing->burst_regs.emc_fbio_cfg5 >> 25) & 1);
            uint32_t opt_dll_mode = (dram_type == DRAM_TYPE_DDR4) ? GetDllState(dst_timing) : DLL_OFF;

            int dram_dev_num = ((reg::Read(MC + MC_EMEM_ADR_CFG) & 1) + 1);

            uint32_t tZQCAL_lpddr4              = dst_timing->tZQCAL_lpddr4;
            uint32_t zqcal_before_cc_cutoff     = dst_timing->zqcal_before_cc_cutoff;
            uint32_t opt_cc_short_zcal          = dst_timing->opt_cc_short_zcal;
            uint32_t opt_short_zcal             = dst_timing->opt_short_zcal;
            uint32_t opt_do_sw_qrst             = dst_timing->opt_do_sw_qrst;
            uint32_t save_restore_clkstop_pd    = dst_timing->save_restore_clkstop_pd;
            // uint32_t opt_E90                    = dst_timing->opt_E90;
            uint32_t cya_allow_ref_cc           = dst_timing->cya_allow_ref_cc;
            uint32_t ref_b4_sref_en             = dst_timing->ref_b4_sref_en;
            uint32_t cya_issue_pc_ref           = dst_timing->cya_issue_pc_ref;

            uint32_t src_rate_khz = src_timing->rate_khz;
            uint32_t dst_rate_khz = dst_timing->rate_khz;

            uint32_t src_clock_period = 1000000000 / src_rate_khz;
            uint32_t dst_clock_period = 1000000000 / dst_rate_khz;

            uint32_t emc_auto_cal_config = reg::Read(EMC + EMC_AUTO_CAL_CONFIG);

            uint32_t adj_dst_t_fc_lpddr4 = (dst_clock_period <= zqcal_before_cc_cutoff) ? dst_t_fc_lpddr4_hz : 0;

            uint32_t emc_dbg_o = reg::Read(EMC + EMC_DBG);
            uint32_t emc_pin_o = reg::Read(EMC + EMC_PIN);
            uint32_t emc_cfg_pipe_clk_o = reg::Read(EMC + EMC_CFG_PIPE_CLK);
            uint32_t emc_dbg = emc_dbg_o;

            uint32_t emc_cfg = dst_timing->burst_regs.emc_cfg;
            uint32_t emc_sel_dpd_ctrl = dst_timing->emc_sel_dpd_ctrl;

            uint32_t next_push, next_dq_e_ivref, next_dqs_e_ivref;

            /* Step 1:
             *   Pre DVFS SW sequence.
             */

            /* Step 1.1: Disable DLL. */
            uint32_t tmp = reg::Read(EMC + EMC_CFG_DIG_DLL);
            tmp &= ~(1 << 0);
            reg::Write(EMC + EMC_CFG_DIG_DLL, tmp);

            /* Calculate 14000 / dst_period. */
            uint32_t div_14000_by_dst_period = std::max<u32>(util::DivideUp(14000, dst_clock_period), 10);

            /* Request a timing update. */
            TimingUpdate(dst_emc_fbio_cfg7);

            /* Wait for DLL to be disabled. */
            WaitForUpdate(EMC_CFG_DIG_DLL, (1 << 0), false, dst_emc_fbio_cfg7);

            /* Step 1.2: Disable AUTOCAL. */
            emc_auto_cal_config = (dst_timing->emc_auto_cal_config & 0x7FFFF9FF) | 0x600;
            reg::Write(EMC + EMC_AUTO_CAL_CONFIG, emc_auto_cal_config);
            reg::Read(EMC + EMC_AUTO_CAL_CONFIG);

            /* Step 1.3: Disable other power features. */
            emc_dbg = SetShadowBypass(ACTIVE, emc_dbg_o);
            reg::Write(EMC + EMC_DBG, emc_dbg);
            reg::Write(EMC + EMC_CFG, emc_cfg & 0x0FFFFFFF);
            reg::Write(EMC + EMC_SEL_DPD_CTRL, emc_sel_dpd_ctrl & 0xFFFFFEC3);
            reg::Write(EMC + EMC_DBG, emc_dbg_o);

            /* Skip this if dvfs_with_training is set. */
            bool compensate_trimmer_applicable = false;
            uint32_t adel = 0;
            if (!training_enabled && dst_timing->periodic_training) {
                /* Wait for DRAM to get out of power down. */
                WaitForUpdate(EMC_EMC_STATUS, dram_dev_num == TWO_RANK ? 0x30 : 0x10, false, dst_emc_fbio_cfg7);

                /* Wait for DRAM to get out of self refresh. */
                WaitForUpdate(EMC_EMC_STATUS, 0x300, false, dst_emc_fbio_cfg7);

                if (dst_timing->periodic_training) {
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
                    adel = PeriodicCompensationHandler(DVFS_SEQUENCE, dram_dev_num, src_timing, dst_timing);

                    /* Check if we should use compensate trimmer. */
                    compensate_trimmer_applicable = dst_timing->periodic_training && ((adel * 128 * (dst_rate_khz / 1000)) / 1000000) > dst_timing->tree_margin;
                }
            }

            reg::Write(EMC + EMC_INTSTATUS, (1 << 4));

            emc_dbg = SetShadowBypass(ACTIVE, emc_dbg);
            reg::Write(EMC + EMC_DBG, emc_dbg);
            reg::Write(EMC + EMC_CFG, emc_cfg & 0x0FFFFFFF);
            reg::Write(EMC + EMC_SEL_DPD_CTRL, emc_sel_dpd_ctrl & 0xFFFFFEC3);
            reg::Write(EMC + EMC_CFG_PIPE_CLK, emc_cfg_pipe_clk_o | (1 << 0));
            reg::Write(EMC + EMC_FDPD_CTRL_CMD_NO_RAMP, dst_timing->emc_fdpd_ctrl_cmd_no_ramp & ~(1 << 0));

            /* Adjust pllm_misc1 as needed. */
            if (dst_timing->pllm_misc1_0_pllm_clamp_ph90) {
                reg::ClearBits(CLKRST + CLK_RST_CONTROLLER_PLLM_MISC1, 0x80000000);
            }

            /* Check if we need to turn on VREF generator. */
            if (((!(src_timing->burst_regs.emc_pmacro_data_pad_tx_ctrl &
                   (1 <<  0))) &&
                 ((dst_timing->burst_regs.emc_pmacro_data_pad_tx_ctrl &
                   (1 <<  0)))) ||
                ((!(src_timing->burst_regs.emc_pmacro_data_pad_tx_ctrl &
                   (1 << 10))) &&
                 ((dst_timing->burst_regs.emc_pmacro_data_pad_tx_ctrl &
                   (1 << 10)))))
            {
                uint32_t pad_tx_ctrl      = dst_timing->burst_regs.emc_pmacro_data_pad_tx_ctrl;
                uint32_t last_pad_tx_ctrl = src_timing->burst_regs.emc_pmacro_data_pad_tx_ctrl;

                next_dqs_e_ivref = pad_tx_ctrl & (1 << 10);
                next_dq_e_ivref = pad_tx_ctrl & (1 << 0);
                next_push = (last_pad_tx_ctrl & ~(1 << 0) & ~(1 << 10)) | next_dq_e_ivref | next_dqs_e_ivref;
                reg::Write(EMC + EMC_PMACRO_DATA_PAD_TX_CTRL, next_push);

                reg::Write(EMC + EMC_DBG, emc_dbg_o);
                util::WaitMicroSeconds(1);
            } else {
                reg::Write(EMC + EMC_DBG, emc_dbg_o);
            }

            /* Check if we need to fixup xm2comppadctrl */
            if ((dst_misc_cfg_1 & 0x20) == 0) {
                reg::Write(EMC + EMC_DBG, SetShadowBypass(ACTIVE, emc_dbg_o));

                uint32_t xm2comppadctrl = reg::Read(EMC + EMC_XM2COMPPADCTRL);

                reg::Write(EMC + EMC_XM2COMPPADCTRL, xm2comppadctrl | 0x08000000);
                util::WaitMicroSeconds(1);
                reg::Write(EMC + EMC_XM2COMPPADCTRL, xm2comppadctrl | 0x18000000);
                util::WaitMicroSeconds(1);
                reg::Write(EMC + EMC_XM2COMPPADCTRL, xm2comppadctrl | 0x38000000);
                util::WaitMicroSeconds(1);

                reg::Write(EMC + EMC_DBG, SetShadowBypass(ASSEMBLY, emc_dbg_o));
            }

            /* Step 2:
             *   Prelock the DLL.
             */

            if (dst_timing->burst_regs.emc_cfg_dig_dll & (1 << 0)) {
                reg::Write(EMC + EMC_DBG, SetShadowBypass(ACTIVE, emc_dbg_o));

                reg::ClearBits(EMC + EMC_PMACRO_DLL_CFG_1, 0x2000);
                reg::Read(EMC + EMC_PMACRO_DLL_CFG_1);

                reg::Write(EMC + EMC_DBG, SetShadowBypass(ASSEMBLY, emc_dbg_o));
                reg::Read(EMC + EMC_DBG);

                DllPrelock(dst_timing, src_timing, training_enabled, dst_clk_src);
            } else {
                DllDisable(dst_emc_fbio_cfg7);
            }

            /* Step 3:
             *   Prepare autocal for the clock change.
             */

            /* Disable AUTOCAL. */
            emc_auto_cal_config = (dst_timing->emc_auto_cal_config & 0x7FFFF9FF) | 0x600;
            reg::Write(EMC + EMC_AUTO_CAL_CONFIG, emc_auto_cal_config);

            reg::Write(EMC + EMC_DBG, SetShadowBypass(ACTIVE, emc_dbg_o));

            reg::Write(EMC + EMC_AUTO_CAL_CONFIG2, dst_timing->emc_auto_cal_config2);

            if (ch0_enable || ch1_enable) {
                reg::Write(EMC0 + EMC_AUTO_CAL_CONFIG3, dst_timing->emc_auto_cal_config3);
                reg::Write(EMC0 + EMC_AUTO_CAL_CONFIG4, dst_timing->emc_auto_cal_config4);
                reg::Write(EMC0 + EMC_AUTO_CAL_CONFIG5, dst_timing->emc_auto_cal_config5);
                reg::Write(EMC0 + EMC_AUTO_CAL_CONFIG6, dst_timing->emc_auto_cal_config6);
                reg::Write(EMC0 + EMC_AUTO_CAL_CONFIG7, dst_timing->emc_auto_cal_config7);
                reg::Write(EMC0 + EMC_AUTO_CAL_CONFIG8, dst_timing->emc_auto_cal_config8);
            }

            reg::Write(EMC + EMC_DBG, emc_dbg_o);

            emc_auto_cal_config = (dst_timing->emc_auto_cal_config & 0x7FFFF9FE) | 0x601;
            reg::Write(EMC + EMC_AUTO_CAL_CONFIG, emc_auto_cal_config);

            /* Step 4:
             *   Update EMC_CFG.
             */

            reg::ClearBits(EMC + EMC_CFG, 0x10000000);
            reg::Write(EMC + EMC_CFG_2, dst_timing->emc_cfg_2);

            /* Step 5:
             *   Prepare reference variables for ZQCAL regs.
             */
            //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: freq_change - step 5\n");
            //mdelay(500);

            uint32_t emc_zcal_interval = src_timing->burst_regs.emc_zcal_interval & 0xFF000000;
            uint32_t dst_emc_zcal_wait_cnt = dst_timing->burst_regs.emc_zcal_wait_cnt;

            uint32_t zq_wait_long, zq_wait_short;

            if (dram_type == DRAM_TYPE_LPDDR4) {
                zq_wait_long  = std::max<u32>(util::DivideUp(1000000, dst_clock_period), 1);
                zq_wait_short = std::max<u32>(util::DivideUp(30000, dst_clock_period), 8) + 1;
            } else if (is_lpddr2 || is_lpddr3) {
                zq_wait_long  = std::max<u32>(util::DivideUp(360000, dst_clock_period), dst_timing->min_mrs_wait) + 4;
                zq_wait_short = 0;
            } else if (dram_type == DRAM_TYPE_DDR4) {
                zq_wait_long  = std::max<u32>(util::DivideUp(320000, dst_clock_period), 256);
                zq_wait_short = 0;
            } else {
                zq_wait_long  = 0;
                zq_wait_short = 0;
            }

            /* Step 6:
             *   Training code.
             */

            {
                uint32_t pintemp = reg::Read(EMC + EMC_PIN);
                if ((train_ca || train_ca_vref) && (dram_dev_num == TWO_RANK)) {
                    reg::Write(EMC + EMC_PIN, pintemp | 0x7);
                }
            }

            /* Step 7:
             *   Program FSP reference registers and send MRWs to new FSPWR.
             */

            uint32_t mr13_flip_fspop, mr13_flip_fspwr;
            if (!g_fsp_for_next_freq) {
                mr13_flip_fspwr = (dst_timing->emc_mrw3 & 0xffffff3f) | 0x80;
                mr13_flip_fspop = (dst_timing->emc_mrw3 & 0xffffff3f) | 0x00;
            } else {
                mr13_flip_fspwr = (dst_timing->emc_mrw3 & 0xffffff3f) | 0x40;
                mr13_flip_fspop = (dst_timing->emc_mrw3 & 0xffffff3f) | 0xc0;
            }

            uint32_t mr13_catr_enable = mr13_flip_fspwr | 1;

            if (dram_dev_num == TWO_RANK) {
                if (train_ca || train_ca_vref) {
                    mr13_flip_fspop  = (mr13_flip_fspop & 0x3FFFFFFF) | (train_second_rank ? 0x80000000 : 0x40000000);
                }

                mr13_catr_enable = (mr13_catr_enable & 0x3FFFFFFF) | (train_second_rank ? 0x40000000 : 0x80000000);
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
            {
                uint32_t pmacro_vttgen_ctrl_1 = reg::Read(EMC + EMC_PMACRO_VTTGEN_CTRL_1);
                uint32_t xm2comppadctrl       = reg::Read(EMC + EMC_XM2COMPPADCTRL);
                for (u32 i = 0; i < dst_timing->num_burst; ++i) {
                    if (!BurstRegisters[i]) {
                        continue;
                    }

                    const u32 reg_addr = BurstRegisters[i];

                    uint32_t wval;
                    if (train_ca || train_ca_vref) {
                        wval = dst_timing->shadow_regs_ca_train_arr[i];
                    } else if (train_wr || train_wr_vref || train_rd || train_rd_vref) {
                        wval = dst_timing->shadow_regs_rdwr_train_arr[i];
                    } else {
                        wval = dst_timing->burst_regs_arr[i];
                    }

                    /* Adjust the value to write. */
                    switch (reg_addr) {
                        case EMC_BASE + EMC_CFG:
                            wval &= (dram_type == DRAM_TYPE_LPDDR4) ? 0x0FFFFFFF : 0xCFFFFFFF;
                            break;
                        case EMC_BASE + EMC_MRS_WAIT_CNT:
                            if (opt_zcal_en_cc && is_lpddr2 && (opt_cc_short_zcal == 0) && (opt_short_zcal != 0)) {
                                wval = (wval & 0xFFFFFC00) | (zq_wait_long & 0x3FF);
                            }
                            break;
                        case EMC_BASE + EMC_ZCAL_WAIT_CNT:
                            if ((opt_short_zcal != 0) && opt_zcal_en_cc && (opt_cc_short_zcal == 0) && (dram_type == DRAM_TYPE_DDR4)) {
                                wval = (wval & 0xFFFFF800) | (zq_wait_long & 0x7FF);
                            }
                            break;
                        case EMC_BASE + EMC_ZCAL_INTERVAL:
                            if (opt_zcal_en_cc) {
                                wval = 0;
                            }
                            break;
                        case EMC_BASE + EMC_PMACRO_BRICK_CTRL_RFU1:
                            wval &= 0xF800F800;
                            break;
                        case EMC_BASE + EMC_PMACRO_CMD_PAD_TX_CTRL:
                            wval |= 0x04000000;
                            break;
                        case EMC_BASE + EMC_PMACRO_AUTOCAL_CFG_COMMON:
                            wval |= 0x00010000;
                            break;
                        case EMC_BASE + EMC_TRAINING_CTRL:
                            if (train_second_rank) {
                                wval |= 0x4000;
                            }
                            break;
                        case EMC_BASE + EMC_REFRESH:
                        case EMC_BASE + EMC_TREFBW:
                            wval >>= 0;
                            break;
                        case EMC_BASE + EMC_XM2COMPPADCTRL:
                            if ((dst_misc_cfg_1 & 0x20) == 0) {
                                wval = (wval & 0x00FFFFFF) | (xm2comppadctrl & 0xFF000000);
                            }
                            break;
                        case EMC_BASE + EMC_DLL_CFG_1:
                            wval = (wval & 0xFFFFDFFF) | (reg::Read(EMC + EMC_PMACRO_DLL_CFG_1) & 0x00002000);
                            break;
                        case EMC_BASE + EMC_PMACRO_VTTGEN_CTRL_1:
                            wval = (wval & 0xFFFF03FF) | (pmacro_vttgen_ctrl_1 & 0xFC00);
                            break;
                        case EMC_BASE + EMC_MRW6:
                        case EMC_BASE + EMC_MRW7:
                        case EMC_BASE + EMC_MRW8:
                        case EMC_BASE + EMC_MRW9:
                        case EMC_BASE + EMC_MRW14:
                        case EMC_BASE + EMC_MRW15:
                        case EMC0_BASE + EMC_MRW10:
                        case EMC0_BASE + EMC_MRW11:
                        case EMC0_BASE + EMC_MRW12:
                        case EMC0_BASE + EMC_MRW13:
                        case EMC1_BASE + EMC_MRW10:
                        case EMC1_BASE + EMC_MRW11:
                        case EMC1_BASE + EMC_MRW12:
                        case EMC1_BASE + EMC_MRW13:
                            if (dram_type != DRAM_TYPE_LPDDR4) {
                                continue;
                            }
                            break;
                    }

                    /* Write the value. */
                    reg::Write(reg_addr, wval);
                }
            }

            if (dram_type == DRAM_TYPE_LPDDR4) {
                /* Use the current timing when training. */
                uint32_t mrw_req;
                if (training_enabled)
                    mrw_req = (23 << 16) | (src_timing->run_clocks & (0xFF << 0));
                else
                    mrw_req = (23 << 16) | (dst_timing->run_clocks & (0xFF << 0));

                reg::Write(EMC + EMC_MRW, mrw_req);
            }

            /* Per channel burst registers. */
            if (dram_type == DRAM_TYPE_LPDDR4) {
                for (u32 i = 0; i < dst_timing->num_burst_per_ch; i++) {
                    if (!PerChannelBurstRegisters[i]) {
                        continue;
                    }

                    const u32 addr = PerChannelBurstRegisters[i];
                    const u32 base = addr & ~0xFFF;

                    /* Filter out channels. */
                    if ((!ch0_enable && base == EMC0) || (!ch1_enable && base == EMC1)) {
                        continue;
                    }

                    /* Write the value. */
                    reg::Write(addr, dst_timing->burst_perch_regs_arr[i]);
                }
            }

            /* Vref regs. */
            for (u32 i = 0; i < dst_timing->vref_num; i++) {
                if (!PerChannelVrefRegisters[i]) {
                    continue;
                }

                const u32 addr = PerChannelVrefRegisters[i];
                const u32 base = addr & ~0xFFF;

                /* Filter out channels. */
                if ((!ch0_enable && base == EMC0) || (!ch1_enable && base == EMC1)) {
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

                    /* Filter out channels. */
                    if ((!ch0_enable && base == EMC0) || (!ch1_enable && base == EMC1)) {
                        continue;
                    }

                    /* Write the value. */
                    reg::Write(addr, dst_timing->training_mod_regs_arr[i]);
                }
            }

            /* Per channel trimmers. */
            for (u32 i = 0; i < dst_timing->num_trim_per_ch; i++) {
                if (!PerChannelTrimRegisters[i]) {
                    continue;
                }

                const u32 addr = PerChannelTrimRegisters[i];
                const u32 base = addr & ~0xFFF;

                /* Filter out channels. */
                if ((!ch0_enable && base == EMC0) || (!ch1_enable && base == EMC1)) {
                    continue;
                }

                uint32_t wval = dst_timing->trim_perch_regs_arr[i];

                if (compensate_trimmer_applicable) {
                    switch (addr) {
                        case EMC0_BASE + EMC_DATA_BRLSHFT_0:
                        case EMC1_BASE + EMC_DATA_BRLSHFT_0:
                        case EMC0_BASE + EMC_DATA_BRLSHFT_1:
                        case EMC1_BASE + EMC_DATA_BRLSHFT_1:
                            wval = ApplyPeriodicCompensationTrimmer(dst_timing, addr);
                            break;
                    }
                }

                /* Write the value. */
                reg::Write(addr, wval);
            }

            /* Trimmers. */
            for (u32 i = 0; i < dst_timing->num_trim; ++i) {
                if (!TrimRegisters[i]) {
                    continue;
                }

                const u32 addr = TrimRegisters[i];

                u32 wval = dst_timing->trim_regs_arr[i];

                if (compensate_trimmer_applicable) {
                    switch (addr) {
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE0_0:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE0_1:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE0_2:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE1_0:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE1_1:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE1_2:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE2_0:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE2_1:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE2_2:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE3_0:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE3_1:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE3_2:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE4_0:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE4_1:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE4_2:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE5_0:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE5_1:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE5_2:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE6_0:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE6_1:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE6_2:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE7_0:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE7_1:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE7_2:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE0_0:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE0_1:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE0_2:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE1_0:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE1_1:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE1_2:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE2_0:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE2_1:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE2_2:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE3_0:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE3_1:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE3_2:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE4_0:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE4_1:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE4_2:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE5_0:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE5_1:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE5_2:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE6_0:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE6_1:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE6_2:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE7_0:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE7_1:
                        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE7_2:
                            wval = ApplyPeriodicCompensationTrimmer(dst_timing, addr);
                            break;
                    }
                }

                /* Write the value. */
                reg::Write(addr, wval);
            }

            if (training_enabled) {
                if (train_wr && dst_timing->periodic_training) {
                    PeriodicCompensationHandler(WRITE_TRAINING_SEQUENCE, dram_dev_num, src_timing, dst_timing);
                }
            } else {
                /* Write burst_mc_regs. */
                for (u32 i = 0; i < dst_timing->num_mc_regs; i++) {
                    reg::Write(BurstMcRegisters[i], dst_timing->burst_mc_regs_arr[i]);
                }

                /* Registers to be programmed on the faster clock. */
                if (dst_timing->rate_khz < src_timing->rate_khz) {
                    for (u32 i = 0; i < dst_timing->num_up_down; i++) {
                        reg::Write(LaScaleRegisters[i], dst_timing->la_scale_regs_arr[i]);
                    }
                }
            }

            if ((dst_misc_cfg_1 & 2) != 0 && (dst_misc_cfg_1 & 1) == 0) {
                reg::Write(EMC + EMC_PMACRO_BRICK_CTRL_RFU1, dst_timing->burst_regs.emc_pmacro_brick_ctrl_rfu1);
                reg::Write(EMC + EMC_PMACRO_CMD_PAD_TX_CTRL, dst_timing->burst_regs.emc_pmacro_cmd_pad_tx_ctrl);
            }

            reg::Write(EMC + EMC_CFG_PIPE_CLK, (1 << 0));
            reg::Write(EMC + EMC_FDPD_CTRL_CMD_NO_RAMP, dst_timing->emc_fdpd_ctrl_cmd_no_ramp & ~(1 << 0));


            /* Step 9:
             *   LPDDR4 section A.
             */

            uint32_t emc_dbg_write_active = emc_dbg_o;
            if (dram_type == DRAM_TYPE_LPDDR4) {
                reg::Write(EMC + EMC_ZCAL_INTERVAL, emc_zcal_interval);
                reg::Write(EMC + EMC_ZCAL_WAIT_CNT, (dst_emc_zcal_wait_cnt & 0xFFFFF800) | 1);
                reg::Write(EMC + EMC_DBG, emc_dbg_o | ((1 << 1) | (1 << 30)));
                reg::Write(EMC + EMC_ZCAL_INTERVAL, emc_zcal_interval);
                reg::Write(EMC + EMC_DBG, emc_dbg_o);

                if (training_enabled) {
                    reg::Write(EMC + EMC_DBG, SetShadowBypass(ACTIVE, emc_dbg_o));
                    reg::Write(EMC + EMC_PMACRO_AUTOCAL_CFG_COMMON, dst_timing->burst_regs.emc_pmacro_autocal_cfg_common | (1 << 16));

                    if (train_ca || train_ca_vref) {
                        reg::Write(EMC + EMC_FBIO_CFG5, src_timing->burst_regs.emc_fbio_cfg5 | (1 << 27));
                    }

                    reg::Write(EMC + EMC_DBG, emc_dbg_o);

                    if (dst_emc_fbio_cfg7 == 0x6) {
                        CcfifoWrite(EMC_CFG_SYNC, 1, 0);
                    }

                    /* Change CFG_SWAP. */
                    CcfifoWrite(EMC_DBG, ((emc_dbg_o & 0xF3FFFFFF) | 0x4000000), 0);
                }

                CcfifoWrite(EMC_SELF_REF, 0x101, 0);

                if (!(train_ca || train_ca_vref) && dst_clock_period <= zqcal_before_cc_cutoff) {
                    CcfifoWrite(EMC_MRW3, mr13_flip_fspwr ^ 0x40, 0);
                    CcfifoWrite(EMC_MRW6, (src_timing->burst_regs.emc_mrw6 & 0x0000C0C0) | (dst_timing->burst_regs.emc_mrw6 & 0xFFFF3F3F), 0);
                    CcfifoWrite(EMC_MRW14, (src_timing->burst_regs.emc_mrw14 & 0x00003838) | (dst_timing->burst_regs.emc_mrw14 & 0xFFFF0707), 0);

                    if (dram_dev_num == TWO_RANK) {
                        CcfifoWrite(EMC_MRW7, (src_timing->burst_regs.emc_mrw7 & 0x0000C0C0) | (dst_timing->burst_regs.emc_mrw7 & 0xFFFF3F3F), 0);
                        CcfifoWrite(EMC_MRW15, (src_timing->burst_regs.emc_mrw15 & 0x00003838) | (dst_timing->burst_regs.emc_mrw15 & 0xFFFF0707), 0);
                    }

                    if (opt_zcal_en_cc) {
                        if (dram_dev_num == ONE_RANK || shared_zq_resistor) {
                            CcfifoWrite(EMC_ZQ_CAL, (2 << 30) | (1 << 0), 0);
                        } else {
                            CcfifoWrite(EMC_ZQ_CAL, (1 << 0), 0);
                        }
                    }
                }

                if (training_enabled) {
                    emc_dbg_write_active = (emc_dbg_o & 0xF7FFFFFF) | 0x4000000 | (1 << 30);
                    CcfifoWrite(EMC_DBG, emc_dbg_write_active, 0);
                }

                if (train_ca || train_ca_vref) {
                    CcfifoWrite(EMC_PMACRO_DATA_RX_TERM_MODE, src_timing->burst_regs.emc_pmacro_data_rx_term_mode & 0xFFFFFCCC, 0);

                    if (dram_dev_num == TWO_RANK && train_second_rank) {
                        CcfifoWrite(EMC_MRW3, mr13_flip_fspop | 0x8, (1000 * src_t_rp) / src_clock_period);
                        CcfifoWrite(EMC_MRW3, mr13_catr_enable | 0x8, 0);
                    } else {
                        CcfifoWrite(EMC_MRW3, mr13_catr_enable | 0x8, (1000 * src_t_rp) / src_clock_period);
                    }

                    CcfifoWrite(EMC_TR_CTRL_0, (dst_timing->emc_tr_ctrl_0 & 0x3F1000) | 0x100012A, 0);

                    CcfifoWrite(EMC_INTSTATUS, 0, 1000000 / src_clock_period);
                } else {
                    CcfifoWrite(EMC_MRW3, mr13_flip_fspop | 0x8, (1000 * src_t_rp) / src_clock_period);

                    CcfifoWrite(EMC_INTSTATUS, 0, std::max<u32>(DivideUpFloat(dst_t_fc_lpddr4_hz, src_clock_period), std::max<u32>(DivideUpFloat(14000, src_clock_period), 10)));
                }

                uint32_t t = 30 + (cya_allow_ref_cc ? ((4000 * src_t_rfc + 1000 * src_t_rp) / src_clock_period) : 0);
                CcfifoWrite(EMC_PIN, emc_pin_o & 0xFFFFFFF8, t);
            } else {
                CcfifoWrite(EMC_SELF_REF, 0x1, 0);
            }

            uint32_t ref_delay_mult = 1;
            if (ref_b4_sref_en) ++ref_delay_mult;
            if (cya_allow_ref_cc) ++ref_delay_mult;
            if (cya_issue_pc_ref) ++ref_delay_mult;

            uint32_t ref_delay = 20 + ref_delay_mult * (((1000 * src_t_rfc) / src_clock_period) + ((1000 * src_t_rp) / src_clock_period));

            /* Step 11:
             *   Ramp down.
             */

            CcfifoWrite(EMC_CFG_SYNC, 1, (dram_type == DRAM_TYPE_LPDDR4) ? 0 : ref_delay);

            bool do_ramp_up = (dst_misc_cfg_1 & 2) == 0 || (dst_misc_cfg_1 & 1) != 0;
            if (!do_ramp_up) {
                CcfifoWrite(EMC_FBIO_CFG5, reg::Read(EMC + EMC_FBIO_CFG5) & 0xF7FFFFFF, 12);
            }

            CcfifoWrite(EMC_DBG, SetShadowBypass(ACTIVE, emc_dbg_write_active & 0xBFFFFFFF), 0);

            if ((dst_misc_cfg_2 & 0x10) == 0) {
                DvfsPowerRampDown(src_timing, dst_timing, false, 0);
            }

            CcfifoWrite(EMC_DBG, SetShadowBypass(ACTIVE, emc_dbg_write_active | 0x40000000), 0);

            uint32_t ramp_down_wait = 0;
            if (do_ramp_up) {
                CcfifoWrite(EMC_PMACRO_CMD_PAD_TX_CTRL, src_timing->burst_regs.emc_pmacro_cmd_pad_tx_ctrl | (1 << 26), 0);
                CcfifoWrite(EMC_FBIO_CFG5, src_timing->burst_regs.emc_fbio_cfg5 | 0x100, 12);

                bool misc_flag = (dst_misc_cfg_1 & 3) == 3;

                uint32_t timescale = (100000 << ((dst_misc_cfg_1 >> 2) & 7));
                uint32_t delay     = (timescale / src_clock_period);

                if (src_rate_khz > 1150747 || misc_flag) {
                    CcfifoWrite(EMC_PMACRO_BRICK_CTRL_RFU1, src_timing->burst_regs.emc_pmacro_brick_ctrl_rfu1 & 0xFEEDFEED, delay + 1);
                    CcfifoWrite(EMC_PMACRO_BRICK_CTRL_RFU1, src_timing->burst_regs.emc_pmacro_brick_ctrl_rfu1 & 0xFE40FE40, delay + 1);
                    CcfifoWrite(EMC_PMACRO_BRICK_CTRL_RFU1, src_timing->burst_regs.emc_pmacro_brick_ctrl_rfu1 & 0xF800F800, delay + 1);


                    ramp_down_wait = 3 * timescale + 12 * src_clock_period;
                } else {
                    CcfifoWrite(EMC_PMACRO_BRICK_CTRL_RFU1, src_timing->burst_regs.emc_pmacro_brick_ctrl_rfu1 & 0xF800F800, delay + 20);

                    ramp_down_wait = timescale + 32 * src_clock_period;
                }

                if (src_rate_khz > 600240 || misc_flag) {
                    CcfifoWrite(EMC_INTSTATUS, 0, delay + 1);

                    ramp_down_wait += 2 * timescale;
                }
            }

            /* Step 12:
             *   Trigger the clock change.
             */

            CcfifoWrite(EMC_STALL_THEN_EXE_AFTER_CLKCHANGE, 1, 0);
            CcfifoWrite(EMC_INTSTATUS, 0, dst_timing->clkchange_delay);

            CcfifoWrite(EMC_DBG, SetShadowBypass(ACTIVE, emc_dbg_write_active & 0xBFFFFFFF), 0);

            if ((dst_misc_cfg_2 & 0x10) == 0) {
                DvfsPowerRampDown(src_timing, dst_timing, false, 1);
            }

            if (training_enabled) {
                CcfifoWrite(EMC_DBG, SetShadowBypass(ACTIVE, emc_dbg_write_active | 0x40000000), 0);
            }

            /* Step 13:
             *   Ramp up.
             */

            uint32_t ramp_up_wait = 0;
            if (do_ramp_up) {
                ramp_up_wait = DvfsPowerRampUp(dst_clock_period, false, src_timing, dst_timing, training);
            }

            if (ramp_up_wait < 1001000 && src_timing->ramp_wait != dst_timing->ramp_wait) {
                CcfifoWrite(EMC_INTSTATUS, 0, 1 + ((1001000 - ramp_up_wait) / dst_clock_period));
            }

            if (do_ramp_up) {
                CcfifoWrite(EMC_DBG, emc_dbg_write_active, 0);
            } else {
                CcfifoWrite(EMC_FBIO_CFG5, reg::Read(EMC + EMC_FBIO_CFG5), 0);
                CcfifoWrite(EMC_DBG, emc_dbg_write_active, 12);
            }

            /* Step 14:
             *   Bringup CKE pins.
             */

            if (dram_type == DRAM_TYPE_LPDDR4) {
                uint32_t pin_val;
                if (train_ca || train_ca_vref) {
                    pin_val = (dst_misc_cfg_0 & 1) ? ((emc_pin_o & 0xFFFCFFF8) | (((dst_misc_cfg_0 >> 1) & 3) << 16)) : emc_pin_o;
                    pin_val &= 0xFFFFFFF8;
                    if (dram_dev_num == TWO_RANK) {
                        if (train_second_rank) {
                            pin_val |= 5;
                        } else {
                            pin_val |= 6;
                        }
                    }
                } else {
                    pin_val = (dst_misc_cfg_0 & 1) ? ((emc_pin_o & 0xFFFCFFFF) | (((dst_misc_cfg_0 >> 1) & 3) << 16)) : emc_pin_o;
                    pin_val &= 0xFFFFFFF8;
                    if (dram_dev_num == TWO_RANK) {
                        pin_val |= 7;
                    } else {
                        pin_val |= 1;
                    }
                }

                CcfifoWrite(EMC_PIN, pin_val, 0);
            }

            /* Step 15:
             *   Calculate zqlatch wait time; has dependency on ramping times.
             */

            uint32_t dst_t_pdex_hz = 1000 * dst_t_pdex;

            uint32_t zq_latch_dvfs_wait_time;
            if (dst_clock_period <= zqcal_before_cc_cutoff) {
                zq_latch_dvfs_wait_time = (ramp_up_wait + ramp_down_wait) / dst_clock_period;
            } else {
                zq_latch_dvfs_wait_time = util::DivideUp(dst_t_pdex_hz, dst_clock_period);
            }

            if (!(train_ca || train_ca_vref) && (dram_type == DRAM_TYPE_LPDDR4) && opt_zcal_en_cc) {
                int offset = (int)((tZQCAL_lpddr4 - adj_dst_t_fc_lpddr4) / dst_clock_period) - (int)zq_latch_dvfs_wait_time;
                int addl_wait = (int)util::DivideUp(dst_t_pdex_hz, dst_clock_period);

                if (dram_dev_num == TWO_RANK) {
                    if (shared_zq_resistor) {
                        if (dst_clock_period > zqcal_before_cc_cutoff) {
                            CcfifoWrite(EMC_ZQ_CAL, (2 << 30) | (1 << 0), std::max<int>(0, addl_wait));
                        }

                        CcfifoWrite(EMC_ZQ_CAL, (2 << 30) | (1 << 1), std::max<int>(0, offset + addl_wait));
                        CcfifoWrite(EMC_ZQ_CAL, (1 << 30) | (1 << 0), 0);

                        if (!training_enabled) {
                            CcfifoWrite(EMC_MRW3, (mr13_flip_fspop & 0xFFFFFFF7) | 0xC000000 | (dst_misc_cfg_2 & 8), 0);
                            CcfifoWrite(EMC_SELF_REF, 0, 0);
                            CcfifoWrite(EMC_REF, 0, 0);
                        }

                        CcfifoWrite(EMC_ZQ_CAL, (1 << 30) | (1 << 1), zq_wait_short + div_14000_by_dst_period + (tZQCAL_lpddr4 / dst_clock_period));
                    } else {
                        if (dst_clock_period > zqcal_before_cc_cutoff) {
                            CcfifoWrite(EMC_ZQ_CAL, (0 << 30) | (1 << 0), std::max<int>(0, addl_wait));
                        }

                        if (!training_enabled) {
                            CcfifoWrite(EMC_MRW3, (mr13_flip_fspop & 0xFFFFFFF7) | 0xC000000 | (dst_misc_cfg_2 & 8), std::max<int>(0, addl_wait));
                            CcfifoWrite(EMC_SELF_REF, 0, 0);
                            CcfifoWrite(EMC_REF, 0, 0);
                        }

                        CcfifoWrite(EMC_ZQ_CAL, (0 << 30) | (1 << 1), div_14000_by_dst_period + std::max<int>(0, offset));
                    }
                } else {
                    if (dst_clock_period > zqcal_before_cc_cutoff) {
                        CcfifoWrite(EMC_ZQ_CAL, (2 << 30) | (1 << 0), std::max<int>(0, addl_wait));
                    }

                    if (!training_enabled) {
                        CcfifoWrite(EMC_MRW3, (mr13_flip_fspop & 0xFFFFFFF7) | 0xC000000 | (dst_misc_cfg_2 & 8), std::max<int>(0, addl_wait));
                        CcfifoWrite(EMC_SELF_REF, 0, 0);
                        CcfifoWrite(EMC_REF, 0x80000000, 0);
                    }

                    CcfifoWrite(EMC_ZQ_CAL, (2 << 30) | (1 << 1), div_14000_by_dst_period + std::max<int>(0, offset));
                }
            }

            /* WAR: delay for zqlatch */
            CcfifoWrite(EMC_INTSTATUS, 0, 10);

            /* Step 16:
             *   LPDDR4 Conditional Training Kickoff.
             */

            if (training_enabled && dram_type == DRAM_TYPE_LPDDR4) {
                if (opt_do_sw_qrst) {
                    CcfifoWrite(EMC_ISSUE_QRST, 1, 0);
                    CcfifoWrite(EMC_ISSUE_QRST, 0, 2);
                }

                CcfifoWrite(EMC_INTSTATUS, 0, (1020000 / dst_clock_period));

                {
                    uint32_t train_cmd = 0;
                    if (train_ca) {
                        train_cmd |= (1 << 1);  /* CA */
                    }
                    if (train_ca_vref) {
                        train_cmd |= (1 << 5);  /* CA_VREF */
                    }
                    if (train_wr) {
                        train_cmd |= (1 << 3);  /* WR */
                    }
                    if (train_wr_vref) {
                        train_cmd |= (1 << 6);  /* WR_VREF */
                    }
                    if (train_rd) {
                        train_cmd |= (1 << 2);  /* RD */
                    }
                    if (train_rd_vref) {
                        train_cmd |= (1 << 7);  /* RD_VREF */
                    }

                    train_cmd |= (1 << 31);     /* GO */

                    CcfifoWrite(EMC_TRAINING_CMD, train_cmd, 0);
                }

                CcfifoWrite(EMC_SWITCH_BACK_CTRL, 1, 0);

                if (!(train_ca || train_ca_vref) || train_second_rank) {
                    CcfifoWrite(EMC_MRW3, mr13_flip_fspop ^ 0xC0, 0);
                    CcfifoWrite(EMC_INTSTATUS, 0, (1000000 / dst_clock_period));
                }

                {
                    uint32_t pin_val = (dst_misc_cfg_0 & 1) ? ((emc_pin_o & 0xFFFCFFFF) | (((dst_misc_cfg_0 >> 1) & 3) << 16)) : emc_pin_o;
                    CcfifoWrite(EMC_PIN, pin_val & 0xFFFFFFF8, 0);
                }

                CcfifoWrite(EMC_CFG_SYNC, 1, 0);

                if ((src_misc_cfg_1 & 3) != 2) {
                    CcfifoWrite(EMC_DBG, SetShadowBypass(ACTIVE, emc_dbg_write_active | 0x40000000), 0);

                    CcfifoWrite(EMC_PMACRO_CMD_PAD_TX_CTRL, dst_timing->burst_regs.emc_pmacro_cmd_pad_tx_ctrl | (1 << 26), 0);
                    CcfifoWrite(EMC_FBIO_CFG5, dst_timing->burst_regs.emc_fbio_cfg5 | 0x100, 12);

                    bool misc_flag = (src_misc_cfg_1 & 3) == 3;

                    uint32_t timescale = (100000 << ((src_misc_cfg_1 >> 2) & 7));
                    uint32_t delay     = (timescale / dst_clock_period);

                    if (dst_rate_khz > 1150747 || misc_flag) {
                        CcfifoWrite(EMC_PMACRO_BRICK_CTRL_RFU1, dst_timing->burst_regs.emc_pmacro_brick_ctrl_rfu1 & 0xFEEDFEED, delay + 1);
                        CcfifoWrite(EMC_PMACRO_BRICK_CTRL_RFU1, dst_timing->burst_regs.emc_pmacro_brick_ctrl_rfu1 & 0xFE40FE40, delay + 1);
                        CcfifoWrite(EMC_PMACRO_BRICK_CTRL_RFU1, dst_timing->burst_regs.emc_pmacro_brick_ctrl_rfu1 & 0xF800F800, delay + 1);
                    } else {
                        CcfifoWrite(EMC_PMACRO_BRICK_CTRL_RFU1, dst_timing->burst_regs.emc_pmacro_brick_ctrl_rfu1 & 0xF800F800, delay + 20);
                    }

                    if (dst_rate_khz > 600240 || misc_flag) {
                        CcfifoWrite(EMC_INTSTATUS, 0, delay + 1);
                    }
                } else {
                    CcfifoWrite(EMC_FBIO_CFG5, reg::Read(EMC + EMC_FBIO_CFG5) & 0xF7FFFFFF, 12);
                    CcfifoWrite(EMC_DBG, SetShadowBypass(ACTIVE, emc_dbg_write_active | 0x40000000), 0);
                }

                CcfifoWrite(EMC_STALL_THEN_EXE_AFTER_CLKCHANGE, 1, 0);
                CcfifoWrite(EMC_DBG, SetShadowBypass(ACTIVE, emc_dbg_write_active & 0xBFFFFFFF), 0);

                if ((dst_misc_cfg_2 & 0x10) == 0) {
                    DvfsPowerRampDown(src_timing, dst_timing, true, 1);
                }

                if ((src_misc_cfg_1 & 3) != 2) {
                    DvfsPowerRampUp(src_clock_period, true, src_timing, dst_timing, training);

                    if (ramp_up_wait < 1001000 && src_timing->ramp_wait != dst_timing->ramp_wait) {
                        CcfifoWrite(EMC_INTSTATUS, 0, 1 + ((1001000 - ramp_up_wait) / dst_clock_period));
                    }

                    CcfifoWrite(EMC_DBG, emc_dbg_write_active, 0);
                } else {
                    if (ramp_up_wait < 1001000 && src_timing->ramp_wait != dst_timing->ramp_wait) {
                        CcfifoWrite(EMC_INTSTATUS, 0, 1 + ((1001000 - ramp_up_wait) / dst_clock_period));
                    }

                    CcfifoWrite(EMC_FBIO_CFG5, reg::Read(EMC + EMC_FBIO_CFG5), 0);
                    CcfifoWrite(EMC_DBG, emc_dbg_write_active, 12);
                }

                {
                    uint32_t pin_val = (src_misc_cfg_0 & 1) ? ((emc_pin_o & 0xFFFCFFFF) | (((src_misc_cfg_0 >> 1) & 3) << 16)) : emc_pin_o;
                    pin_val &= 0xFFFFFFF8;
                    pin_val |= (dram_dev_num == TWO_RANK) ? 7 : 1;
                    CcfifoWrite(EMC_PIN, pin_val, 0);
                }

                if (train_ca || train_ca_vref) {
                    CcfifoWrite(EMC_TR_CTRL_0, 0x2A, (200000 / src_clock_period));
                    CcfifoWrite(EMC_TR_CTRL_0, 0x20, (1000000 / src_clock_period));
                    CcfifoWrite(EMC_MRW3, mr13_catr_enable & 0xFFFFFFFE, 0);
                    CcfifoWrite(EMC_INTSTATUS, 0, (1000000 / src_clock_period));
                    CcfifoWrite(EMC_PMACRO_DATA_RX_TERM_MODE, src_timing->burst_regs.emc_pmacro_data_rx_term_mode, 0);
                }

                CcfifoWrite(EMC_DBG, emc_dbg_o, 0);

                if (opt_zcal_en_cc) {
                    if (shared_zq_resistor) {
                        CcfifoWrite(EMC_ZQ_CAL, (2 << 30) | (1 << 0), 0);
                        CcfifoWrite(EMC_ZQ_CAL, (2 << 30) | (1 << 1), 1 + util::DivideUp(1000000, src_clock_period));

                        if ((!(train_ca || train_ca_vref) || train_second_rank) && dram_dev_num == TWO_RANK) {
                            CcfifoWrite(EMC_ZQ_CAL, (1 << 30) | (1 << 0), 0);
                            CcfifoWrite(EMC_ZQ_CAL, (1 << 30) | (1 << 1), 1 + util::DivideUp(1000000, src_clock_period));
                        }
                    } else {
                        CcfifoWrite(EMC_ZQ_CAL, (dram_dev_num == ONE_RANK ? (2 << 30) : (0 << 30)) | (1 << 0), 0);
                        CcfifoWrite(EMC_ZQ_CAL, (dram_dev_num == ONE_RANK ? (2 << 30) : (0 << 30)) | (1 << 1), 1 + util::DivideUp(1000000, src_clock_period));
                    }
                }

                if (!(train_ca || train_ca_vref)) {
                    CcfifoWrite(EMC_MRW3, ((mr13_flip_fspop & 0xF3FFFFF7) | (dst_misc_cfg_2 & 8)) ^ 0x0C0000C0, 0);
                }

                CcfifoWrite(EMC_SELF_REF, 0x0, 0);
            }

            /* Step 17:
             *   exit self refresh.
             */

            if (dram_type != DRAM_TYPE_LPDDR4) {
                CcfifoWrite(EMC_SELF_REF, 0, 0);
            }

            /* Step 18:
             *   Send MRWs to LPDDR3/DDR3.
             */

            if (is_lpddr2) {
                CcfifoWrite(EMC_MRW2, dst_timing->emc_mrw2, 0);
                CcfifoWrite(EMC_MRW, dst_timing->emc_mrw, 0);

                if (is_lpddr3) {
                    CcfifoWrite(EMC_MRW4, dst_timing->emc_mrw4, 0);
                }
            } else if (dram_type == DRAM_TYPE_DDR4) {
                if (opt_dll_mode) {
                    CcfifoWrite(EMC_EMRS, dst_timing->emc_emrs & ~(1 << 26), 0);
                }
                CcfifoWrite(EMC_EMRS2, dst_timing->emc_emrs2 & ~(1 << 26), 0);
                CcfifoWrite(EMC_MRS, dst_timing->emc_mrs | (1 << 26), 0);
            }

            /* Step 19:
             *   ZQCAL for LPDDR3/DDR3
             */

            if (opt_zcal_en_cc) {
                if (is_lpddr2) {
                    uint32_t zq_op = opt_cc_short_zcal ? dst_timing->zq_op_cc_short_zcal : dst_timing->zq_op_cc_long_zcal;
                    uint32_t zcal_wait_time_ps = opt_cc_short_zcal ? dst_timing->zcal_wait_time_ps_cc_short_zcal : dst_timing->zcal_wait_time_ps_cc_long_zcal;
                    uint32_t zcal_wait_time_clocks = util::DivideUp(zcal_wait_time_ps, dst_clock_period);

                    CcfifoWrite(EMC_MRS_WAIT_CNT2, (zcal_wait_time_clocks & 0x3FF) | ((zcal_wait_time_clocks & 0x7FF) << 16), 0);
                    CcfifoWrite(EMC_MRW, (zq_op | 0x880C0000) - 0x20000, 0);

                    if (dram_dev_num == TWO_RANK) {
                        CcfifoWrite(EMC_MRW, zq_op | 0x480A0000, 0);
                    }
                } else if (dram_type == DRAM_TYPE_DDR4) {
                    CcfifoWrite(EMC_ZQ_CAL, (2 << 30) | (1 << 0) | (opt_cc_short_zcal ? 0 : (1 << 4)), 0);
                    if (dram_dev_num == TWO_RANK) {
                        CcfifoWrite(EMC_ZQ_CAL, (1 << 30) | (1 << 0) | (opt_cc_short_zcal ? 0 : (1 << 4)), 0);
                    }
                }
            }

            /* Step 20:
             *   Issue ref and optional QRST.
             */

            if (training_enabled || dram_type != DRAM_TYPE_LPDDR4) {
                CcfifoWrite(EMC_REF, dram_dev_num == ONE_RANK ? 0x80000000 : 0x00000000, 0);
            }

            if (opt_do_sw_qrst) {
                CcfifoWrite(EMC_ISSUE_QRST, 1, 0);
                CcfifoWrite(EMC_ISSUE_QRST, 0, 2);
            }

            /* Step 21:
             *   Restore ZCAL and ZCAL interval.
             */

            if (save_restore_clkstop_pd || opt_zcal_en_cc || (training_enabled && dram_type == DRAM_TYPE_LPDDR4)) {
                CcfifoWrite(EMC_DBG, SetShadowBypass(ACTIVE, emc_dbg_o), 0);

                if (opt_zcal_en_cc) {
                    if (training_enabled) {
                        CcfifoWrite(EMC_ZCAL_INTERVAL, (dst_misc_cfg_2 & 2) ? 0 : src_timing->burst_regs.emc_zcal_interval, 0);
                    } else if (dram_type != DRAM_TYPE_LPDDR4) {
                        CcfifoWrite(EMC_ZCAL_INTERVAL, (dst_misc_cfg_2 & 2) ? 0 : dst_timing->burst_regs.emc_zcal_interval, 0);
                    }
                }

                if (save_restore_clkstop_pd || (training_enabled && dram_type == DRAM_TYPE_LPDDR4)) {
                    CcfifoWrite(EMC_CFG, dst_timing->burst_regs.emc_cfg & ~(1 << 28), 0);
                }

                if (training_enabled && dram_type == DRAM_TYPE_LPDDR4) {
                    CcfifoWrite(EMC_SEL_DPD_CTRL, src_timing->emc_sel_dpd_ctrl, 0);
                }

                CcfifoWrite(EMC_DBG, emc_dbg_o, 0);
            }

            /* Step 22:
             *   Restore EMC_CFG_PIPE_CLK.
             */

            CcfifoWrite(EMC_CFG_PIPE_CLK, emc_cfg_pipe_clk_o, 0);
            CcfifoWrite(EMC_INTSTATUS, 0, dst_timing->pipe_clk_delay);

            /* Step 23:
             *   Do clock change.
             */

            if (training_enabled) {
                uint32_t clk_source_emc;
                if (ch0_enable || ch1_enable) {
                    clk_source_emc = reg::Read(CLKRST + CLK_RST_CONTROLLER_CLK_SOURCE_EMC);
                    reg::Write(CLKRST + CLK_RST_CONTROLLER_CLK_SOURCE_EMC_SAFE, clk_source_emc);
                } else {
                    clk_source_emc = 0;
                }

                ChangeDllSrc(src_timing, clk_source_emc);
            }

            {
                uint32_t dig_dll = (reg::Read(EMC + EMC_CFG_DIG_DLL) & 0xFFFFFFE4) | 8;
                if ((dst_misc_cfg_2 & 1) == 0) {
                    dig_dll = (dig_dll & 0xFFFFFF3F) | 0x80;
                }
                reg::Write(EMC + EMC_CFG_DIG_DLL, dig_dll);
                reg::Read(EMC + EMC_CFG_DIG_DLL);
            }

            reg::Read(MC + MC_EMEM_ADR_CFG);
            reg::Read(EMC + EMC_INTSTATUS);

            if (ch0_enable || ch1_enable) {
                reg::Write(CLKRST + CLK_RST_CONTROLLER_CLK_SOURCE_EMC, dst_clk_src);
                reg::Read(CLKRST + CLK_RST_CONTROLLER_CLK_SOURCE_EMC);
            }

            if (WaitForUpdate(EMC_INTSTATUS, (1 << 4), true, dst_emc_fbio_cfg7)) {
                return;
            }

            /* Step 24:
             *   Save training results.
             */

            if (training_enabled) {
                uint32_t tmp_emem_numdev = reg::Read(MC + MC_EMEM_ADR_CFG) & 1;

                uint32_t emc_dbg_tmp = reg::Read(EMC + EMC_DBG);
                reg::Write(EMC + EMC_DBG, emc_dbg_tmp | 1);       /* Set READ_MUX to ASSEMBLY. */

                uint32_t tmp_dram_dev_num = 1 + tmp_emem_numdev;

                /* Save CA results. */
                if (train_ca) {
                    dst_timing->trim_perch_regs.emc0_cmd_brlshft_0 = reg::Read(EMC0 + EMC_CMD_BRLSHFT_0);
                    dst_timing->trim_perch_regs.emc1_cmd_brlshft_1 = reg::Read(EMC1 + EMC_CMD_BRLSHFT_1);
                    dst_timing->trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_4 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_4);
                    dst_timing->trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_5 = reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_5);

                    if (train_bit_level) {
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd0_0 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD0_0);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd0_1 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD0_1);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd0_2 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD0_2);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd1_0 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD1_0);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd1_1 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD1_1);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd1_2 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD1_2);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd2_0 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD2_0);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd2_1 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD2_1);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd2_2 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD2_2);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd3_0 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD3_0);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd3_1 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD3_1);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd3_2 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD3_2);
                    }
                }

                /* Save CA_VREF results. */
                if (train_ca_vref) {
                    uint32_t emc0_training_opt_ca_vref = reg::Read(EMC0 + EMC_TRAINING_OPT_CA_VREF);
                    uint32_t emc1_training_opt_ca_vref = reg::Read(EMC1 + EMC_TRAINING_OPT_CA_VREF);

                    uint32_t rank_mask = tmp_dram_dev_num == TWO_RANK ? 0x480C0000 : 0xC80C0000;

                    dst_timing->burst_perch_regs.emc0_mrw10 = (emc0_training_opt_ca_vref & 0xFFFF) | 0x880C0000;
                    dst_timing->burst_perch_regs.emc1_mrw10 = (emc1_training_opt_ca_vref & 0xFFFF)  | 0x880C0000;
                    dst_timing->burst_perch_regs.emc0_mrw11 = (rank_mask & 0xFFFFFF00) | ((emc0_training_opt_ca_vref >> 16) & 0xFFFF);
                    dst_timing->burst_perch_regs.emc1_mrw11 = (rank_mask & 0xFFFFFF00) | ((emc1_training_opt_ca_vref >> 16) & 0xFFFF);
                }

                /* Save RD results. */
                if (train_rd) {
                    dst_timing->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank0_0 = reg::Read(EMC0 + EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_0);
                    dst_timing->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank0_1 = reg::Read(EMC0 + EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_1);
                    dst_timing->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank0_2 = reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_2);
                    dst_timing->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank0_3 = reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_3);

                    if (tmp_dram_dev_num == TWO_RANK) {
                        dst_timing->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank1_0 = reg::Read(EMC0 + EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_0);
                        dst_timing->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank1_1 = reg::Read(EMC0 + EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_1);
                        dst_timing->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank1_2 = reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_2);
                        dst_timing->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank1_3 = reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_3);
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
                        dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte4_0 = reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE4_0);
                        dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte4_1 = reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE4_1);
                        dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte4_2 = reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE4_2);
                        dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte5_0 = reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE5_0);
                        dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte5_1 = reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE5_1);
                        dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte5_2 = reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE5_2);
                        dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte6_0 = reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE6_0);
                        dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte6_1 = reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE6_1);
                        dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte6_2 = reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE6_2);
                        dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte7_0 = reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE7_0);
                        dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte7_1 = reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE7_1);
                        dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte7_2 = reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE7_2);

                        if (tmp_dram_dev_num == TWO_RANK) {
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
                            dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte4_0 = reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE4_0);
                            dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte4_1 = reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE4_1);
                            dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte4_2 = reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE4_2);
                            dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte5_0 = reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE5_0);
                            dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte5_1 = reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE5_1);
                            dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte5_2 = reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE5_2);
                            dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte6_0 = reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE6_0);
                            dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte6_1 = reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE6_1);
                            dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte6_2 = reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE6_2);
                            dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte7_0 = reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE7_0);
                            dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte7_1 = reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE7_1);
                            dst_timing->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte7_2 = reg::Read(EMC1 + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE7_2);
                        }
                    }

                    /* Save RD_VREF results. */
                    if (train_rd_vref) {
                        uint32_t emc_pmacro_ib_vref_dq_0 = reg::Read(EMC0 + EMC_PMACRO_IB_VREF_DQ_0);
                        uint32_t emc_pmacro_ib_vref_dq_1 = reg::Read(EMC0 + EMC_PMACRO_IB_VREF_DQ_1);

                        #define GET_SAVE_RESTORE_MOD_REG(n) ((dst_timing->save_restore_mod_regs[n] & 0x80000000) ? (~dst_timing->save_restore_mod_regs[n]) : (dst_timing->save_restore_mod_regs[n]))

                        uint8_t ib_vref_dq_byte0_icr = ((emc_pmacro_ib_vref_dq_0 >>  0) & 0x7F) + (GET_SAVE_RESTORE_MOD_REG(0) & 0x7F);
                        uint8_t ib_vref_dq_byte1_icr = ((emc_pmacro_ib_vref_dq_0 >>  8) & 0x7F) + (GET_SAVE_RESTORE_MOD_REG(1) & 0x7F);
                        uint8_t ib_vref_dq_byte2_icr = ((emc_pmacro_ib_vref_dq_0 >> 16) & 0x7F) + (GET_SAVE_RESTORE_MOD_REG(2) & 0x7F);
                        uint8_t ib_vref_dq_byte3_icr = ((emc_pmacro_ib_vref_dq_0 >> 24) & 0x7F) + (GET_SAVE_RESTORE_MOD_REG(3) & 0x7F);
                        uint8_t ib_vref_dq_byte4_icr = ((emc_pmacro_ib_vref_dq_1 >>  0) & 0x7F) + (GET_SAVE_RESTORE_MOD_REG(4) & 0x7F);
                        uint8_t ib_vref_dq_byte5_icr = ((emc_pmacro_ib_vref_dq_1 >>  8) & 0x7F) + (GET_SAVE_RESTORE_MOD_REG(5) & 0x7F);
                        uint8_t ib_vref_dq_byte6_icr = ((emc_pmacro_ib_vref_dq_1 >> 16) & 0x7F) + (GET_SAVE_RESTORE_MOD_REG(6) & 0x7F);
                        uint8_t ib_vref_dq_byte7_icr = ((emc_pmacro_ib_vref_dq_1 >> 24) & 0x7F) + (GET_SAVE_RESTORE_MOD_REG(7) & 0x7F);

                        dst_timing->trim_regs.emc_pmacro_ib_vref_dq_0 = ((ib_vref_dq_byte0_icr & 0x7F) | (ib_vref_dq_byte1_icr & 0x7F) << 8) | ((ib_vref_dq_byte2_icr & 0x7F) << 16) | ((ib_vref_dq_byte3_icr & 0x7F) << 24);


                        dst_timing->trim_regs.emc_pmacro_ib_vref_dq_1 = ((ib_vref_dq_byte4_icr & 0x7F) | (ib_vref_dq_byte5_icr & 0x7F) << 8) | ((ib_vref_dq_byte6_icr & 0x7F) << 16) | ((ib_vref_dq_byte7_icr & 0x7F) << 24);

                        #undef GET_SAVE_RESTORE_MOD_REG
                    }
                }

                /* Save WR results. */
                if (train_wr) {
                    dst_timing->trim_perch_regs.emc0_data_brlshft_0 = reg::Read(EMC0 + EMC_DATA_BRLSHFT_0);
                    dst_timing->trim_perch_regs.emc1_data_brlshft_0 = reg::Read(EMC1 + EMC_DATA_BRLSHFT_0);

                    if (tmp_dram_dev_num == TWO_RANK) {
                        dst_timing->trim_perch_regs.emc0_data_brlshft_1 = reg::Read(EMC0 + EMC_DATA_BRLSHFT_1);
                        dst_timing->trim_perch_regs.emc1_data_brlshft_1 = reg::Read(EMC1 + EMC_DATA_BRLSHFT_1);
                    }

                    dst_timing->trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_0 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_0);
                    dst_timing->trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_1 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_1);
                    dst_timing->trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_2 = reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_2);
                    dst_timing->trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_3 = reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_3);

                    if (tmp_dram_dev_num == TWO_RANK) {
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_long_dq_rank1_0 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_0);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_long_dq_rank1_1 = reg::Read(EMC0 + EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_1);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_long_dq_rank1_2 = reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_2);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_long_dq_rank1_3 = reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_3);
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
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte4_0 = reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE4_0);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte4_1 = reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE4_1);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte4_2 = reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE4_2);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte5_0 = reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE5_0);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte5_1 = reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE5_1);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte5_2 = reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE5_2);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte6_0 = reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE6_0);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte6_1 = reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE6_1);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte6_2 = reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE6_2);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte7_0 = reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE7_0);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte7_1 = reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE7_1);
                        dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte7_2 = reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE7_2);

                        if (tmp_dram_dev_num == TWO_RANK) {
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
                            dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte4_0 = reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE4_0);
                            dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte4_1 = reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE4_1);
                            dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte4_2 = reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE4_2);
                            dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte5_0 = reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE5_0);
                            dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte5_1 = reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE5_1);
                            dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte5_2 = reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE5_2);
                            dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte6_0 = reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE6_0);
                            dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte6_1 = reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE6_1);
                            dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte6_2 = reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE6_2);
                            dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte7_0 = reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE7_0);
                            dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte7_1 = reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE7_1);
                            dst_timing->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte7_2 = reg::Read(EMC1 + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE7_2);
                        }
                    }

                    /* Save WR_VREF results. */
                    if (train_wr_vref) {
                        uint32_t emc0_training_opt_dq_ob_vref = reg::Read(EMC0 + EMC_TRAINING_OPT_DQ_OB_VREF);
                        uint32_t emc1_training_opt_dq_ob_vref = reg::Read(EMC1 + EMC_TRAINING_OPT_DQ_OB_VREF);

                        #define GET_SAVE_RESTORE_MOD_REG(n) ((dst_timing->save_restore_mod_regs[n] & 0x80000000) ? (~dst_timing->save_restore_mod_regs[n]) : (dst_timing->save_restore_mod_regs[n]))
                        uint8_t mod_reg_8  = GET_SAVE_RESTORE_MOD_REG( 8);
                        uint8_t mod_reg_9  = GET_SAVE_RESTORE_MOD_REG( 9);
                        uint8_t mod_reg_10 = GET_SAVE_RESTORE_MOD_REG(10);
                        uint8_t mod_reg_11 = GET_SAVE_RESTORE_MOD_REG(11);
                        #undef GET_SAVE_RESTORE_MOD_REG

                        uint32_t rank_mask = tmp_dram_dev_num == TWO_RANK ? 0x480E0000 : 0xC80E0000;

                        uint8_t emc0_mrw12_byte0 = (mod_reg_8  + ((emc0_training_opt_dq_ob_vref >>  0) & 0xFF));
                        uint8_t emc0_mrw12_byte1 = (mod_reg_9  + ((emc0_training_opt_dq_ob_vref >>  8) & 0xFF));
                        uint8_t emc0_mrw13_byte0 = (mod_reg_8  + ((emc0_training_opt_dq_ob_vref >> 16) & 0xFF));
                        uint8_t emc0_mrw13_byte1 = (mod_reg_9  + ((emc0_training_opt_dq_ob_vref >> 24) & 0xFF));
                        uint8_t emc1_mrw12_byte0 = (mod_reg_10 + ((emc1_training_opt_dq_ob_vref >>  0) & 0xFF));
                        uint8_t emc1_mrw12_byte1 = (mod_reg_11 + ((emc1_training_opt_dq_ob_vref >>  8) & 0xFF));
                        uint8_t emc1_mrw13_byte0 = (mod_reg_10 + ((emc1_training_opt_dq_ob_vref >> 16) & 0xFF));
                        uint8_t emc1_mrw13_byte1 = (mod_reg_11 + ((emc1_training_opt_dq_ob_vref >> 24) & 0xFF));

                        dst_timing->burst_perch_regs.emc0_mrw12 = (emc0_mrw12_byte0 << 0) | (emc0_mrw12_byte1 << 8) | 0x880E0000;
                        dst_timing->burst_perch_regs.emc1_mrw12 = (emc1_mrw12_byte0 << 0) | (emc1_mrw12_byte1 << 8) | 0x880E0000;
                        dst_timing->burst_perch_regs.emc1_mrw13 = (emc1_mrw13_byte0 << 0) | (emc1_mrw13_byte1 << 8) | rank_mask;
                        dst_timing->burst_perch_regs.emc0_mrw13 = (emc0_mrw13_byte0 << 0) | (emc0_mrw13_byte1 << 8) | rank_mask;
                    }
                }

                reg::Write(EMC + EMC_DBG, emc_dbg_tmp);
            }

            /* Step 25:
             *   Program MC updown registers.
             */

            if (dst_timing->rate_khz > src_timing->rate_khz && !training_enabled) {
                for (u32 i = 0; i < dst_timing->num_up_down; i++) {
                    reg::Write(LaScaleRegisters[i], dst_timing->la_scale_regs_arr[i]);
                }

                /* Request a timing update. */
                TimingUpdate(dst_emc_fbio_cfg7);
            }

            /* Step 26:
             *   Restore ZCAL registers.
             */

            if (dram_type == DRAM_TYPE_LPDDR4 && !training_enabled) {
                reg::Write(EMC + EMC_DBG, SetShadowBypass(ACTIVE, emc_dbg_o));

                reg::Write(EMC + EMC_ZCAL_WAIT_CNT, dst_timing->burst_regs.emc_zcal_wait_cnt);
                reg::Write(EMC + EMC_ZCAL_INTERVAL, (dst_misc_cfg_2 & 2) ? 0 : dst_timing->burst_regs.emc_zcal_interval);

                reg::Write(EMC + EMC_DBG, emc_dbg_o);
            }

            if (dram_type != DRAM_TYPE_LPDDR4 && opt_cc_short_zcal && opt_zcal_en_cc && !opt_short_zcal) {
                util::WaitMicroSeconds(2);

                if (is_lpddr2) {
                    reg::Write(EMC + EMC_DBG, SetShadowBypass(ACTIVE, emc_dbg_o));

                    reg::Write(EMC + EMC_MRS_WAIT_CNT, dst_timing->burst_regs.emc_mrs_wait_cnt);

                    reg::Write(EMC + EMC_DBG, emc_dbg_o);
                } else if (dram_type == DRAM_TYPE_DDR4) {
                    reg::Write(EMC + EMC_DBG, SetShadowBypass(ACTIVE, emc_dbg_o));

                    reg::Write(EMC + EMC_ZCAL_WAIT_CNT, dst_timing->burst_regs.emc_zcal_wait_cnt);

                    reg::Write(EMC + EMC_DBG, emc_dbg_o);
                }
            }

            if (training_enabled) {
                if (!src_timing->pllm_misc1_0_pllm_clamp_ph90) {
                    reg::SetBits(CLKRST + CLK_RST_CONTROLLER_PLLM_MISC1, 0x80000000);
                }
            } else {
                if (!dst_timing->pllm_misc1_0_pllm_clamp_ph90) {
                    reg::SetBits(CLKRST + CLK_RST_CONTROLLER_PLLM_MISC1, 0x80000000);
                }
            }

            /* Step 27:
             *   Restore EMC_CFG, FDPD registers.
             */

            reg::Write(EMC + EMC_DBG, SetShadowBypass(ACTIVE, emc_dbg_o));
            reg::Write(EMC + EMC_CFG, dst_timing->burst_regs.emc_cfg);
            reg::Write(EMC + EMC_DBG, emc_dbg_o);

            reg::Write(EMC + EMC_FDPD_CTRL_CMD_NO_RAMP, dst_timing->emc_fdpd_ctrl_cmd_no_ramp);
            reg::Write(EMC + EMC_SEL_DPD_CTRL, dst_timing->emc_sel_dpd_ctrl);

            /* Step 28:
             *   Training recover.
             */

            if (training_enabled && dram_type == DRAM_TYPE_LPDDR4) {
                reg::Write(EMC + EMC_DBG, SetShadowBypass(ACTIVE, emc_dbg_o));

                reg::Write(EMC + EMC_CFG, dst_timing->burst_regs.emc_cfg);
                reg::Write(EMC + EMC_SEL_DPD_CTRL, dst_timing->emc_sel_dpd_ctrl);
                reg::Write(EMC + EMC_ZCAL_WAIT_CNT, src_timing->burst_regs.emc_zcal_wait_cnt);
                reg::Write(EMC + EMC_ZCAL_INTERVAL, (dst_misc_cfg_2 & 2) ? 0 : dst_timing->burst_regs.emc_zcal_interval);
                reg::Write(EMC + EMC_AUTO_CAL_CONFIG2, dst_timing->emc_auto_cal_config2);
                if (ch0_enable || ch1_enable) {
                    reg::Write(EMC0 + EMC_AUTO_CAL_CONFIG3, dst_timing->emc_auto_cal_config3);
                    reg::Write(EMC0 + EMC_AUTO_CAL_CONFIG4, dst_timing->emc_auto_cal_config4);
                    reg::Write(EMC0 + EMC_AUTO_CAL_CONFIG5, dst_timing->emc_auto_cal_config5);
                    reg::Write(EMC0 + EMC_AUTO_CAL_CONFIG6, dst_timing->emc_auto_cal_config6);
                    reg::Write(EMC0 + EMC_AUTO_CAL_CONFIG7, dst_timing->emc_auto_cal_config7);
                    reg::Write(EMC0 + EMC_AUTO_CAL_CONFIG8, dst_timing->emc_auto_cal_config8);
                }

                reg::Write(EMC + EMC_DBG, emc_dbg_o);

                reg::Write(EMC + EMC_TR_DVFS, dst_timing->burst_regs.emc_tr_dvfs & ~(1 << 0));
            }

            /* Step 29:
             *   Power fix WAR.
             */

            reg::Write(EMC + EMC_DBG, SetShadowBypass(ACTIVE, emc_dbg_o));

            reg::Write(EMC + EMC_PMACRO_AUTOCAL_CFG_COMMON, dst_timing->burst_regs.emc_pmacro_autocal_cfg_common);

            reg::Write(EMC + EMC_DBG, emc_dbg_o);

            reg::Write(EMC + EMC_PMACRO_CFG_PM_GLOBAL_0, 0xFF0000);
            reg::Write(EMC + EMC_PMACRO_TRAINING_CTRL_0, 0x8);
            reg::Write(EMC + EMC_PMACRO_TRAINING_CTRL_1, 0x8);
            reg::Write(EMC + EMC_PMACRO_CFG_PM_GLOBAL_0, 0);

            reg::Write(EMC + EMC_DBG, SetShadowBypass(ACTIVE, emc_dbg_o));

            /* Fixup xm2comppadctrl */
            if ((dst_misc_cfg_1 & 0x20) == 0) {
                uint32_t xm2comppadctrl = reg::Read(EMC + EMC_XM2COMPPADCTRL);

                reg::Write(EMC + EMC_XM2COMPPADCTRL, xm2comppadctrl & 0x1CFFFFFF);
                util::WaitMicroSeconds(1);
                reg::Write(EMC + EMC_XM2COMPPADCTRL, xm2comppadctrl & 0x0CFFFFFF);
                util::WaitMicroSeconds(1);
                reg::Write(EMC + EMC_XM2COMPPADCTRL, xm2comppadctrl & 0x04FFFFFF);
                util::WaitMicroSeconds(1);
            }

            reg::SetBits(EMC + EMC_PMACRO_DLL_CFG_1, 0x2000);
            reg::Read(EMC + EMC_PMACRO_DLL_CFG_1);
            util::WaitMicroSeconds(2);

            /* Select EMC DLL clock source. */
            {
                reg::SetBits(CLKRST + CLK_RST_CONTROLLER_CLK_SOURCE_EMC_DLL, 0xC00);
                reg::Read(CLKRST + CLK_RST_CONTROLLER_CLK_SOURCE_EMC_DLL);
            }

            reg::Write(EMC + EMC_DBG, SetShadowBypass(ASSEMBLY, emc_dbg_o));
            reg::Read(EMC + EMC_DBG);

            /* Step 30:
             *   Re-enable autocal.
             */

            if (!training_enabled) {
                if (dst_timing->burst_regs.emc_cfg_dig_dll & 1) {
                    uint32_t dig_dll = (reg::Read(EMC + EMC_CFG_DIG_DLL) & 0xFFFFFFE4) | 9;
                    if ((dst_misc_cfg_2 & 1) == 0) {
                        dig_dll = (dig_dll & 0xFFFFFF3F) | 0x80;
                    }
                    reg::Write(EMC + EMC_CFG_DIG_DLL, dig_dll);

                    /* Request a timing update. */
                    TimingUpdate(dst_emc_fbio_cfg7);
                }
            }

            if (!(training_enabled && dram_type == DRAM_TYPE_LPDDR4)) {
                reg::Write(EMC + EMC_AUTO_CAL_CONFIG, training_enabled ? src_timing->emc_auto_cal_config : dst_timing->emc_auto_cal_config);
            }

            if (training_enabled) {
                g_fsp_for_next_freq = !g_fsp_for_next_freq;
            }

            if (src_timing->periodic_training) {
                /* Reset all clock tree values. */
                src_timing->current_dram_clktree_c0d0u0 = src_timing->trained_dram_clktree_c0d0u0;
                src_timing->current_dram_clktree_c0d0u1 = src_timing->trained_dram_clktree_c0d0u1;
                src_timing->current_dram_clktree_c0d1u0 = src_timing->trained_dram_clktree_c0d1u0;
                src_timing->current_dram_clktree_c0d1u1 = src_timing->trained_dram_clktree_c0d1u1;
                src_timing->current_dram_clktree_c1d0u0 = src_timing->trained_dram_clktree_c1d0u0;
                src_timing->current_dram_clktree_c1d0u1 = src_timing->trained_dram_clktree_c1d0u1;
                src_timing->current_dram_clktree_c1d1u0 = src_timing->trained_dram_clktree_c1d1u0;
                src_timing->current_dram_clktree_c1d1u1 = src_timing->trained_dram_clktree_c1d1u1;
            }

            /* Disable pll. */
            PllDisable(dst_clk_src);
        }

        void CleanupActiveShadowCopy(EmcDvfsTimingTable *src_timing, EmcDvfsTimingTable *dst_timing) {
            /* Change CFG_SWAP to ASSEMBLY_ONLY */
            uint32_t emc_dbg = reg::Read(EMC + EMC_DBG);
            emc_dbg = ((emc_dbg & 0xF3FFFFFF) | 0x8000000);
            reg::Write(EMC + EMC_DBG, emc_dbg);

            /* Wait for update. */
            TimingUpdate(src_timing->emc_fbio_cfg7);

            /* Change CFG_SWAP to ACTIVE_ONLY */
            emc_dbg = reg::Read(EMC + EMC_DBG);
            emc_dbg &= 0xF3FFFFFF;
            reg::Write(EMC + EMC_DBG, emc_dbg);

            /* Set PMACRO_DLL_CFG_1, preserving MDDLL_SEL_CLK_SRC */
            reg::Write(EMC + EMC_PMACRO_DLL_CFG_1, (reg::Read(EMC + EMC_PMACRO_DLL_CFG_1) & 0x00002000) | (src_timing->burst_regs.emc_pmacro_dll_cfg_1 & 0xFFFFDFFF));

            /* Update CFG_DIG_DLL */
            uint32_t emc_cfg_dig_dll = reg::Read(EMC + EMC_CFG_DIG_DLL);
            emc_cfg_dig_dll = (dst_timing->misc_cfg_2 & 1) ? (emc_cfg_dig_dll & 0xFFFFFFFE) : ((emc_cfg_dig_dll & 0xFFFFFF3E) | 0x80);
            reg::Write(EMC + EMC_CFG_DIG_DLL, emc_cfg_dig_dll);

            /* Wait for update. */
            TimingUpdate(src_timing->emc_fbio_cfg7);

            /* Disable or enable DLL */
            emc_cfg_dig_dll = reg::Read(EMC + EMC_CFG_DIG_DLL);
            if (src_timing->burst_regs.emc_cfg_dig_dll & 1) {
                emc_cfg_dig_dll |= 0x01;
            } else {
                emc_cfg_dig_dll &= 0xFFFFFFFE;
            }
            if ((dst_timing->misc_cfg_2 & 1) == 0) {
                emc_cfg_dig_dll = ((emc_cfg_dig_dll & 0xFFFFFF3F) | 0x80);
            }
            reg::Write(EMC + EMC_CFG_DIG_DLL, emc_cfg_dig_dll);

            /* Wait for update. */
            TimingUpdate(src_timing->emc_fbio_cfg7);

            /* Wait for DLL_LOCK to be set */
            WaitForUpdate(EMC_DIG_DLL_STATUS, (1 << 2), true, src_timing->emc_fbio_cfg7);

            /* Wait for update. */
            TimingUpdate(src_timing->emc_fbio_cfg7);

            /* Set AUTO_CAL_CONFIG. */
            uint32_t emc_auto_cal_config = reg::Read(EMC + EMC_AUTO_CAL_CONFIG);
            emc_auto_cal_config = (dst_timing->misc_cfg_2 & 4) ? (emc_auto_cal_config & 0xDFFFF9FF) : ((emc_auto_cal_config & 0x5FFFF9FF) | 0x80000000);
            reg::Write(EMC + EMC_AUTO_CAL_CONFIG, emc_auto_cal_config | 0x20000000);
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

            reg::Write(EMC + EMC_TRAINING_QUSE_CTRL_MISC, (dst_timing->burst_regs.emc_training_read_ctrl_misc & 0xFFFF0000) | 0x00001000);

            /* Do training, if we need to. */
            const u32 needed_training = dst_timing->needs_training;
            if (needed_training && !dst_timing->trained) {
                /* Determine what training to do. */
                u32 training_params[4];
                u32 num_params = 0;

                if (needed_training & (CA_TRAINING | CA_VREF_TRAINING)) {
                    training_params[num_params++] = (needed_training & (CA_TRAINING | CA_VREF_TRAINING | BIT_LEVEL_TRAINING));
                    if (dram_dev_num == TWO_RANK) {
                        training_params[num_params++] = (needed_training & (CA_TRAINING | CA_VREF_TRAINING | TRAIN_SECOND_RANK | BIT_LEVEL_TRAINING));
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

        void Dvfs(EmcDvfsTimingTable *dst_timing, EmcDvfsTimingTable *src_timing, bool train) {
            /* Get the clock sources/rates. */
            u32 clk_src_emc_from = src_timing->clk_src_emc;
            u32 clk_src_emc_to   = dst_timing->clk_src_emc;
            u32 rate_from        = src_timing->rate_khz;
            u32 rate_to          = dst_timing->rate_khz;

            /* Get channel enables. */
            const bool ch0_enable = reg::GetField(dst_timing->emc_fbio_cfg7, EMC_REG_BITS_MASK(FBIO_CFG7_CH0_ENABLE)) == EMC_FBIO_CFG7_CH0_ENABLE_ENABLE;
            const bool ch1_enable = reg::GetField(dst_timing->emc_fbio_cfg7, EMC_REG_BITS_MASK(FBIO_CFG7_CH1_ENABLE)) == EMC_FBIO_CFG7_CH1_ENABLE_ENABLE;

            /* Reprogram pll. */
            const u32 prev_2x_clk_src = reg::GetField(clk_src_emc_from, CLK_RST_REG_BITS_MASK(CLK_SOURCE_EMC_EMC_2X_CLK_SRC));
            const u32 next_2x_clk_src = reg::GetField(clk_src_emc_to,   CLK_RST_REG_BITS_MASK(CLK_SOURCE_EMC_EMC_2X_CLK_SRC));
            if (next_2x_clk_src != PLLP_OUT0 && next_2x_clk_src != PLLP_UD) {
                if (ch0_enable || ch1_enable) {
                    if (PllReprogram(rate_to, clk_src_emc_to, rate_from, clk_src_emc_from)) {
                        if (prev_2x_clk_src == PLLMB_UD || prev_2x_clk_src == PLLMB_OUT0) {
                            g_next_pll = 0;
                        } else if (prev_2x_clk_src == PLLM_UD || prev_2x_clk_src == PLLM_OUT0) {
                            g_next_pll = !g_next_pll;
                        }

                        clk_src_emc_to = ProgramPllm(rate_to, clk_src_emc_to, clk_src_emc_to, g_next_pll, dst_timing);
                    } else {
                        if (next_2x_clk_src == PLLM_UD || next_2x_clk_src == PLLMB_UD) {
                            if (g_next_pll) {
                                reg::SetField(clk_src_emc_to, CLK_RST_REG_BITS_VALUE(CLK_SOURCE_EMC_EMC_2X_CLK_SRC, PLLMB_UD));
                            }
                        } else if (next_2x_clk_src == PLLM_OUT0 || next_2x_clk_src == PLLMB_OUT0) {
                            if (g_next_pll) {
                                reg::SetField(clk_src_emc_to, CLK_RST_REG_BITS_VALUE(CLK_SOURCE_EMC_EMC_2X_CLK_SRC, PLLMB_OUT0));
                            }
                        }
                    }
                }
            }

            if (train) {
                TrainFreq(src_timing, dst_timing, clk_src_emc_to);
                if (ch0_enable || ch1_enable) {
                    if (PllReprogram(dst_timing->rate_khz, dst_timing->clk_src_emc, src_timing->rate_khz, src_timing->clk_src_emc)) {
                        g_next_pll = !g_next_pll;
                    }
                }
            } else {
                FreqChange(src_timing, dst_timing, 0, clk_src_emc_to);
            }
        }

    }

    void DoMemoryTrainingMariko(bool *out_did_training, int index, void *mtc_tables_buffer) {
        /* Get timing tables. */
        auto *timing = GetEmcDvfsTimingTables(index, mtc_tables_buffer);
        auto *src_timing    = timing + 0;
        auto *dst_timing    = timing + 1;

        /* Check timing tables. */
        if (src_timing->rate_khz != 204000 || dst_timing->rate_khz != 1600000) {
            ShowFatalError("EmcDvfsTimingTables seem corrupted %" PRIu32 " %" PRIu32 "?\n", src_timing->rate_khz, dst_timing->rate_khz);
        }

        /* Check that we should do training. */
        if (src_timing->clk_src_emc != reg::Read(CLKRST + CLK_RST_CONTROLLER_CLK_SOURCE_EMC)) {
            /* Our clock source isn't what's expected, so presumably training has already been done? */
            /* Either way, the safe bet is to skip it. */
            *out_did_training = false;
            return;
        }

        /* Train 1600MHz. */
        Dvfs(dst_timing, src_timing, true);

        /* Switch to 1600MHz. */
        Dvfs(dst_timing, src_timing, false);

        /* Set ourselves as having done training */
        *out_did_training = true;
    }

    void RestoreMemoryClockRateMariko(void *mtc_tables_buffer) {
        /* Get timing tables. */
        auto *timing_tables = reinterpret_cast<EmcDvfsTimingTable *>(mtc_tables_buffer);
        auto *src_timing    = timing_tables + 0;
        auto *dst_timing    = timing_tables + 1;

        /* Check timing tables. */
        if (src_timing->rate_khz != 204000 || dst_timing->rate_khz != 1600000) {
            ShowFatalError("EmcDvfsTimingTables seem corrupted %" PRIu32 " %" PRIu32 "?\n", src_timing->rate_khz, dst_timing->rate_khz);
        }

        /* Switch to 204MHz */
        Dvfs(src_timing, dst_timing, false);
    }

}
