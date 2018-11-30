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

#include "pm_boot_mode.hpp"
#include "pm_info.hpp"
#include "pm_shell.hpp"
#include "pm_process_track.hpp"
#include "pm_registration.hpp"
#include "pm_debug_monitor.hpp"

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

void RegisterPrivilegedProcessesWithFs() {
    /* Ensures that all privileged processes are registered with full FS permissions. */
    constexpr u64 PRIVILEGED_PROCESS_MIN = 0;
    constexpr u64 PRIVILEGED_PROCESS_MAX = 0x4F;
    
    const u32 PRIVILEGED_FAH[0x1C/sizeof(u32)] = {0x00000001, 0x00000000, 0x80000000, 0x0000001C, 0x00000000, 0x0000001C, 0x00000000};
    const u32 PRIVILEGED_FAC[0x2C/sizeof(u32)] = {0x00000001, 0x00000000, 0x80000000, 0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF};
    
    u32 num_pids;
    u64 pids[PRIVILEGED_PROCESS_MAX+1];
    if (R_SUCCEEDED(svcGetProcessList(&num_pids, pids, sizeof(pids)/sizeof(pids[0])))) {
        for (u32 i = 0; i < num_pids; i++) {
            const u64 pid = pids[i];
            if (PRIVILEGED_PROCESS_MIN <= pid && pid <= PRIVILEGED_PROCESS_MAX) {
                fsprUnregisterProgram(pid);
                fsprRegisterProgram(pid, pid, FsStorageId_NandSystem,  PRIVILEGED_FAH, sizeof(PRIVILEGED_FAH), PRIVILEGED_FAC, sizeof(PRIVILEGED_FAC));
            }
        }
    } else {
        for (u64 pid = PRIVILEGED_PROCESS_MIN; pid <= PRIVILEGED_PROCESS_MAX; pid++) {
            fsprUnregisterProgram(pid);
            fsprRegisterProgram(pid, pid, FsStorageId_NandSystem,  PRIVILEGED_FAH, sizeof(PRIVILEGED_FAH), PRIVILEGED_FAC, sizeof(PRIVILEGED_FAC));
        }
    }
}

void __appInit(void) {
    Result rc;

    rc = smInitialize();
    if (R_FAILED(rc)) {
        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_SM));
    }
    
    rc = fsprInitialize();
    if (R_FAILED(rc))  {
        fatalSimple(0xCAFE << 4 | 1);
    }
    
    /* This works around a bug with process permissions on < 4.0.0. */
    RegisterPrivilegedProcessesWithFs();
    
    rc = smManagerAmsInitialize();
    if (R_SUCCEEDED(rc)) {
        smManagerAmsEndInitialDefers();
    } else {
        fatalSimple(0xCAFE << 4 | 2);
    }
    
    rc = smManagerInitialize();
    if (R_FAILED(rc))  {
        fatalSimple(0xCAFE << 4 | 3);
    }
        
    rc = lrInitialize();
    if (R_FAILED(rc))  {
        fatalSimple(0xCAFE << 4 | 4);
    }
    
    rc = ldrPmInitialize();
    if (R_FAILED(rc))  {
        fatalSimple(0xCAFE << 4 | 5);
    }
    
    rc = splInitialize();
    if (R_FAILED(rc))  {
        fatalSimple(0xCAFE << 4 | 6);
    }
    
    rc = fsInitialize();
    if (R_FAILED(rc)) {
        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_FS));
    }
    
    CheckAtmosphereVersion(CURRENT_ATMOSPHERE_VERSION);
}

void __appExit(void) {
    /* Cleanup services. */
    fsdevUnmountAll();
    splExit();
    smManagerExit();
    ldrPmExit();
    fsprExit();
    lrExit();
    fsExit();
    smExit();
}

int main(int argc, char **argv)
{
    Thread process_track_thread = {0};
    consoleDebugInit(debugDevice_SVC);
    
    /* Initialize and spawn the Process Tracking thread. */
    Registration::InitializeSystemResources();
    if (R_FAILED(threadCreate(&process_track_thread, &ProcessTracking::MainLoop, NULL, 0x4000, 0x15, 0))) {
        /* TODO: Panic. */
    }
    if (R_FAILED(threadStart(&process_track_thread))) {
        /* TODO: Panic. */
    }
    
    /* TODO: What's a good timeout value to use here? */
    auto server_manager = new WaitableManager(1);
        
    /* TODO: Create services. */
    server_manager->AddWaitable(new ServiceServer<ShellService>("pm:shell", 3));
    server_manager->AddWaitable(new ServiceServer<DebugMonitorService>("pm:dmnt", 2));
    server_manager->AddWaitable(new ServiceServer<BootModeService>("pm:bm", 5));
    server_manager->AddWaitable(new ServiceServer<InformationService>("pm:info", 1));
    
    /* Loop forever, servicing our services. */
    server_manager->Process();
    
    /* Cleanup. */
    delete server_manager;
    return 0;
}

