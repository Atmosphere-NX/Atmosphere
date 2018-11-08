/*
 * Copyright (c) 2018 Atmosphère-NX
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
#include "ldr_ro_service.hpp"

extern "C" {
    extern u32 __start__;

    u32 __nx_applet_type = AppletType_None;

    #define INNER_HEAP_SIZE 0x20000
    size_t nx_inner_heap_size = INNER_HEAP_SIZE;
    char   nx_inner_heap[INNER_HEAP_SIZE];
    
    void __libnx_initheap(void);
    void __appInit(void);
    void __appExit(void);

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

    /* Initialize services we need (TODO: SPL) */
    rc = smInitialize();
    if (R_FAILED(rc)) {
        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_SM));
    }
    
    rc = fsInitialize();
    if (R_FAILED(rc)) {
        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_FS));
    }
        
    
    rc = lrInitialize();
    if (R_FAILED(rc))  {
        fatalSimple(0xCAFE << 4 | 1);
    }
    
    rc = fsldrInitialize();
    if (R_FAILED(rc))  {
        fatalSimple(0xCAFE << 4 | 2);
    }
    
    CheckAtmosphereVersion(CURRENT_ATMOSPHERE_VERSION);
}

void __appExit(void) {
    /* Cleanup services. */
    fsdevUnmountAll();
    fsldrExit();
    lrExit();
    fsExit();
    smExit();
}

struct LoaderServerOptions {
    static constexpr size_t PointerBufferSize = 0x400;
    static constexpr size_t MaxDomains = 0;
    static constexpr size_t MaxDomainObjects = 0;
};

int main(int argc, char **argv)
{
    consoleDebugInit(debugDevice_SVC);
            
    auto server_manager = new WaitableManager<LoaderServerOptions>(1);
        
    /* Add services to manager. */
    server_manager->AddWaitable(new ServiceServer<ProcessManagerService>("ldr:pm", 1));
    server_manager->AddWaitable(new ServiceServer<ShellService>("ldr:shel", 3));
    server_manager->AddWaitable(new ServiceServer<DebugMonitorService>("ldr:dmnt", 2));
    if (GetRuntimeFirmwareVersion() < FirmwareVersion_300) {
        /* On 1.0.0-2.3.0, Loader services ldr:ro instead of ro. */
        server_manager->AddWaitable(new ServiceServer<RelocatableObjectsService>("ldr:ro", 0x20));
    }
        
    /* Loop forever, servicing our services. */
    server_manager->Process();
    
    delete server_manager;
    
	return 0;
}

