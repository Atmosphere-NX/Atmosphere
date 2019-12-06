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
#include "creport_crash_report.hpp"
#include "creport_utils.hpp"


extern "C" {
    extern u32 __start__;

    u32 __nx_applet_type = AppletType_None;
    u32 __nx_fs_num_sessions = 1;
    u32 __nx_fsdev_direntry_cache_size = 1;

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

    ncm::ProgramId CurrentProgramId = ncm::ProgramId::Creport;

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
    hos::SetVersionForLibnx();

    sm::DoWithSession([&]() {
        R_ASSERT(fsInitialize());
    });

    R_ASSERT(fsdevMountSdmc());
}

void __appExit(void) {
    /* Cleanup services. */
    fsdevUnmountAll();
    fsExit();
}

static creport::CrashReport g_crash_report;

int main(int argc, char **argv) {
    /* Validate arguments. */
    if (argc < 2) {
        return EXIT_FAILURE;
    }
    for (int i = 0; i < argc; i++) {
        if (argv[i] == NULL) {
            return EXIT_FAILURE;
        }
    }

    /* Parse crashed PID. */
    os::ProcessId crashed_pid = creport::ParseProcessIdArgument(argv[0]);

    /* Try to debug the crashed process. */
    g_crash_report.BuildReport(crashed_pid, argv[1][0] == '1');
    if (!g_crash_report.IsComplete()) {
        return EXIT_FAILURE;
    }

    /* Save report to file. */
    g_crash_report.SaveReport();

    /* Try to terminate the process. */
    {
        sm::ScopedServiceHolder<nsdevInitialize, nsdevExit> ns_holder;
        if (ns_holder) {
            nsdevTerminateProcess(static_cast<u64>(crashed_pid));
        }
    }

    /* Don't fatal if we have extra info, or if we're 5.0.0+ and an application crashed. */
    if (hos::GetVersion() >= hos::Version_500) {
        if (g_crash_report.IsApplication()) {
            return EXIT_SUCCESS;
        }
    } else if (argv[1][0] == '1') {
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
