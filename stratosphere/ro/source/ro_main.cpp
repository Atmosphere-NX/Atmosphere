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

#include "ro_debug_monitor.hpp"
#include "ro_service.hpp"
#include "ro_registration.hpp"

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
    u64 __stratosphere_title_id = TitleId_Ro;
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

    DoWithSmSession([&]() {
        rc = setsysInitialize();
        if (R_FAILED(rc)) {
            std::abort();
        }

        rc = fsInitialize();
        if (R_FAILED(rc)) {
            std::abort();
        }

        if (GetRuntimeFirmwareVersion() < FirmwareVersion_300) {
            rc = pminfoInitialize();
            if (R_FAILED(rc)) {
                std::abort();
            }
        }

        rc = fsdevMountSdmc();
        if (R_FAILED(rc)) {
            std::abort();
        }
    });

    CheckAtmosphereVersion(CURRENT_ATMOSPHERE_VERSION);
}

void __appExit(void) {
    fsdevUnmountAll();
    fsExit();
    if (GetRuntimeFirmwareVersion() < FirmwareVersion_300) {
        pminfoExit();
    }
    setsysExit();
}

/* Helpers to create RO objects. */
static const auto MakeRoServiceForSelf = []() { return std::make_shared<RelocatableObjectsService>(RoModuleType_ForSelf); };
static const auto MakeRoServiceForOthers = []() { return std::make_shared<RelocatableObjectsService>(RoModuleType_ForOthers); };

int main(int argc, char **argv)
{
    /* Initialize. */
    Registration::Initialize();

    /* Static server manager. */
    static auto s_server_manager = WaitableManager(1);

    /* Create services. */
    s_server_manager.AddWaitable(new ServiceServer<DebugMonitorService>("ro:dmnt", 2));
    /* NOTE: Official code passes 32 for ldr:ro max sessions. We will pass 2, because that's the actual limit. */
    s_server_manager.AddWaitable(new ServiceServer<RelocatableObjectsService, +MakeRoServiceForSelf>("ldr:ro", 2));
    if (GetRuntimeFirmwareVersion() >= FirmwareVersion_700) {
        s_server_manager.AddWaitable(new ServiceServer<RelocatableObjectsService, +MakeRoServiceForOthers>("ro:1", 2));
    }

    /* Loop forever, servicing our services. */
    s_server_manager.Process();

    /* Cleanup */
    return 0;
}

