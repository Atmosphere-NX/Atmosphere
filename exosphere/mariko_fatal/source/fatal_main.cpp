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
#include "fatal_sdmmc.hpp"

namespace ams::secmon::fatal {

    namespace {

        constinit u8 g_test_buffer[sdmmc::SectorSize * 2];

    }

    void Main() {
        /* Set library register addresses. */
        actmon::SetRegisterAddress(MemoryRegionVirtualDeviceActivityMonitor.GetAddress());
        clkrst::SetRegisterAddress(MemoryRegionVirtualDeviceClkRst.GetAddress());
          flow::SetRegisterAddress(MemoryRegionVirtualDeviceFlowController.GetAddress());
          fuse::SetRegisterAddress(MemoryRegionVirtualDeviceFuses.GetAddress());
           gic::SetRegisterAddress(MemoryRegionVirtualDeviceGicDistributor.GetAddress(), MemoryRegionVirtualDeviceGicCpuInterface.GetAddress());
           i2c::SetRegisterAddress(i2c::Port_1, MemoryRegionVirtualDeviceI2c1.GetAddress());
           i2c::SetRegisterAddress(i2c::Port_5, MemoryRegionVirtualDeviceI2c5.GetAddress());
        pinmux::SetRegisterAddress(MemoryRegionVirtualDeviceApbMisc.GetAddress(), MemoryRegionVirtualDeviceGpio.GetAddress());
           pmc::SetRegisterAddress(MemoryRegionVirtualDevicePmc.GetAddress());
            se::SetRegisterAddress(MemoryRegionVirtualDeviceSecurityEngine.GetAddress(), MemoryRegionVirtualDeviceSecurityEngine2.GetAddress());
          uart::SetRegisterAddress(MemoryRegionVirtualDeviceUart.GetAddress());
           wdt::SetRegisterAddress(MemoryRegionVirtualDeviceTimer.GetAddress());
          util::SetRegisterAddress(MemoryRegionVirtualDeviceTimer.GetAddress());

        /* Ensure that the log library is initialized. */
        log::Initialize();

        AMS_SECMON_LOG("%s\n", "Fatal start.");

        /* Initialize the sdmmc driver. */
        Result result = InitializeSdCard();
        AMS_SECMON_LOG("InitializeSdCard: %08x\n", result.GetValue());

        /* Get the connection status. */
        sdmmc::SpeedMode speed_mode;
        sdmmc::BusWidth bus_width;
        result = CheckSdCardConnection(std::addressof(speed_mode), std::addressof(bus_width));
        AMS_SECMON_LOG("CheckSdCardConnection: %08x\n", result.GetValue());
        AMS_SECMON_LOG("    Speed Mode: %u\n", static_cast<u32>(speed_mode));
        AMS_SECMON_LOG("    Bus Width:  %u\n", static_cast<u32>(bus_width));

        /* Read the first two sectors from the SD Card. */
        std::memset(g_test_buffer, 0xCC, sizeof(g_test_buffer));
        result = ReadSdCard(g_test_buffer, sizeof(g_test_buffer), 0, sizeof(g_test_buffer) / sdmmc::SectorSize);
        AMS_SECMON_LOG("ReadSdCard: %08x\n", result.GetValue());
        AMS_DUMP(g_test_buffer, sizeof(g_test_buffer));

        /* TODO */
        AMS_INFINITE_LOOP();
    }

}
