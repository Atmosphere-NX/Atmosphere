/*
 * Copyright (c) Atmosphère-NX
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
#include "boot_driver_management.hpp"
#include "boot_fan_enable.hpp"
#include "boot_pinmux_initial_configuration.hpp"
#include "boot_repair_boot_images.hpp"
#include "boot_splash_screen.hpp"
#include "boot_power_utils.hpp"

namespace ams {

    namespace boot {

        namespace {

            constinit u8 g_exp_heap_memory[20_KB];
            constinit u8 g_unit_heap_memory[5_KB];
            constinit lmem::HeapHandle g_exp_heap_handle;
            constinit lmem::HeapHandle g_unit_heap_handle;

            constinit sf::ExpHeapMemoryResource  g_exp_heap_memory_resource;
            constinit sf::UnitHeapMemoryResource g_unit_heap_memory_resource;

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

                /* Attach the memory resources. */
                g_exp_heap_memory_resource.Attach(g_exp_heap_handle);
                g_unit_heap_memory_resource.Attach(g_unit_heap_handle);

                /* Register with ddsf. */
                ddsf::SetMemoryResource(std::addressof(g_exp_heap_memory_resource));
                ddsf::SetDeviceCodeEntryHolderMemoryResource(std::addressof(g_unit_heap_memory_resource));
            }

        }

    }

    namespace hos {

        void SetNonApproximateVersionInternal();

    }

    namespace init {

        void InitializeSystemModule() {
            /* Initialize heaps. */
            boot::InitializeHeaps();

            /* Connect to sm. */
            R_ABORT_UNLESS(sm::Initialize());

            /* Initialize fs. */
            fs::InitializeForSystem();
            fs::SetAllocator(boot::Allocate, boot::Deallocate);
            fs::SetEnabledAutoAbort(false);

            /* Initialize spl. */
            spl::Initialize();

            /* Set the true hos version. */
            hos::SetNonApproximateVersionInternal();

            /* Verify that we can sanely execute. */
            ams::CheckApiVersion();
        }

        void FinalizeSystemModule() { /* ... */ }

        void Startup() { /* ... */ }

    }

    void ExceptionHandler(FatalErrorContext *ctx) {
        /* We're boot sysmodule, so manually reboot to fatal error. */
        boot::RebootForFatalError(ctx);
    }

    void Main() {
        /* Set thread name. */
        os::SetThreadNamePointer(os::GetCurrentThread(), AMS_GET_SYSTEM_THREAD_NAME(boot, Main));
        AMS_ASSERT(os::GetThreadPriority(os::GetCurrentThread()) == AMS_GET_SYSTEM_THREAD_PRIORITY(boot, Main));

        /* Perform atmosphere-specific init. */
        ams::InitializeForBoot();

        /* Set the reboot payload with ams.mitm. */
        boot::SetInitialRebootPayload();

        /* Change voltage from 3.3v to 1.8v for select devices. */
        boot::ChangeGpioVoltageTo1_8v();

        /* Setup gpio. */
        gpio::driver::SetInitialGpioConfig();

        /* Initialize the gpio server library. */
        boot::InitializeGpioDriverLibrary();

        /* Initialize the i2c server library. */
        boot::InitializeI2cDriverLibrary();

        /* Get the hardware type. */
        const auto hw_type = spl::GetHardwareType();

        /* Initialize the power control library without interrupt event handling. */
        if (hw_type != spl::HardwareType::Calcio) {
            powctl::Initialize(false);
        }

        /* Check USB PLL/UTMIP clock. */
        boot::CheckClock();

        /* Talk to PMIC/RTC, set boot reason with SPL. */
        boot::DetectBootReason();

        /* Display the splash screen and check the battery charge. */
        if (hw_type != spl::HardwareType::Calcio) {
            /* Display splash screen for two seconds. */
            boot::ShowSplashScreen();

            /* Check that the battery has enough to boot. */
            boot::CheckBatteryCharge();
        }

        /* Configure pinmux + drive pads. */
        boot::SetInitialPinmuxConfiguration();

        /* Configure the PMC wake pin settings. */
        gpio::driver::SetInitialWakePinConfig();

        /* Configure output clock. */
        if (hw_type != spl::HardwareType::Calcio) {
            boot::SetInitialClockConfiguration();
        }

        /* Set Fan enable config (Copper only). */
        boot::SetFanPowerEnabled();

        /* Repair boot partitions in NAND if needed. */
        boot::CheckAndRepairBootImages();

        /* Finalize the power control library. */
        if (hw_type != spl::HardwareType::Calcio) {
            powctl::Finalize();
        }

        /* Finalize the i2c server library. */
        boot::FinalizeI2cDriverLibrary();

        /* Finalize the gpio server library. */
        boot::FinalizeGpioDriverLibrary();

        /* Tell PM to start boot2. */
        R_ABORT_UNLESS(pmshellInitialize());
        R_ABORT_UNLESS(pmshellNotifyBootFinished());
    }

}

/* Override operator new. */
void *operator new(size_t size) {
    return ams::boot::Allocate(size);
}

void *operator new(size_t size, const std::nothrow_t &) {
    return ams::boot::Allocate(size);
}

void operator delete(void *p) {
    return ams::boot::Deallocate(p, 0);
}

void operator delete(void *p, size_t size) {
    return ams::boot::Deallocate(p, size);
}

void *operator new[](size_t size) {
    return ams::boot::Allocate(size);
}

void *operator new[](size_t size, const std::nothrow_t &) {
    return ams::boot::Allocate(size);
}

void operator delete[](void *p) {
    return ams::boot::Deallocate(p, 0);
}

void operator delete[](void *p, size_t size) {
    return ams::boot::Deallocate(p, size);
}
