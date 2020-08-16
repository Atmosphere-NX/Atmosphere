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

        constexpr inline const uintptr_t CLKRST   = secmon::MemoryRegionPhysicalDeviceClkRst.GetAddress();
        constexpr inline const uintptr_t PMC      = secmon::MemoryRegionPhysicalDevicePmc.GetAddress();
        constexpr inline const uintptr_t TIMER    = secmon::MemoryRegionPhysicalDeviceTimer.GetAddress();

    }

    void ConfigureOscillators() {
        /* Enable the crystal oscillator, and copy the drive strength from pmc. */
        const auto xofs = reg::GetValue(PMC + APBDEV_PMC_OSC_EDPD_OVER, PMC_REG_BITS_MASK(OSC_EDPD_OVER_XOFS));

        reg::ReadWrite(CLKRST + CLK_RST_CONTROLLER_OSC_CTRL, CLK_RST_REG_BITS_ENUM (OSC_CTRL_XOE, ENABLE),
                                                             CLK_RST_REG_BITS_VALUE(OSC_CTRL_XOFS,  xofs));

        /* Configure CLK_M_DIVISOR to 2. */
        reg::Write(CLKRST + CLK_RST_CONTROLLER_SPARE_REG0, CLK_RST_REG_BITS_ENUM(SPARE_REG0_CLK_M_DIVISOR, CLK_M_DIVISOR2));
        reg::Read(CLKRST + CLK_RST_CONTROLLER_SPARE_REG0);

        /* Restore TIMERUS config to 19.2 MHz. */
        reg::Write(TIMER + TIMERUS_USEC_CFG, TIMER_REG_BITS_VALUE(USEC_CFG_USEC_DIVIDEND,  5 - 1),
                                             TIMER_REG_BITS_VALUE(USEC_CFG_USEC_DIVISOR,  96 - 1));

    }

}
