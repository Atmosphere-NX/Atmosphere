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
#include "fatal_display.hpp"

namespace ams::secmon::fatal {

    namespace {

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

        /* Display the fatal error. */
        {
            InitializeDisplay();
            ShowDisplay(f_ctx, result);
            FinalizeDisplay();
        }

        /* Ensure we have nothing waiting to be logged. */
        AMS_LOG_FLUSH();

        /* TODO: Wait for a button press, then reboot. */
        AMS_INFINITE_LOOP();
    }

}
