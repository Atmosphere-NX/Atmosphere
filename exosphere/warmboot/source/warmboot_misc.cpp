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
#include "warmboot_misc.hpp"

namespace ams::warmboot {

    namespace {

        constexpr inline const uintptr_t APB_MISC = secmon::MemoryRegionPhysicalDeviceApbMisc.GetAddress();
        constexpr inline const uintptr_t PMC      = secmon::MemoryRegionPhysicalDevicePmc.GetAddress();
        constexpr inline const uintptr_t SYSTEM   = secmon::MemoryRegionPhysicalDeviceSystem.GetAddress();

    }

    void ConfigureMiscSystemDebug() {
        /* Enable RTCK daisy-chaining. */
        reg::Write(APB_MISC + APB_MISC_PP_CONFIG_CTL, APB_MISC_REG_BITS_ENUM(PP_CONFIG_CTL_TBE, ENABLE));

        /* If we're in production mode, perform JTAG configuration. */
        /* NOTE: While this is what NVidia's code does, this is almost certainly a logic error. */
        /* They intend to configure JTAG only when *not* in production mode. */
        /* However, here we will do what they do, and not what they intend. */
        const bool production_mode = fuse::IsOdmProductionMode();
        if (production_mode) {
            const bool jtag_sts = reg::HasValue(PMC + APBDEV_PMC_STICKY_BITS, PMC_REG_BITS_ENUM(STICKY_BITS_JTAG_STS, ENABLE));

            reg::ReadWrite(SYSTEM + SB_PFCFG, SB_REG_BITS_ENUM_SEL(PFCFG_DBGEN,   jtag_sts, ENABLE, DISABLE),
                                              SB_REG_BITS_ENUM_SEL(PFCFG_SPNIDEN, jtag_sts, ENABLE, DISABLE),
                                              SB_REG_BITS_ENUM    (PFCFG_NIDEN,   ENABLE),
                                              SB_REG_BITS_ENUM    (PFCFG_SPIDEN,  DISABLE));

            reg::ReadWrite(APB_MISC + APB_MISC_PP_CONFIG_CTL, APB_MISC_REG_BITS_ENUM_SEL(PP_CONFIG_CTL_JTAG, jtag_sts, ENABLE, DISABLE));
        }

        /* Configure HDA codec disable. */
        reg::ReadWrite(PMC + APBDEV_PMC_STICKY_BITS, PMC_REG_BITS_ENUM_SEL(STICKY_BITS_HDA_LPBK_DIS, production_mode, ENABLE, DISABLE));

        /* Set E_INPUT in PINMUX_AUX_GPIO_PA6 (needed by the XUSB and SATA controllers). */
        reg::ReadWrite(APB_MISC + PINMUX_AUX_GPIO_PA6, PINMUX_REG_BITS_ENUM(AUX_E_INPUT, ENABLE));
    }

}
