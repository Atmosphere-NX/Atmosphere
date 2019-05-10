/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <malloc.h>

#include <switch.h>
#include <atmosphere.h>
#include <stratosphere.hpp>

#include "boot_functions.hpp"
#include "boot_reboot_manager.hpp"

extern "C" {
    extern u32 __start__;

    u32 __nx_applet_type = AppletType_None;

    #define INNER_HEAP_SIZE 0x200000
    size_t nx_inner_heap_size = INNER_HEAP_SIZE;
    char   nx_inner_heap[INNER_HEAP_SIZE];

    void __libnx_initheap(void);
    void __appInit(void);
    void __appExit(void);

    /* Exception handling. */
    alignas(16) u8 __nx_exception_stack[0x1000];
    u64 __nx_exception_stack_size = sizeof(__nx_exception_stack);
    void __libnx_exception_handler(ThreadExceptionDump *ctx);
    u64 __stratosphere_title_id = TitleId_Boot;
    void __libstratosphere_exception_handler(AtmosphereFatalErrorContext *ctx);
}

void __libnx_exception_handler(ThreadExceptionDump *ctx) {
    StratosphereCrashHandler(ctx);
}

void __libstratosphere_exception_handler(AtmosphereFatalErrorContext *ctx) {
    /* We're boot sysmodule, so manually reboot to fatal error. */
    BootRebootManager::RebootForFatalError(ctx);
}

void __libnx_initheap(void) {
    void*  addr = nx_inner_heap;
    size_t size = nx_inner_heap_size;

    /* Newlib */
    extern char* fake_heap_start;
    extern char* fake_heap_end;

    fake_heap_start = (char*)addr;
    fake_heap_end   = (char*)addr + size;
}

void __appInit(void) {
    Result rc;

    SetFirmwareVersionForLibnx();

    /* Initialize services we need (TODO: NCM) */
    DoWithSmSession([&]() {
        rc = fsInitialize();
        if (R_FAILED(rc)) {
            std::abort();
        }

        rc = splInitialize();
        if (R_FAILED(rc)) {
            std::abort();
        }

        rc = pmshellInitialize();
        if (R_FAILED(rc)) {
            std::abort();
        }
    });

    CheckAtmosphereVersion(CURRENT_ATMOSPHERE_VERSION);
}

void __appExit(void) {
    /* Cleanup services. */
    fsdevUnmountAll();
    pmshellExit();
    splExit();
    fsExit();
}

int main(int argc, char **argv)
{
    consoleDebugInit(debugDevice_SVC);

    /* Change voltage from 3.3v to 1.8v for select devices. */
    Boot::ChangeGpioVoltageTo1_8v();

    /* Setup GPIO. */
    Boot::SetInitialGpioConfiguration();

    /* Check USB PLL/UTMIP clock. */
    Boot::CheckClock();

    /* Talk to PMIC/RTC, set boot reason with SPL. */
    Boot::DetectBootReason();

    const HardwareType hw_type = Boot::GetHardwareType();
    if (hw_type != HardwareType_Copper) {
        /* Display splash screen for two seconds. */
        Boot::ShowSplashScreen();

        /* Check that the battery has enough to boot. */
        Boot::CheckBatteryCharge();
    }

    /* Configure pinmux + drive pads. */
    Boot::ConfigurePinmux();

    /* Configure the PMC wake pin settings. */
    Boot::SetInitialWakePinConfiguration();

    /* Configure output clock. */
    if (hw_type != HardwareType_Copper) {
        Boot::SetInitialClockConfiguration();
    }

    /* Set Fan enable config (Copper only). */
    Boot::SetFanEnabled();

    /* Repair boot partitions in NAND if needed. */
    Boot::CheckAndRepairBootImages();

    /* Tell PM to start boot2. */
    if (R_FAILED(pmshellNotifyBootFinished())) {
        std::abort();
    }

    return 0;
}
