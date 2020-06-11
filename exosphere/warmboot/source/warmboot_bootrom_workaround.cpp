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
#include "warmboot_clkrst.hpp"

namespace ams::warmboot {

    namespace {

        constexpr inline const uintptr_t CLKRST = secmon::MemoryRegionPhysicalDeviceClkRst.GetAddress();
        constexpr inline const uintptr_t EMC    = EMC_ADDRESS(0);

    }

    void ApplyMbistWorkaround() {
        /* Clear all LVL2 clock gate overrides to zero. */
        reg::Write(CLKRST + CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRA, 0);
        reg::Write(CLKRST + CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRB, 0);
        reg::Write(CLKRST + CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRC, 0);
        reg::Write(CLKRST + CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRD, 0);
        reg::Write(CLKRST + CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRE, 0);

        /* Clear clock enable for all but a select few devices. */
        auto devices_to_clear_l = reg::Read(CLKRST + CLK_RST_CONTROLLER_CLK_OUT_ENB_L);
        reg::ClearBits(static_cast<volatile u32 &>(devices_to_clear_l), CLK_RST_REG_BITS_MASK(CLK_ENB_L_CLK_ENB_RTC   ),
                                                                        CLK_RST_REG_BITS_MASK(CLK_ENB_L_CLK_ENB_TMR   ),
                                                                        CLK_RST_REG_BITS_MASK(CLK_ENB_L_CLK_ENB_GPIO  ),
                                                                        CLK_RST_REG_BITS_MASK(CLK_ENB_L_CLK_ENB_CACHE2));
        reg::Write(CLKRST + CLK_RST_CONTROLLER_CLK_ENB_L_CLR, devices_to_clear_l);

        auto devices_to_clear_h = reg::Read(CLKRST + CLK_RST_CONTROLLER_CLK_OUT_ENB_H);
        reg::ClearBits(static_cast<volatile u32 &>(devices_to_clear_h), CLK_RST_REG_BITS_MASK(CLK_ENB_H_CLK_ENB_MEM ),
                                                                        CLK_RST_REG_BITS_MASK(CLK_ENB_H_CLK_ENB_PMC ),
                                                                        CLK_RST_REG_BITS_MASK(CLK_ENB_H_CLK_ENB_FUSE),
                                                                        CLK_RST_REG_BITS_MASK(CLK_ENB_H_CLK_ENB_EMC ));
        reg::Write(CLKRST + CLK_RST_CONTROLLER_CLK_ENB_H_CLR, devices_to_clear_h);

        auto devices_to_clear_u = reg::Read(CLKRST + CLK_RST_CONTROLLER_CLK_OUT_ENB_U);
        reg::ClearBits(static_cast<volatile u32 &>(devices_to_clear_u), CLK_RST_REG_BITS_MASK(CLK_ENB_U_CLK_ENB_CSITE),
                                                                        CLK_RST_REG_BITS_MASK(CLK_ENB_U_CLK_ENB_IRAMA),
                                                                        CLK_RST_REG_BITS_MASK(CLK_ENB_U_CLK_ENB_IRAMB),
                                                                        CLK_RST_REG_BITS_MASK(CLK_ENB_U_CLK_ENB_IRAMC),
                                                                        CLK_RST_REG_BITS_MASK(CLK_ENB_U_CLK_ENB_IRAMD),
                                                                        CLK_RST_REG_BITS_MASK(CLK_ENB_U_CLK_ENB_CRAM2));
        reg::Write(CLKRST + CLK_RST_CONTROLLER_CLK_ENB_U_CLR, devices_to_clear_u);

        auto devices_to_clear_v = reg::Read(CLKRST + CLK_RST_CONTROLLER_CLK_OUT_ENB_V);
        reg::ClearBits(static_cast<volatile u32 &>(devices_to_clear_v), CLK_RST_REG_BITS_MASK(CLK_ENB_V_CLK_ENB_MSELECT      ),
                                                                        CLK_RST_REG_BITS_MASK(CLK_ENB_V_CLK_ENB_SPDIF_DOUBLER),
                                                                        CLK_RST_REG_BITS_MASK(CLK_ENB_V_CLK_ENB_TZRAM        ),
                                                                        CLK_RST_REG_BITS_MASK(CLK_ENB_V_CLK_ENB_SE           ));
        reg::Write(CLKRST + CLK_RST_CONTROLLER_CLK_ENB_V_CLR, devices_to_clear_v);

        auto devices_to_clear_w = reg::Read(CLKRST + CLK_RST_CONTROLLER_CLK_OUT_ENB_W);
        reg::ClearBits(static_cast<volatile u32 &>(devices_to_clear_w), CLK_RST_REG_BITS_MASK(CLK_ENB_W_CLK_ENB_PCIERX0),
                                                                        CLK_RST_REG_BITS_MASK(CLK_ENB_W_CLK_ENB_PCIERX1),
                                                                        CLK_RST_REG_BITS_MASK(CLK_ENB_W_CLK_ENB_PCIERX2),
                                                                        CLK_RST_REG_BITS_MASK(CLK_ENB_W_CLK_ENB_PCIERX3),
                                                                        CLK_RST_REG_BITS_MASK(CLK_ENB_W_CLK_ENB_PCIERX4),
                                                                        CLK_RST_REG_BITS_MASK(CLK_ENB_W_CLK_ENB_PCIERX5),
                                                                        CLK_RST_REG_BITS_MASK(CLK_ENB_W_CLK_ENB_ENTROPY));
        reg::Write(CLKRST + CLK_RST_CONTROLLER_CLK_ENB_W_CLR, devices_to_clear_w);

        auto devices_to_clear_x = reg::Read(CLKRST + CLK_RST_CONTROLLER_CLK_OUT_ENB_X);
        reg::ClearBits(static_cast<volatile u32 &>(devices_to_clear_x), CLK_RST_REG_BITS_MASK(CLK_ENB_X_CLK_ENB_MC_CAPA ),
                                                                        CLK_RST_REG_BITS_MASK(CLK_ENB_X_CLK_ENB_MC_CBPA ),
                                                                        CLK_RST_REG_BITS_MASK(CLK_ENB_X_CLK_ENB_MC_CPU  ),
                                                                        CLK_RST_REG_BITS_MASK(CLK_ENB_X_CLK_ENB_MC_BBC  ),
                                                                        CLK_RST_REG_BITS_MASK(CLK_ENB_X_CLK_ENB_EMC_DLL ),
                                                                        CLK_RST_REG_BITS_MASK(CLK_ENB_X_CLK_ENB_GPU     ),
                                                                        CLK_RST_REG_BITS_MASK(CLK_ENB_X_CLK_ENB_DBGAPB  ),
                                                                        CLK_RST_REG_BITS_MASK(CLK_ENB_X_CLK_ENB_PLLG_REF));
        reg::Write(CLKRST + CLK_RST_CONTROLLER_CLK_ENB_X_CLR, devices_to_clear_x);

        auto devices_to_clear_y = reg::Read(CLKRST + CLK_RST_CONTROLLER_CLK_OUT_ENB_Y);
        reg::ClearBits(static_cast<volatile u32 &>(devices_to_clear_y), CLK_RST_REG_BITS_MASK(CLK_ENB_Y_CLK_ENB_MC_CCPA),
                                                                        CLK_RST_REG_BITS_MASK(CLK_ENB_Y_CLK_ENB_MC_CDPA));
        reg::Write(CLKRST + CLK_RST_CONTROLLER_CLK_ENB_Y_CLR, devices_to_clear_y);

        /* If CH1 is enabled, enable clock to MC1. */
        if (reg::HasValue(EMC + EMC_FBIO_CFG7, EMC_REG_BITS_ENUM(FBIO_CFG7_CH1_ENABLE, ENABLE))) {
            reg::Write(CLKRST + CLK_RST_CONTROLLER_CLK_ENB_W_SET, CLK_RST_REG_BITS_ENUM(CLK_ENB_W_CLK_ENB_MC1, ENABLE));
        }
    }

}
