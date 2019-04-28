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

#include "ldr_process_manager.hpp"
#include "ldr_debug_monitor.hpp"
#include "ldr_shell.hpp"

extern "C" {
    extern u32 __start__;

    u32 __nx_applet_type = AppletType_None;

    #define INNER_HEAP_SIZE 0x30000
    size_t nx_inner_heap_size = INNER_HEAP_SIZE;
    char   nx_inner_heap[INNER_HEAP_SIZE];

    void __libnx_initheap(void);
    void __appInit(void);
    void __appExit(void);

    /* Exception handling. */
    alignas(16) u8 __nx_exception_stack[0x1000];
    u64 __nx_exception_stack_size = sizeof(__nx_exception_stack);
    void __libnx_exception_handler(ThreadExceptionDump *ctx);
    u64 __stratosphere_title_id = TitleId_Loader;
    void __libstratosphere_exception_handler(AtmosphereFatalErrorContext *ctx);
}

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
    Result rc;

    SetFirmwareVersionForLibnx();

    /* Initialize services we need (TODO: SPL) */
    DoWithSmSession([&]() {
        rc = fsInitialize();
        if (R_FAILED(rc)) {
            std::abort();
        }

        rc = lrInitialize();
        if (R_FAILED(rc))  {
            std::abort();
        }

        rc = fsldrInitialize();
        if (R_FAILED(rc))  {
            std::abort();
        }
    });


    CheckAtmosphereVersion(CURRENT_ATMOSPHERE_VERSION);
}

void __appExit(void) {
    /* Cleanup services. */
    fsdevUnmountAll();
    fsldrExit();
    lrExit();
    fsExit();
}

struct LoaderServerOptions {
    static constexpr size_t PointerBufferSize = 0x400;
    static constexpr size_t MaxDomains = 0;
    static constexpr size_t MaxDomainObjects = 0;
};

int main(int argc, char **argv)
{
    consoleDebugInit(debugDevice_SVC);

    static auto s_server_manager = WaitableManager<LoaderServerOptions>(1);

    /* Add services to manager. */
    s_server_manager.AddWaitable(new ServiceServer<ProcessManagerService>("ldr:pm", 1));
    s_server_manager.AddWaitable(new ServiceServer<ShellService>("ldr:shel", 3));
    s_server_manager.AddWaitable(new ServiceServer<DebugMonitorService>("ldr:dmnt", 2));
        
    /* Loop forever, servicing our services. */
    s_server_manager.Process();
    
	return 0;
}

