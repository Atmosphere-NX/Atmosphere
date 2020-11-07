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

#pragma once

#include "reg_util.h"

namespace t210 {

    const struct CLK_RST_CONTROLLER {
        static const uintptr_t base_addr = 0x60006000;
        using Peripheral = CLK_RST_CONTROLLER;

        enum class OscFreq {
            OSC13 = 0,
            OSC16P8 = 1,
            OSC19P2 = 4,
            OSC38P4 = 5,
            OSC12 = 8,
            OSC48 = 9,
            OSC26 = 12,
        };

        BEGIN_DEFINE_REGISTER(RST_DEVICES_U_0, uint32_t, 0xc)
            DEFINE_RW_FIELD(SWR_XUSB_DEV_RST, 31)
            DEFINE_RW_FIELD(SWR_XUSB_HOST_RST, 25)
        END_DEFINE_REGISTER(RST_DEVICES_U_0)

        BEGIN_DEFINE_REGISTER(CLK_OUT_ENB_U_0, uint32_t, 0x18)
            DEFINE_RW_FIELD(CLK_ENB_XUSB_DEV, 31)
            DEFINE_RW_FIELD(CLK_ENB_XUSB_HOST, 25)
        END_DEFINE_REGISTER(CLK_OUT_ENB_U_0)
        
        BEGIN_DEFINE_REGISTER(OSC_CTRL_0, uint32_t, 0x50)
            DEFINE_RW_FIELD(OSC_FREQ, 28, 31, OscFreq)
        END_DEFINE_REGISTER(OSC_CTRL_0)

        BEGIN_DEFINE_REGISTER(PLLU_BASE_0, uint32_t, 0xc0)
            DEFINE_RW_FIELD(PLLU_BYPASS, 31)
            DEFINE_RW_FIELD(PLLU_ENABLE, 30)
            DEFINE_RW_FIELD(PLLU_REF_DIS, 29)
            DEFINE_RO_FIELD(PLLU_LOCK, 27)
            DEFINE_RW_FIELD(PLLU_CLKENABLE_48M, 25)
            DEFINE_RW_FIELD(PLLU_OVERRIDE, 24)
            DEFINE_RW_FIELD(PLLU_CLKENABLE_ICUSB, 23)
            DEFINE_RW_FIELD(PLLU_CLKENABLE_HSIC, 22)
            DEFINE_RW_FIELD(PLLU_CLKENABLE_USB, 21)
            DEFINE_RW_FIELD(PLLU_DIVP, 16, 20)
            DEFINE_RW_FIELD(PLLU_DIVN, 8, 15)
            DEFINE_RW_FIELD(PLLU_DIVM, 0, 7)
        END_DEFINE_REGISTER(PLLU_BASE_0)

        BEGIN_DEFINE_REGISTER(PLLU_OUTA_0, uint32_t, 0xc4)
            DEFINE_RW_FIELD(PLLU_OUT2_RATIO, 24, 21)
            DEFINE_RW_FIELD(PLLU_OUT2_OVRRIDE, 18)
            DEFINE_RW_FIELD(PLLU_OUT2_CLKEN, 17)
            DEFINE_RW_FIELD(PLLU_OUT2_RSTEN, 16)
        
            DEFINE_RW_FIELD(PLLU_OUT1_RATIO, 8, 15)
            DEFINE_RW_FIELD(PLLU_OUT1_OVRRIDE, 2)
            DEFINE_RW_FIELD(PLLU_OUT1_CLKEN, 1)
            DEFINE_RW_FIELD(PLLU_OUT1_RSTN, 0)
        END_DEFINE_REGISTER(PLLU_OUTA_0)
        
        BEGIN_DEFINE_REGISTER(PLLU_MISC_0, uint32_t, 0xcc)
            DEFINE_RW_FIELD(PLLU_IDDQ, 31)
            DEFINE_RO_FIELD(PLLU_FREQLOCK, 30)
            DEFINE_RW_FIELD(PLLU_EN_LCKDET, 29)
            BEGIN_DEFINE_RW_SYMBOLIC_FIELD(PLLU_PTS, 27, 28)
                DEFINE_RW_SYMBOLIC_VALUE(DISABLE, 0)
                DEFINE_RW_SYMBOLIC_VALUE(VCO, 1)
                DEFINE_RW_SYMBOLIC_VALUE(FO, 2)
                DEFINE_RW_SYMBOLIC_VALUE(FO_ICUSB, 3)
            END_DEFINE_SYMBOLIC_FIELD(PLLU_PTS)
            DEFINE_RW_FIELD(PLLU_KCP, 25, 26)
            DEFINE_RW_FIELD(PLLU_KVCO, 24)
            DEFINE_RW_FIELD(PLLU_SETUP, 0, 23)
        END_DEFINE_REGISTER(PLLU_MISC_0)

        BEGIN_DEFINE_REGISTER(CLK_OUT_ENB_Y_0, uint32_t, 0x298)
            DEFINE_RW_FIELD(CLK_ENB_USB2_TRK, 18)
        END_DEFINE_REGISTER(CLK_OUT_ENB_Y_0)
        
        BEGIN_DEFINE_REGISTER(RST_DEVICES_W_0, uint32_t, 0x35c)
            DEFINE_RW_FIELD(SWR_XUSB_SS_RST, 28)
            DEFINE_RW_FIELD(SWR_DVFS_RST, 27)
            DEFINE_RW_FIELD(SWR_ENTROPY_RST, 21)
            DEFINE_RW_FIELD(SWR_XUSB_PADCTL_RST, 14)
            DEFINE_RW_FIELD(SWR_RESERVED10_RST, 13)
            DEFINE_RW_FIELD(SWR_RESERVED9_RST, 12)
            // SWR_RESERVED8_RST conspicuously missing from TRM
            DEFINE_RW_FIELD(SWR_RESERVED7_RST, 10)
            DEFINE_RW_FIELD(SWR_RESERVED6_RST, 9)
            DEFINE_RW_FIELD(SWR_CEC_RST, 8)
            DEFINE_RW_FIELD(SWR_RESERVED5_RST, 7)
            DEFINE_RW_FIELD(SWR_RESERVED4_RST, 6)
            DEFINE_RW_FIELD(SWR_RESERVED3_RST, 5)
            DEFINE_RW_FIELD(SWR_RESERVED2_RST, 4)
            DEFINE_RW_FIELD(SWR_RESERVED1_RST, 3)
            DEFINE_RW_FIELD(SWR_RESERVED0_RST, 2)
            DEFINE_RW_FIELD(SWR_SATACOLD_RST, 1)
        END_DEFINE_REGISTER(RST_DEVICES_W_0)

        BEGIN_DEFINE_REGISTER(CLK_OUT_ENB_W_0, uint32_t, 0x364)
            DEFINE_RW_FIELD(CLK_ENB_XUSB_SS, 28)
            DEFINE_RW_FIELD(CLK_ENB_XUSB, 15)
        END_DEFINE_REGISTER(CLK_OUT_ENB_W_0)

        BEGIN_DEFINE_REGISTER(UTMIP_PLL_CFG0_0, uint32_t, 0x480)
            DEFINE_RW_FIELD(UTMIP_PLL_VREG_CTRL, 27, 28)
            DEFINE_RW_FIELD(UTMIP_PLL_KCP, 25, 26)
            DEFINE_RW_FIELD(UTMIP_PLL_NDIV, 16, 23)
            DEFINE_RW_FIELD(UTMIP_PLL_MDIV, 8, 15)
            DEFINE_RW_FIELD(UTMIP_PLL_LOCK_OVR, 2)
            DEFINE_RW_FIELD(UTMIP_PLL_RESERVED2, 1)
            DEFINE_RW_FIELD(UTMIP_PLL_EN_LCKDET, 0)
        END_DEFINE_REGISTER(UTMIP_PLL_CFG0_0)

        BEGIN_DEFINE_REGISTER(UTMIP_PLL_CFG1_0, uint32_t, 0x484)
            DEFINE_RW_FIELD(UTMIP_PLLU_ENABLE_DLY_COUNT, 27, 31)
            DEFINE_RW_FIELD(UTMIP_PLL_RSVD, 18, 26)
            DEFINE_RW_FIELD(UTMIP_FORCE_PLLU_POWERUP, 17)
            DEFINE_RW_FIELD(UTMIP_FORCE_PLLU_POWERDOWN, 16)
            DEFINE_RW_FIELD(UTMIP_FORCE_PLL_ENABLE_POWERUP, 15)
            DEFINE_RW_FIELD(UTMIP_FORCE_PLL_ENABLE_POWERDOWN, 14)
            DEFINE_RW_FIELD(UTMIP_FORCE_PLL_ACTIVE_POWERUP, 13)
            DEFINE_RW_FIELD(UTMIP_FORCE_PLL_ACTIVE_POWERDOWN, 12)
            DEFINE_RW_FIELD(UTMIP_XTAL_FREQ_COUNT, 0, 11)
        END_DEFINE_REGISTER(UTMIP_PLL_CFG1_0)

        BEGIN_DEFINE_REGISTER(UTMIP_PLL_CFG2_0, uint32_t, 0x488)
            DEFINE_RW_FIELD(UTMIP_PHY_XTAL_CLOCKEN, 30)
            DEFINE_RW_FIELD(UTMIP_FORCE_PD_RESERVED, 26, 29)
            DEFINE_RW_FIELD(UTMIP_FORCE_PD_SAMP_D_POWERUP, 25)
            DEFINE_RW_FIELD(UTMIP_FORCE_PD_SAMP_D_POWERDOWN, 24)
            DEFINE_RW_FIELD(UTMIP_PLL_ACTIVE_DLY_COUNT, 18, 23)
            DEFINE_RW_FIELD(UTMIP_PLLU_STABLE_COUNT, 6, 17)
            DEFINE_RW_FIELD(UTMIP_FORCE_PD_SAMP_C_POWERUP, 5)
            DEFINE_RW_FIELD(UTMIP_FORCE_PD_SAMP_C_POWERDOWN, 4)
            DEFINE_RW_FIELD(UTMIP_FORCE_PD_SAMP_B_POWERUP, 3)
            DEFINE_RW_FIELD(UTMIP_FORCE_PD_SAMP_B_POWERDOWN, 2)
            DEFINE_RW_FIELD(UTMIP_FORCE_PD_SAMP_A_POWERUP, 1)
            DEFINE_RW_FIELD(UTMIP_FORCE_PD_SAMP_A_POWERDOWN, 0)
        END_DEFINE_REGISTER(UTMIP_PLL_CFG2_0)

        BEGIN_DEFINE_REGISTER(UTMIP_PLL_CFG3_0, uint32_t, 0x4c0)
            DEFINE_RW_FIELD(UTMIP_PLL_REF_SRC_SEL, 26)
            DEFINE_RW_FIELD(UTMIP_PLL_REF_DIS, 25)
            DEFINE_RW_FIELD(UTMIP_PLL_PTS, 24)
            DEFINE_RW_FIELD(UTMIP_PLL_SETUP, 0, 23) // "Debug control bits."
        END_DEFINE_REGISTER(UTMIP_PLL_CFG3_0)
        
        BEGIN_DEFINE_REGISTER(UTMIPLL_HW_PWRDN_CFG0_0, uint32_t, 0x52c)
            DEFINE_RO_FIELD(UTMIPLL_LOCK, 31)
            DEFINE_RO_FIELD(UTMIPLL_SEQ_STATE, 26, 27)
            DEFINE_RW_FIELD(UTMIPLL_SEQ_START_STATE, 25)
            DEFINE_RW_FIELD(UTMIPLL_SEQ_ENABLE, 24)
            DEFINE_RW_FIELD(UTMIPLL_IDDQ_PD_INCLUDE, 7)
            DEFINE_RW_FIELD(UTMIPLL_USE_LOCKDET, 6)
            DEFINE_RW_FIELD(UTMIPLL_SEQ_RESET_INPUT_VALUE, 5)
            DEFINE_RW_FIELD(UTMIPLL_SEQ_IN_SWCTL, 4)
            DEFINE_RW_FIELD(UTMIPLL_CLK_ENABLE_OVERRIDE_VALUE, 3)
            DEFINE_RW_FIELD(UTMIPLL_CLK_ENABLE_SWCTL, 2)
            DEFINE_RW_FIELD(UTMIPLL_IDDQ_OVERRIDE_VALUE, 1)
            DEFINE_RW_FIELD(UTMIPLL_IDDQ_SWCTL, 0)
        END_DEFINE_REGISTER(UTMIPLL_HW_PWRDN_CFG0_0)

        BEGIN_DEFINE_REGISTER(CLK_SOURCE_XUSB_FS_0, uint32_t, 0x608)
            BEGIN_DEFINE_RW_SYMBOLIC_FIELD(XUSB_FS_CLK_SRC, 29, 31)
                DEFINE_RW_SYMBOLIC_VALUE(CLK_M, 0)
                DEFINE_RW_SYMBOLIC_VALUE(FO_48M, 2)
                DEFINE_RW_SYMBOLIC_VALUE(PLLP_OUT0, 4)
                DEFINE_RW_SYMBOLIC_VALUE(HSIC_480, 6)
            END_DEFINE_SYMBOLIC_FIELD(XUSB_FS_CLK_SRC)
        
            DEFINE_RW_FIELD(XUSB_FS_CLK_DIVISOR, 0, 7)
        END_DEFINE_REGISTER(CLK_SOURCE_XUSB_FS_0)
        
        BEGIN_DEFINE_REGISTER(CLK_SOURCE_XUSB_CORE_DEV_0, uint32_t, 0x60c)
            BEGIN_DEFINE_RW_SYMBOLIC_FIELD(XUSB_CORE_DEV_CLK_SRC, 29, 31)
                DEFINE_RW_SYMBOLIC_VALUE(CLK_M, 0)
                DEFINE_RW_SYMBOLIC_VALUE(PLLP_OUT0, 1)
                DEFINE_RW_SYMBOLIC_VALUE(PLLREFE_CLKOUT, 5)
            END_DEFINE_SYMBOLIC_FIELD(XUSB_CORE_DEV_CLK_SRC)
        
            DEFINE_RW_FIELD(XUSB_CORE_DEV_CLK_DIVISOR, 0, 7)
        END_DEFINE_REGISTER(CLK_SOURCE_XUSB_CORE_DEV_0)

        BEGIN_DEFINE_REGISTER(CLK_SOURCE_XUSB_SS_0, uint32_t, 0x610)
            BEGIN_DEFINE_RW_SYMBOLIC_FIELD(XUSB_SS_CLK_SRC, 29, 31)
                DEFINE_RW_SYMBOLIC_VALUE(CLK_M, 0)
                DEFINE_RW_SYMBOLIC_VALUE(PLLREFE_CLKOUT, 1)
                DEFINE_RW_SYMBOLIC_VALUE(CLK_S, 2)
                DEFINE_RW_SYMBOLIC_VALUE(HSIC_480, 3)
            END_DEFINE_SYMBOLIC_FIELD(XUSB_SS_CLK_SRC)

            DEFINE_RW_FIELD(XUSB_HS_HSICP_CLK_SEL, 26)
            DEFINE_RW_FIELD(XUSB_HS_CLK_BYPASS_SWITCH, 25)
            DEFINE_RW_FIELD(XUSB_SS_CLK_BYPASS_SWITCH, 24)
            DEFINE_RW_FIELD(XUSB_SS_CLK_DIVISOR, 0, 7)
        END_DEFINE_REGISTER(CLK_SOURCE_XUSB_SS_0)

        BEGIN_DEFINE_REGISTER(CLK_SOURCE_USB2_HSIC_TRK_0, uint32_t, 0x6cc)
            DEFINE_RW_FIELD(USB2_HSIC_TRK_CLK_DIVISOR, 0, 7)
        END_DEFINE_REGISTER(CLK_SOURCE_USB2_HSIC_TRK_0)
    } CLK_RST_CONTROLLER;
    
} // namespace t210