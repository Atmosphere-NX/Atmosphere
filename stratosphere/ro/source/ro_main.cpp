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
#include <stratosphere/ro.hpp>
#include <stratosphere/spl.hpp>

#include "ro_debug_monitor.hpp"
#include "ro_service.hpp"

extern "C" {
    extern u32 __start__;

    u32 __nx_applet_type = AppletType_None;

    #define INNER_HEAP_SIZE 0x30000
    size_t nx_inner_heap_size = INNER_HEAP_SIZE;
    char   nx_inner_heap[INNER_HEAP_SIZE];

    void __libnx_initheap(void);
    void __appInit(void);
    void __appExit(void);
}

/* Exception handling. */
sts::ncm::TitleId __stratosphere_title_id = sts::ncm::TitleId::Ro;

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
        R_ASSERT(setsysInitialize());
        R_ASSERT(fsInitialize());
        if (GetRuntimeFirmwareVersion() < FirmwareVersion_300) {
            R_ASSERT(pminfoInitialize());
        }
    });

    R_ASSERT(fsdevMountSdmc());

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

using namespace sts;

/* Helpers to create RO objects. */
static const auto MakeRoServiceForSelf = []() { return std::make_shared<ro::Service>(ro::ModuleType::ForSelf); };
static const auto MakeRoServiceForOthers = []() { return std::make_shared<ro::Service>(ro::ModuleType::ForOthers); };

int main(int argc, char **argv)
{
    /* Initialize Debug config. */
    {
        DoWithSmSession([]() {
            R_ASSERT(splInitialize());
        });
        ON_SCOPE_EXIT { splExit(); };

        ro::SetDevelopmentHardware(spl::IsDevelopmentHardware());
        ro::SetDevelopmentFunctionEnabled(spl::IsDevelopmentFunctionEnabled());
    }

    /* Static server manager. */
    static auto s_server_manager = WaitableManager(1);

    /* Create services. */
    s_server_manager.AddWaitable(new ServiceServer<ro::DebugMonitorService>("ro:dmnt", 2));
    /* NOTE: Official code passes 32 for ldr:ro max sessions. We will pass 2, because that's the actual limit. */
    s_server_manager.AddWaitable(new ServiceServer<ro::Service, +MakeRoServiceForSelf>("ldr:ro", 2));
    if (GetRuntimeFirmwareVersion() >= FirmwareVersion_700) {
        s_server_manager.AddWaitable(new ServiceServer<ro::Service, +MakeRoServiceForOthers>("ro:1", 2));
    }

    /* Loop forever, servicing our services. */
    s_server_manager.Process();

    /* Cleanup */
    return 0;
}

