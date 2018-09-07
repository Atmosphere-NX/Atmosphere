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
#include <stratosphere.hpp>

#include "sm_mitm.h"

#include "mitm_server.hpp"
#include "fsmitm_service.hpp"
#include "fsmitm_worker.hpp"

#include "mitm_query_service.hpp"

#include "fsmitm_utils.hpp"

extern "C" {
    extern u32 __start__;

    u32 __nx_applet_type = AppletType_None;

    #define INNER_HEAP_SIZE 0x1000000
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
    
    rc = smInitialize();
    if (R_FAILED(rc)) {
        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_SM));
    }
    
    rc = smMitMInitialize();
    if (R_FAILED(rc)) {
        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_SM));
    }
    
    rc = fsInitialize();
    if (R_FAILED(rc)) {
        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_FS));
    }
    
    rc = splInitialize();
    if (R_FAILED(rc))  {
        fatalSimple(0xCAFE << 4 | 3);
    }
    
    /* Check for exosphere API compatibility. */
    u64 exosphere_cfg;
    if (R_SUCCEEDED(splGetConfig((SplConfigItem)65000, &exosphere_cfg))) {
        /* MitM requires Atmosphere API 0.1. */
        u16 api_version = (exosphere_cfg >> 16) & 0xFFFF;
        if (api_version < 0x0001) {
            fatalSimple(0xCAFE << 4 | 0xFE);
        }
    } else {
        fatalSimple(0xCAFE << 4 | 0xFF);
    }
    
    //splExit();
}

void __appExit(void) {
    /* Cleanup services. */
    fsExit();
    smMitMExit();
    smExit();
}

int main(int argc, char **argv)
{
    Thread worker_thread = {0};
    Thread sd_initializer_thread = {0};
    consoleDebugInit(debugDevice_SVC);
    
    consoleDebugInit(debugDevice_SVC);
    
    if (R_FAILED(threadCreate(&worker_thread, &FsMitMWorker::Main, NULL, 0x20000, 45, 0))) {
        /* TODO: Panic. */
    }
    if (R_FAILED(threadStart(&worker_thread))) {
        /* TODO: Panic. */
    }
    
    if (R_FAILED(threadCreate(&sd_initializer_thread, &Utils::InitializeSdThreadFunc, NULL, 0x4000, 0x15, 0))) {
        /* TODO: Panic. */
    }
    if (R_FAILED(threadStart(&sd_initializer_thread))) {
        /* TODO: Panic. */
    }
    
    /* TODO: What's a good timeout value to use here? */
    auto server_manager = std::make_unique<MultiThreadedWaitableManager>(1, U64_MAX, 0x20000);
    //auto server_manager = std::make_unique<WaitableManager>(U64_MAX);
        
    /* Create fsp-srv mitm. */
    ISession<MitMQueryService<FsMitMService>> *fs_query_srv = NULL;
    MitMServer<FsMitMService> *fs_srv = new MitMServer<FsMitMService>(&fs_query_srv, "fsp-srv", 61);
    server_manager->add_waitable(fs_srv);
    server_manager->add_waitable(fs_query_srv);
            
    /* Loop forever, servicing our services. */
    server_manager->process();

    return 0;
}

