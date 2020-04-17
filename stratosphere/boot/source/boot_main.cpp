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
#include "boot_boot_reason.hpp"
#include "boot_change_voltage.hpp"
#include "boot_check_battery.hpp"
#include "boot_check_clock.hpp"
#include "boot_clock_initial_configuration.hpp"
#include "boot_fan_enable.hpp"
#include "boot_repair_boot_images.hpp"
#include "boot_splash_screen.hpp"
#include "boot_wake_pins.hpp"

#include "gpio/gpio_initial_configuration.hpp"
#include "pinmux/pinmux_initial_configuration.hpp"

#include "boot_power_utils.hpp"

using namespace ams;

extern "C" {
    extern u32 __start__;

    u32 __nx_applet_type = AppletType_None;

    /* TODO: Evaluate to what extent this can be reduced further. */
    #define INNER_HEAP_SIZE 0x20000
    size_t nx_inner_heap_size = INNER_HEAP_SIZE;
    char   nx_inner_heap[INNER_HEAP_SIZE];

    void __libnx_initheap(void);
    void __appInit(void);
    void __appExit(void);

    /* Exception handling. */
    alignas(16) u8 __nx_exception_stack[ams::os::MemoryPageSize];
    u64 __nx_exception_stack_size = sizeof(__nx_exception_stack);
    void __libnx_exception_handler(ThreadExceptionDump *ctx);
}

namespace ams {

    ncm::ProgramId CurrentProgramId = ncm::SystemProgramId::Boot;

    void ExceptionHandler(FatalErrorContext *ctx) {
        /* We're boot sysmodule, so manually reboot to fatal error. */
        boot::RebootForFatalError(ctx);
    }

    namespace result {

        bool CallFatalOnResultAssertion = false;

    }

}

using namespace ams;

void __libnx_exception_handler(ThreadExceptionDump *ctx) {
    ams::CrashHandler(ctx);
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
    hos::InitializeForStratosphere();

    /* Initialize services we need (TODO: NCM) */
    sm::DoWithSession([&]() {
        R_ABORT_UNLESS(fsInitialize());
        R_ABORT_UNLESS(splInitialize());
        R_ABORT_UNLESS(pmshellInitialize());
    });

    ams::CheckApiVersion();
}

void __appExit(void) {
    /* Cleanup services. */
    pmshellExit();
    splExit();
    fsExit();
}

int main(int argc, char **argv)
{
    /* Set thread name. */
    os::SetThreadNamePointer(os::GetCurrentThread(), AMS_GET_SYSTEM_THREAD_NAME(boot, Main));
    AMS_ASSERT(os::GetThreadPriority(os::GetCurrentThread()) == AMS_GET_SYSTEM_THREAD_PRIORITY(boot, Main));

    /* Change voltage from 3.3v to 1.8v for select devices. */
    boot::ChangeGpioVoltageTo1_8v();

    /* Setup GPIO. */
    gpio::SetInitialConfiguration();

    /* Check USB PLL/UTMIP clock. */
    boot::CheckClock();

    /* Talk to PMIC/RTC, set boot reason with SPL. */
    boot::DetectBootReason();

    const auto hw_type = spl::GetHardwareType();
    if (hw_type != spl::HardwareType::Copper && hw_type != spl::HardwareType::Calcio) {
        /* Display splash screen for two seconds. */
        boot::ShowSplashScreen();

        /* Check that the battery has enough to boot. */
        boot::CheckBatteryCharge();
    }

    /* Configure pinmux + drive pads. */
    pinmux::SetInitialConfiguration();

    /* Configure the PMC wake pin settings. */
    boot::SetInitialWakePinConfiguration();

    /* Configure output clock. */
    if (hw_type != spl::HardwareType::Copper && hw_type != spl::HardwareType::Calcio) {
        boot::SetInitialClockConfiguration();
    }

    /* Set Fan enable config (Copper only). */
    boot::SetFanEnabled();

    /* Repair boot partitions in NAND if needed. */
    boot::CheckAndRepairBootImages();

    /* Tell PM to start boot2. */
    R_ABORT_UNLESS(pmshellNotifyBootFinished());

    return 0;
}
