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

#include "amsmitm_modules.hpp"
#include "utils.hpp"

extern "C" {
    extern u32 __start__;

    u32 __nx_applet_type = AppletType_None;

    #define INNER_HEAP_SIZE 0x1000000
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

sts::ncm::TitleId __stratosphere_title_id = sts::ncm::TitleId::AtmosphereMitm;

void __libnx_exception_handler(ThreadExceptionDump *ctx) {
    StratosphereCrashHandler(ctx);
}

void __libstratosphere_exception_handler(AtmosphereFatalErrorContext *ctx) {
    /* We're bpc-mitm (or ams_mitm, anyway), so manually reboot to fatal error. */
    Utils::RebootToFatalError(ctx);
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
        R_ASSERT(pmdmntInitialize());
        R_ASSERT(pminfoInitialize());
        R_ASSERT(splFsInitialize());
    });

    CheckAtmosphereVersion(CURRENT_ATMOSPHERE_VERSION);
}

void __appExit(void) {
    /* Cleanup services. */
    splFsExit();
    pminfoExit();
    pmdmntExit();
    fsExit();
}

int main(int argc, char **argv)
{
    consoleDebugInit(debugDevice_SVC);
    HosThread initializer_thread;

    LaunchAllMitmModules();

    R_ASSERT(initializer_thread.Initialize(&Utils::InitializeThreadFunc, NULL, 0x4000, 0x15));
    R_ASSERT(initializer_thread.Start());

    /* Wait for all mitm modules to end. */
    WaitAllMitmModules();

    return 0;
}

