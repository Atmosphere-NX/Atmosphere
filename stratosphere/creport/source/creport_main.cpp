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
#include "creport_crash_report.hpp"
#include "creport_utils.hpp"

namespace ams {

    namespace creport {

        namespace {

            constinit u8 g_fs_heap_memory[4_KB];
            lmem::HeapHandle g_fs_heap_handle;

            void *AllocateForFs(size_t size) {
                return lmem::AllocateFromExpHeap(g_fs_heap_handle, size);
            }

            void DeallocateForFs(void *p, size_t size) {
                AMS_UNUSED(size);
                return lmem::FreeToExpHeap(g_fs_heap_handle, p);
            }

            void InitializeFsHeap() {
                g_fs_heap_handle = lmem::CreateExpHeap(g_fs_heap_memory, sizeof(g_fs_heap_memory), lmem::CreateOption_None);
            }

        }

    }

    namespace init {

        void InitializeSystemModule() {
            /* Initialize heap. */
            creport::InitializeFsHeap();

            /* Initialize our connection to sm. */
            R_ABORT_UNLESS(sm::Initialize());

            /* Initialize fs. */
            fs::InitializeForSystem();
            fs::SetAllocator(creport::AllocateForFs, creport::DeallocateForFs);
            fs::SetEnabledAutoAbort(false);

            /* Mount the SD card. */
            R_ABORT_UNLESS(fs::MountSdCard("sdmc"));
        }

        void FinalizeSystemModule() { /* ... */ }

        void Startup() { /* ... */ }

    }

    namespace {

        constinit creport::CrashReport g_crash_report;

    }

    void Main() {
        /* Set thread name. */
        os::SetThreadNamePointer(os::GetCurrentThread(), AMS_GET_SYSTEM_THREAD_NAME(creport, Main));
        AMS_ASSERT(os::GetThreadPriority(os::GetCurrentThread()) == AMS_GET_SYSTEM_THREAD_PRIORITY(creport, Main));

        /* Get arguments. */
        const int num_args = os::GetHostArgc();
        char ** const args = os::GetHostArgv();

        /* Validate arguments. */
        if (num_args < 2) {
            return;
        }

        for (auto i = 0; i < num_args; ++i) {
            if (args[i] == nullptr) {
                return;
            }
        }

        /* Parse arguments. */
        const os::ProcessId crashed_pid = creport::ParseProcessIdArgument(args[0]);
        const bool has_extra_info       = args[1][0] == '1';
        const bool enable_screenshot    = num_args >= 3 && args[2][0] == '1';
        const bool enable_jit_debug     = num_args >= 4 && args[3][0] == '1';

        /* Initialize the crash report. */
        g_crash_report.Initialize();

        /* Try to debug the crashed process. */
        {
            g_crash_report.BuildReport(crashed_pid, has_extra_info);
            ON_SCOPE_EXIT { if (g_crash_report.IsOpen()) { g_crash_report.Close(); } };

            if (!g_crash_report.IsComplete()) {
                return;
            }

            /* Save report to file. */
            g_crash_report.SaveReport(enable_screenshot);
        }

        /* Try to terminate the process, if we should. */
        const auto fw_ver = hos::GetVersion();
        if (fw_ver < hos::Version_11_0_0 || !enable_jit_debug) {
            if (fw_ver >= hos::Version_10_0_0) {
                /* Use pgl to terminate. */
                if (R_SUCCEEDED(pgl::Initialize())) {
                    ON_SCOPE_EXIT { pgl::Finalize(); };

                    pgl::TerminateProcess(crashed_pid);
                }
            } else {
                /* Use ns to terminate. */
                if (R_SUCCEEDED(::nsdevInitialize())) {
                    ON_SCOPE_EXIT { ::nsdevExit(); };

                    nsdevTerminateProcess(crashed_pid.value);
                }
            }
        }

        /* If we're on 5.0.0+ and an application crashed, or if we have extra info, we don't need to fatal. */
        if (fw_ver >= hos::Version_5_0_0) {
            if (g_crash_report.IsApplication()) {
                return;
            }
        } else if (has_extra_info) {
            return;
        }

        /* We also don't need to fatal on user break. */
        if (g_crash_report.IsUserBreak()) {
            return;
        }

        /* Throw fatal error. */
        {
            ::FatalCpuContext ctx;
            g_crash_report.GetFatalContext(std::addressof(ctx));
            fatalThrowWithContext(g_crash_report.GetResult().GetValue(), FatalPolicy_ErrorScreen, std::addressof(ctx));
        }
    }

}
