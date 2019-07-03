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

#include "dmnt_service.hpp"
#include "dmnt_cheat_service.hpp"
#include "dmnt_cheat_manager.hpp"
#include "dmnt_config.hpp"

extern "C" {
    extern u32 __start__;

    u32 __nx_applet_type = AppletType_None;

    #define INNER_HEAP_SIZE 0x80000
    size_t nx_inner_heap_size = INNER_HEAP_SIZE;
    char   nx_inner_heap[INNER_HEAP_SIZE];

    void __libnx_initheap(void);
    void __appInit(void);
    void __appExit(void);
}

/* Exception handling. */
sts::ncm::TitleId __stratosphere_title_id = sts::ncm::TitleId::Dmnt;

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
        R_ASSERT(pmdmntInitialize());
        R_ASSERT(ldrDmntInitialize());
        /* TODO: We provide this on every sysver via ro. Do we need a shim? */
        if (GetRuntimeFirmwareVersion() >= FirmwareVersion_300) {
            R_ASSERT(roDmntInitialize());
        }
        R_ASSERT(nsdevInitialize());
        R_ASSERT(lrInitialize());
        R_ASSERT(setInitialize());
        R_ASSERT(setsysInitialize());
        R_ASSERT(hidInitialize());
        R_ASSERT(fsInitialize());
    });

    R_ASSERT(fsdevMountSdmc());

    CheckAtmosphereVersion(CURRENT_ATMOSPHERE_VERSION);
}

void __appExit(void) {
    /* Cleanup services. */
    fsdevUnmountAll();
    fsExit();
    hidExit();
    setsysExit();
    setExit();
    lrExit();
    nsdevExit();
    roDmntExit();
    ldrDmntExit();
    pmdmntExit();
}

int main(int argc, char **argv)
{
    consoleDebugInit(debugDevice_SVC);

    /* Initialize configuration manager. */
    DmntConfigManager::RefreshConfiguration();

    /* Start cheat manager. */
    DmntCheatManager::InitializeCheatManager();

    /* Nintendo uses four threads. Add a fifth for our cheat service. */
    static auto s_server_manager = WaitableManager(5);

    /* Create services. */

    /* TODO: Implement rest of dmnt:- in ams.tma development branch. */
    /* server_manager->AddWaitable(new ServiceServer<DebugMonitorService>("dmnt:-", 4)); */


    s_server_manager.AddWaitable(new ServiceServer<DmntCheatService>("dmnt:cht", 1));

    /* Loop forever, servicing our services. */
    s_server_manager.Process();

    return 0;
}

