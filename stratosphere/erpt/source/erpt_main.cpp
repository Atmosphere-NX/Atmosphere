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

namespace ams {

    namespace erpt {

        namespace {

            constexpr size_t MemoryHeapSize = 196_KB;
            alignas(os::MemoryPageSize) u8 g_memory_heap[MemoryHeapSize];

        }

    }

    namespace init {

        void InitializeSystemModule() {
            /* Initialize our connection to sm. */
            R_ABORT_UNLESS(sm::Initialize());

            /* Initialize services we need (which won't be initialized later). */
            R_ABORT_UNLESS(setInitialize());
            R_ABORT_UNLESS(setsysInitialize());
            R_ABORT_UNLESS(pscmInitialize());
            R_ABORT_UNLESS(time::Initialize());
            if (hos::GetVersion() >= hos::Version_11_0_0) {
                R_ABORT_UNLESS(ectxrInitialize());
            }

            /* Verify that we can sanely execute. */
            ams::CheckApiVersion();
        }

        void FinalizeSystemModule() { /* ... */ }

        void Startup() { /* ... */ }

    }

    void Main() {
        /* Set thread name. */
        os::SetThreadNamePointer(os::GetCurrentThread(), AMS_GET_SYSTEM_THREAD_NAME(erpt, Main));
        AMS_ASSERT(os::GetThreadPriority(os::GetCurrentThread()) == AMS_GET_SYSTEM_THREAD_PRIORITY(erpt, Main));

        /* Decide whether or not to clean up reports periodically. */
        {
            u8 disable_report_cleanup = 0;
            if (settings::fwdbg::GetSettingsItemValue(std::addressof(disable_report_cleanup), sizeof(disable_report_cleanup), "erpt", "disable_automatic_report_cleanup") == sizeof(disable_report_cleanup)) {
                erpt::srv::SetEnabledAutomaticReportCleanup(disable_report_cleanup == 0);
            } else {
                erpt::srv::SetEnabledAutomaticReportCleanup(true);
            }
        }

        /* Set the memory heap for erpt::srv namespace, perform other service init/etc. */
        R_ABORT_UNLESS(erpt::srv::Initialize(erpt::g_memory_heap, erpt::MemoryHeapSize));

        /* Atmosphere always wants to redirect new reports to the SD card, to prevent them from being logged. */
        erpt::srv::SetRedirectNewReportsToSdCard(true);

        /* Configure the serial number, OS version, product model, and region. */
        {
            const auto &sys_info = erpt::srv::GetSystemInfo();

            settings::system::SerialNumber serial_number = {};
            settings::system::GetSerialNumber(std::addressof(serial_number));

            R_ABORT_UNLESS(erpt::srv::SetSerialNumber(serial_number.str,
                                                      strnlen(serial_number.str, sizeof(serial_number.str) - 1) + 1));

            R_ABORT_UNLESS(erpt::srv::SetProductModel(sys_info.product_model, static_cast<u32>(std::strlen(sys_info.product_model))));

            R_ABORT_UNLESS(erpt::srv::SetRegionSetting(sys_info.region, static_cast<u32>(std::strlen(sys_info.region))));
        }

        /* Start the erpt server. */
        R_ABORT_UNLESS(erpt::srv::InitializeAndStartService());

        /* Launch sprofile on 13.0.0+ */
        if (hos::GetVersion() >= hos::Version_13_0_0) {
            /* Initialize the sprofile server. */
            sprofile::srv::Initialize();

            /* Start the sprofile ipc server. */
            sprofile::srv::StartIpcServer();
        }

        /* Wait forever. */
        erpt::srv::Wait();
    }

}
