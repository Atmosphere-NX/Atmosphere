/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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

        using EmcDvfsTimingTable = mariko::EmcDvfsTimingTable;

        EmcDvfsTimingTable *GetEmcDvfsTimingTables(int index, void *mtc_tables_buffer) {
            /* Get the compressed table. */
            u8 *cmp_table;
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

        u32 ProgramPllm(u32 next_rate_khz, u32 next_clk_src, u32 ret_clk_src, bool is_pllmb, EmcDvfsTimingTable *timing_tables) {
            u32 ret = ret_clk_src;

            const uint32_t base = ((timing_tables->pllmb_divm & 0xFF) | ((timing_tables->pllmb_divn & 0xFF) << 8) | ((timing_tables->pllmb_divp & 1) << 20));
            if (is_pllmb) {
                reg::Write(CLKRST + CLK_RST_CONTROLLER_PLLMB_BASE, base);
                reg::Read(CLKRST + CLK_RST_CONTROLLER_PLLMB_BASE);

                reg::SetBits(CLKRST + CLK_RST_CONTROLLER_PLLMB_MISC1, 0x10000000);

                if (timing_tables->pll_en_ssc & 1) {
                    reg::Write(CLKRST + CLK_RST_CONTROLLER_PLLMB_SS_CFG,   timing_tables->pllmb_ss_cfg);
                    reg::Write(CLKRST + CLK_RST_CONTROLLER_PLLMB_SS_CTRL1, timing_tables->pllmb_ss_ctrl1);
                    reg::Write(CLKRST + CLK_RST_CONTROLLER_PLLMB_SS_CTRL2, timing_tables->pllmb_ss_ctrl2);
                } else {
                    reg::Write(CLKRST + CLK_RST_CONTROLLER_PLLMB_SS_CFG,   timing_tables->pllmb_ss_cfg   & 0xBFFFFFFF);
                    reg::Write(CLKRST + CLK_RST_CONTROLLER_PLLMB_SS_CTRL2, timing_tables->pllmb_ss_ctrl2 & 0x0000FFFF);
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

                if (timing_tables->pll_en_ssc & 1) {
                    reg::Write(CLKRST + CLK_RST_CONTROLLER_PLLM_SS_CFG,   timing_tables->pllm_ss_cfg);
                    reg::Write(CLKRST + CLK_RST_CONTROLLER_PLLM_SS_CTRL1, timing_tables->pllm_ss_ctrl1);
                    reg::Write(CLKRST + CLK_RST_CONTROLLER_PLLM_SS_CTRL2, timing_tables->pllm_ss_ctrl2);
                } else {
                    reg::Write(CLKRST + CLK_RST_CONTROLLER_PLLM_SS_CFG,   timing_tables->pllm_ss_cfg   & 0xBFFFFFFF);
                    reg::Write(CLKRST + CLK_RST_CONTROLLER_PLLM_SS_CTRL2, timing_tables->pllm_ss_ctrl2 & 0x0000FFFF);
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

        void FreqChange(EmcDvfsTimingTable *src_timing, EmcDvfsTimingTable *dst_timing, u32 training, u32 next_clk_src) {
            /* TODO */
        }

        void TrainFreq(EmcDvfsTimingTable *src_timing, EmcDvfsTimingTable *dst_timing, u32 next_clk_src) {
            /* TODO */
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
        auto *timing_tables = GetEmcDvfsTimingTables(index, mtc_tables_buffer);
        auto *src_timing    = timing_tables + 0;
        auto *dst_timing    = timing_tables + 1;

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
