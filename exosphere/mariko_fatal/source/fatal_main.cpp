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
#include "fatal_save_context.hpp"
#include "fatal_sound.hpp"
#include "fatal_display.hpp"

namespace ams::secmon::fatal {

    namespace {

        constexpr inline int I2cAddressMax77620Pmic = 0x3C;

        ALWAYS_INLINE const ams::impl::FatalErrorContext *GetFatalErrorContext() {
            return MemoryRegionVirtualTzramMarikoProgramFatalErrorContext.GetPointer<ams::impl::FatalErrorContext>();
        }

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

        /* Save the fatal error context. */
        const auto *f_ctx = GetFatalErrorContext();
        Result result = SaveFatalErrorContext(f_ctx);
        if (R_SUCCEEDED(result)) {
            AMS_SECMON_LOG("Saved fatal error context to /atmosphere/fatal_reports/report_%016" PRIx64 ".bin!\n", f_ctx->report_identifier);
        } else {
            AMS_SECMON_LOG("Failed to save fatal error context: %08x\n", result.GetValue());
        }

        /* Ensure that i2c-1/i2c-5 are usable for communicating with the audio device/pmic. */
        clkrst::EnableI2c1Clock();
        clkrst::EnableI2c5Clock();
        i2c::Initialize(i2c::Port_1);
        i2c::Initialize(i2c::Port_5);

        /* Shut down audio. */
        {
            StopSound();
        }

        /* Display the fatal error. */
        {
            AMS_SECMON_LOG("Showing Display, LCD Vendor = %04x\n", GetLcdVendor());
            InitializeDisplay();
            ShowDisplay(f_ctx, result);
        }

        /* Ensure we have nothing waiting to be logged. */
        AMS_LOG_FLUSH();

        /* Wait for power button to be pressed. */
        while (!pmic::IsPowerButtonPressed()) {
            util::WaitMicroSeconds(100);
        }

        /* Reboot. */
        pmic::ShutdownSystem(true);

        /* Wait for our reboot to complete. */
        AMS_INFINITE_LOOP();
    }

}
