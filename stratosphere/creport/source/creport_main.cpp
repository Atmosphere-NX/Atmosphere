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
#include <cstdio>
#include <cstring>
#include <malloc.h>

#include <switch.h>
#include <stratosphere.hpp>
#include "creport_crash_report.hpp"
#include "creport_utils.hpp"


extern "C" {
    extern u32 __start__;

    u32 __nx_applet_type = AppletType_None;

    #define INNER_HEAP_SIZE 0x100000
    size_t nx_inner_heap_size = INNER_HEAP_SIZE;
    char   nx_inner_heap[INNER_HEAP_SIZE];

    void __libnx_initheap(void);
    void __appInit(void);
    void __appExit(void);

    /* Exception handling. */
    alignas(16) u8 __nx_exception_stack[0x1000];
    u64 __nx_exception_stack_size = sizeof(__nx_exception_stack);
    void __libnx_exception_handler(ThreadExceptionDump *ctx);
    void __libstratosphere_exception_handler(AtmosphereFatalErrorContext *ctx);
}

sts::ncm::TitleId __stratosphere_title_id = sts::ncm::TitleId::Creport;

void __libnx_exception_handler(ThreadExceptionDump *ctx) {
    StratosphereCrashHandler(ctx);
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
    SetFirmwareVersionForLibnx();

    DoWithSmSession([&]() {
        R_ASSERT(fsInitialize());
    });

    R_ASSERT(fsdevMountSdmc());
}

void __appExit(void) {
    /* Cleanup services. */
    fsdevUnmountAll();
    fsExit();
}

static sts::creport::CrashReport g_Creport;

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
    u64 crashed_pid = sts::creport::ParseProcessIdArgument(argv[0]);

    /* Try to debug the crashed process. */
    g_Creport.BuildReport(crashed_pid, argv[1][0] == '1');
    if (!g_Creport.IsComplete()) {
        return EXIT_FAILURE;
    }

    /* Save report to file. */
    g_Creport.SaveReport();

    /* Try to terminate the process. */
    {
        sts::sm::ScopedServiceHolder<nsdevInitialize, nsdevExit> ns_holder;
        if (ns_holder) {
            nsdevTerminateProcess(crashed_pid);
        }
    }

    /* Don't fatal if we have extra info, or if we're 5.0.0+ and an application crashed. */
    if (GetRuntimeFirmwareVersion() >= FirmwareVersion_500) {
        if (g_Creport.IsApplication()) {
            return EXIT_SUCCESS;
        }
    } else if (argv[1][0] == '1') {
        return EXIT_SUCCESS;
    }

    /* Also don't fatal if we're a user break. */
    if (g_Creport.IsUserBreak()) {
        return EXIT_SUCCESS;
    }

    /* Throw fatal error. */
    FatalContext ctx;
    g_Creport.GetFatalContext(&ctx);
    fatalWithContext(g_Creport.GetResult(), FatalType_ErrorScreen, &ctx);
}
