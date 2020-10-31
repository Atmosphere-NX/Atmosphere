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
#include <stratosphere.hpp>
#include "boot_boot_reason.hpp"
#include "boot_change_voltage.hpp"
#include "boot_check_battery.hpp"
#include "boot_check_clock.hpp"
#include "boot_clock_initial_configuration.hpp"
#include "boot_fan_enable.hpp"
#include "boot_repair_boot_images.hpp"
#include "boot_splash_screen.hpp"

#include "pinmux/pinmux_initial_configuration.hpp"

#include "boot_power_utils.hpp"

using namespace ams;

extern "C" {
    extern u32 __start__;

    u32 __nx_applet_type = AppletType_None;
    u32 __nx_fs_num_sessions = 1;

    /* TODO: Evaluate to what extent this can be reduced further. */
    #define INNER_HEAP_SIZE 0x1000
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

namespace {

    u8 g_exp_heap_memory[20_KB];
    u8 g_unit_heap_memory[5_KB];
    lmem::HeapHandle g_exp_heap_handle;
    lmem::HeapHandle g_unit_heap_handle;

    std::optional<sf::ExpHeapMemoryResource>  g_exp_heap_memory_resource;
    std::optional<sf::UnitHeapMemoryResource> g_unit_heap_memory_resource;

    void *Allocate(size_t size) {
        void *mem = lmem::AllocateFromExpHeap(g_exp_heap_handle, size);
        return mem;
    }

    void Deallocate(void *p, size_t size) {
        AMS_UNUSED(size);
        lmem::FreeToExpHeap(g_exp_heap_handle, p);
    }

    void InitializeHeaps() {
        /* Create the heaps. */
        g_exp_heap_handle  = lmem::CreateExpHeap(g_exp_heap_memory, sizeof(g_exp_heap_memory), lmem::CreateOption_ThreadSafe);
        g_unit_heap_handle = lmem::CreateUnitHeap(g_unit_heap_memory, sizeof(g_unit_heap_memory), sizeof(ddsf::DeviceCodeEntryHolder), lmem::CreateOption_ThreadSafe);

        /* Create the memory resources. */
        g_exp_heap_memory_resource.emplace(g_exp_heap_handle);
        g_unit_heap_memory_resource.emplace(g_unit_heap_handle);

        /* Register with ddsf. */
        ddsf::SetMemoryResource(std::addressof(*g_exp_heap_memory_resource));
        ddsf::SetDeviceCodeEntryHolderMemoryResource(std::addressof(*g_unit_heap_memory_resource));
    }

}

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

    InitializeHeaps();
}

void __appInit(void) {
    hos::InitializeForStratosphere();

    fs::SetAllocator(Allocate, Deallocate);

    /* Initialize services we need (TODO: NCM) */
    sm::DoWithSession([&]() {
        R_ABORT_UNLESS(fsInitialize());
        spl::Initialize();
        R_ABORT_UNLESS(pmshellInitialize());
    });

    ams::CheckApiVersion();
}

void __appExit(void) {
    /* Cleanup services. */
    pmshellExit();
    spl::Finalize();
    fsExit();
}

void *operator new(size_t size) {
    return Allocate(size);
}

void *operator new(size_t size, const std::nothrow_t &) {
    return Allocate(size);
}

void operator delete(void *p) {
    return Deallocate(p, 0);
}

void *operator new[](size_t size) {
    return Allocate(size);
}

void *operator new[](size_t size, const std::nothrow_t &) {
    return Allocate(size);
}

void operator delete[](void *p) {
    return Deallocate(p, 0);
}

int main(int argc, char **argv)
{
    /* Set thread name. */
    os::SetThreadNamePointer(os::GetCurrentThread(), AMS_GET_SYSTEM_THREAD_NAME(boot, Main));
    AMS_ASSERT(os::GetThreadPriority(os::GetCurrentThread()) == AMS_GET_SYSTEM_THREAD_PRIORITY(boot, Main));

    /* Perform atmosphere-specific init. */
    ams::InitializeForBoot();

    /* Set the reboot payload with ams.mitm. */
    boot::SetInitialRebootPayload();

    /* Change voltage from 3.3v to 1.8v for select devices. */
    boot::ChangeGpioVoltageTo1_8v();

    /* Setup GPIO. */
    gpio::driver::SetInitialGpioConfig();

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
    gpio::driver::SetInitialWakePinConfig();

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
