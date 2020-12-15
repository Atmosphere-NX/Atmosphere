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
#include "creport_crash_report.hpp"
#include "creport_utils.hpp"


extern "C" {
    extern u32 __start__;

    u32 __nx_applet_type = AppletType_None;
    u32 __nx_fs_num_sessions = 1;

    #define INNER_HEAP_SIZE 0x4000
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

    ncm::ProgramId CurrentProgramId = ncm::SystemProgramId::Creport;

    namespace result {

        bool CallFatalOnResultAssertion = true;

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

    sm::DoWithSession([&]() {
        R_ABORT_UNLESS(fsInitialize());
    });

    R_ABORT_UNLESS(fs::MountSdCard("sdmc"));
}

void __appExit(void) {
    /* Cleanup services. */
    fsExit();
}

static creport::CrashReport g_crash_report;

int main(int argc, char **argv) {
    /* Set thread name. */
    os::SetThreadNamePointer(os::GetCurrentThread(), AMS_GET_SYSTEM_THREAD_NAME(creport, Main));
    AMS_ASSERT(os::GetThreadPriority(os::GetCurrentThread()) == AMS_GET_SYSTEM_THREAD_PRIORITY(creport, Main));

    /* Validate arguments. */
    if (argc < 2) {
        return EXIT_FAILURE;
    }
    for (int i = 0; i < argc; i++) {
        if (argv[i] == NULL) {
            return EXIT_FAILURE;
        }
    }

    /* Parse arguments. */
    const os::ProcessId crashed_pid = creport::ParseProcessIdArgument(argv[0]);
    const bool has_extra_info       = argv[1][0] == '1';
    const bool enable_screenshot    = argc >= 3 && argv[2][0] == '1';
    const bool enable_jit_debug     = argc >= 4 && argv[3][0] == '1';

    /* Initialize the crash report. */
    g_crash_report.Initialize();

    /* Try to debug the crashed process. */
    g_crash_report.BuildReport(crashed_pid, has_extra_info);
    if (!g_crash_report.IsComplete()) {
        return EXIT_FAILURE;
    }

    /* Save report to file. */
    g_crash_report.SaveReport(enable_screenshot);

    /* If we should, try to terminate the process. */
    if (hos::GetVersion() < hos::Version_11_0_0 || !enable_jit_debug) {
        if (hos::GetVersion() >= hos::Version_10_0_0) {
            /* On 10.0.0+, use pgl to terminate. */
            sm::ScopedServiceHolder<pgl::Initialize, pgl::Finalize> pgl_holder;
            if (pgl_holder) {
                pgl::TerminateProcess(crashed_pid);
            }
        } else {
            /* On < 10.0.0, use ns:dev to terminate. */
            sm::ScopedServiceHolder<nsdevInitialize, nsdevExit> ns_holder;
            if (ns_holder) {
                nsdevTerminateProcess(static_cast<u64>(crashed_pid));
            }
        }
    }

    /* Don't fatal if we have extra info, or if we're 5.0.0+ and an application crashed. */
    if (hos::GetVersion() >= hos::Version_5_0_0) {
        if (g_crash_report.IsApplication()) {
            return EXIT_SUCCESS;
        }
    } else if (has_extra_info) {
        return EXIT_SUCCESS;
    }

    /* Also don't fatal if we're a user break. */
    if (g_crash_report.IsUserBreak()) {
        return EXIT_SUCCESS;
    }

    /* Throw fatal error. */
    ::FatalCpuContext ctx;
    g_crash_report.GetFatalContext(&ctx);
    fatalThrowWithContext(g_crash_report.GetResult().GetValue(), FatalPolicy_ErrorScreen, &ctx);
}
