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

#include "tma_usb_comms.hpp"

extern "C" {
    extern u32 __start__;

    u32 __nx_applet_type = AppletType_None;

    #define INNER_HEAP_SIZE 0x100000
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
    
    rc = pscInitialize();
    if (R_FAILED(rc)) {
        fatalSimple(rc);
    }
    
    CheckAtmosphereVersion();
}

void __appExit(void) {
    /* Cleanup services. */
    pscExit();
    smExit();
}

void PmThread(void *arg) {
    /* Setup psc module. */
    Result rc;
    PscPmModule tma_module = {0};
    if (R_FAILED((rc = pscGetPmModule(&tma_module, 0x1E, nullptr, 0, true)))) {
        fatalSimple(rc);
    }
    
    /* For now, just do what dummy tma does -- loop forever, acknowledging everything. */
    while (true) {
        if (R_FAILED((rc = eventWait(&tma_module.event, U64_MAX)))) {
            fatalSimple(rc);
        }
        
        PscPmState state;
        u32 flags;
        if (R_FAILED((rc = pscPmModuleGetRequest(&tma_module, &state, &flags)))) {
            fatalSimple(rc);
        }
        
        
        if (R_FAILED((rc = pscPmModuleAcknowledge(&tma_module, state)))) {
            fatalSimple(rc);
        }
    }
}

int main(int argc, char **argv)
{
    consoleDebugInit(debugDevice_SVC);
    Thread pm_thread = {0};
    if (R_FAILED(threadCreate(&pm_thread, &PmThread, NULL, 0x4000, 0x15, 0))) {
        /* TODO: Panic. */
    }
    if (R_FAILED(threadStart(&pm_thread))) {
        /* TODO: Panic. */
    }
    
    TmaUsbComms::Initialize();
    TmaPacket *packet = new TmaPacket();
    usbDsWaitReady(U64_MAX);
    packet->Write<u64>(0xCAFEBABEDEADCAFEUL);
    packet->Write<u64>(0xCCCCCCCCCCCCCCCCUL);
    TmaUsbComms::SendPacket(packet);
    packet->ClearOffset();
    while (true) {
        if (TmaUsbComms::ReceivePacket(packet) == TmaConnResult::Success) {
            TmaUsbComms::SendPacket(packet);
        }
    }

    
    return 0;
}

