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

        #include "fusee_mtc_tables_erista.inc"
        #include "fusee_mtc_ram_training_pattern_erista.inc"

        using EmcDvfsTimingTable = erista::EmcDvfsTimingTable;

        EmcDvfsTimingTable *GetEmcDvfsTimingTables() {
            const auto index = GetMemoryTrainingTableIndex();
            switch (index) {
                case 0:
                case 3:
                    return reinterpret_cast<EmcDvfsTimingTable *>(T210SdevEmcDvfsTableS4gb01);
                case 1:
                    return reinterpret_cast<EmcDvfsTimingTable *>(T210SdevEmcDvfsTableS6gb01);
                case 2:
                    return reinterpret_cast<EmcDvfsTimingTable *>(T210SdevEmcDvfsTableH4gb01);
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
            if (next_rate_khz != 1600000) {
                ShowFatalError("Unexpected ProgramPllm next rate %" PRIu32 "\n", next_rate_khz);
            }

            const u32 divn = 0x7D;
            const u32 divm = 0x03;
            const u32 divp = 0x00;

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

        void FreqChange(EmcDvfsTimingTable *src_timing_tables, EmcDvfsTimingTable *dst_timing_tables, u32 training, u32 next_clk_src) {
            /* TODO */
        }

        void CleanupActiveShadowCopy(EmcDvfsTimingTable *src_timing_tables, EmcDvfsTimingTable *dst_timing_tables) {
            /* TODO */
        }

        void TrainFreq(EmcDvfsTimingTable *src_timing_tables, EmcDvfsTimingTable *dst_timing_tables, u32 next_clk_src) {
            /* Get dram dev num. */
            const u32 dram_dev_num = (reg::Read(MC + MC_EMEM_ADR_CFG) & 1) + 1;

            /* Write RAM patterns, if first training. */
            if (!g_did_first_training) {
                const auto * const pattern = GetEmcRamTrainingPattern();
                for (u32 i = 0; i < 0x100; ++i) {
                    reg::Write(EMC + EMC_TRAINING_PATRAM_DQ,   pattern[dst_timing_tables->training_pattern].dq[i]);
                    reg::Write(EMC + EMC_TRAINING_PATRAM_DMI,  pattern[dst_timing_tables->training_pattern].dmi[i]);
                    reg::Write(EMC + EMC_TRAINING_PATRAM_CTRL, 0x80000000 | i);
                }

                g_did_first_training = true;
            }

            /* Do training, if we need to. */
            const u32 needed_training = dst_timing_tables->needs_training;
            if (needed_training && !dst_timing_tables->trained) {
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
                    FreqChange(src_timing_tables, dst_timing_tables, training_params[i], next_clk_src);
                    CleanupActiveShadowCopy(src_timing_tables, dst_timing_tables);
                }

                /* Set tables as trained. */
                dst_timing_tables->trained = 1;
            }
        }

        void Dvfs(EmcDvfsTimingTable *dst_timing_tables, EmcDvfsTimingTable *src_timing_tables, bool train) {
            /* Get the old 2x clock source. */
            const u32 prev_2x_clk_src = reg::GetValue(CLKRST + CLK_RST_CONTROLLER_CLK_SOURCE_EMC, CLK_RST_REG_BITS_MASK(CLK_SOURCE_EMC_EMC_2X_CLK_SRC));

            /* Set g_next_pll. */
            g_next_pll = prev_2x_clk_src == PLLMB_UD || prev_2x_clk_src == PLLMB_OUT0;

            /* Reprogram pll. */
            u32 next_clk_src;
            if (PllReprogram(dst_timing_tables->rate_khz, dst_timing_tables->clk_src_emc, src_timing_tables->rate_khz, src_timing_tables->clk_src_emc)) {
                if (prev_2x_clk_src == PLLMB_UD || prev_2x_clk_src == PLLMB_OUT0) {
                    g_next_pll = 0;
                } else if (prev_2x_clk_src == PLLM_UD || prev_2x_clk_src == PLLM_OUT0) {
                    g_next_pll = !g_next_pll;
                }

                next_clk_src = ProgramPllm(dst_timing_tables->rate_khz, dst_timing_tables->clk_src_emc, g_next_pll);
            } else {
                next_clk_src = dst_timing_tables->clk_src_emc;

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
                TrainFreq(src_timing_tables, dst_timing_tables, next_clk_src);
                if (PllReprogram(dst_timing_tables->rate_khz, dst_timing_tables->clk_src_emc, src_timing_tables->rate_khz, src_timing_tables->clk_src_emc)) {
                    g_next_pll = !g_next_pll;
                }
            } else {
                FreqChange(src_timing_tables, dst_timing_tables, 0, next_clk_src);
            }
        }

    }

    void DoMemoryTrainingErista() {
        /* Get timing tables. */
        auto *timing_tables     = GetEmcDvfsTimingTables();
        auto *src_timing_tables = timing_tables + 0;
        auto *dst_timing_tables = timing_tables + 1;

        /* Check timing tables. */
        if (src_timing_tables->rate_khz != 204000 || dst_timing_tables->rate_khz != 1600000) {
            ShowFatalError("EmcDvfsTimingTables seem corrupted %" PRIu32 " %" PRIu32 "?\n", src_timing_tables->rate_khz, dst_timing_tables->rate_khz);
        }

        /* Check that we should do training. */
        if (src_timing_tables->clk_src_emc != reg::Read(CLKRST + CLK_RST_CONTROLLER_CLK_SOURCE_EMC)) {
            /* Our clock source isn't what's expected, so presumably training has already been done? */
            /* Either way, the safe bet is to skip it. */
            return;
        }

        /* Train 1600MHz. */
        Dvfs(dst_timing_tables, src_timing_tables, true);

        /* Switch to 1600MHz. */
        Dvfs(dst_timing_tables, src_timing_tables, false);

        /* TODO: Periodic compensation */
    }

}
