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
#include "secmon_boot.hpp"
#include "secmon_boot_functions.hpp"
#include "../secmon_setup.hpp"
#include "../secmon_misc.hpp"

namespace ams::secmon {

    void Main() {
        /* Set library register addresses. */
        /*   actmon::SetRegisterAddress(MemoryRegionVirtualDeviceActivityMonitor.GetAddress()); */
             clkrst::SetRegisterAddress(MemoryRegionVirtualDeviceClkRst.GetAddress());
        /* flowctrl::SetRegisterAddress(); */
               fuse::SetRegisterAddress(MemoryRegionVirtualDeviceFuses.GetAddress());
                gic::SetRegisterAddress(MemoryRegionVirtualDeviceGicDistributor.GetAddress(), MemoryRegionVirtualDeviceGicCpuInterface.GetAddress());
                i2c::SetRegisterAddress(i2c::Port_1, MemoryRegionVirtualDeviceI2c1.GetAddress());
                i2c::SetRegisterAddress(i2c::Port_5, MemoryRegionVirtualDeviceI2c5.GetAddress());
        /*   pinmux::SetRegisterAddress(); */
                pmc::SetRegisterAddress(MemoryRegionVirtualDevicePmc.GetAddress());
                 se::SetRegisterAddress(MemoryRegionVirtualDeviceSecurityEngine.GetAddress());
               uart::SetRegisterAddress(MemoryRegionVirtualDeviceUart.GetAddress());
                wdt::SetRegisterAddress(MemoryRegionVirtualDeviceTimer.GetAddress());
               util::SetRegisterAddress(MemoryRegionVirtualDeviceTimer.GetAddress());

        /* Get the secure monitor parameters. */
        auto &secmon_params = *reinterpret_cast<pkg1::SecureMonitorParameters *>(MemoryRegionVirtualDeviceBootloaderParams.GetAddress());

        /* Perform initialization. */
        {
            /* Perform initial setup. */
            /* This checks the security engine's validity, and configures common interrupts in the GIC. */
            /* This also initializes the global configuration context. */
            secmon::Setup1();

            /* Save the boot info. */
            secmon::SaveBootInfo(secmon_params);

            /* Perform cold-boot specific init. */
            secmon::boot::InitializeColdBoot();

            /* Setup the SoC security measures. */
            secmon::SetupSocSecurity();

            /* TODO: More init. */

            /* Clear the crt0 code that was present in iram. */
            secmon::boot::ClearIram();

            /* Alert the bootloader that we're initialized. */
            secmon_params.secmon_state = pkg1::SecureMonitorState_Initialized;
        }
    }

}