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
 
#include <switch.h>
#include "dmnt_cheat_manager.hpp"
#include "pm_shim.h"

static HosMutex g_cheat_lock;
static HosThread g_detect_thread, g_vm_thread;

static bool g_has_cheat_process;
static CheatProcessMetadata g_cheat_process_metadata = {0};
static Handle g_cheat_process_debug_hnd = 0;

Handle DmntCheatManager::PrepareDebugNextApplication() {
    Result rc;
    Handle event_h;
    if (R_FAILED((rc = pmdmntEnableDebugForApplication(&event_h)))) {
        fatalSimple(rc);
    }
    
    return event_h;
}

static void PopulateMemoryExtents(MemoryRegionExtents *extents, Handle p_h, u64 id_base, u64 id_size) {
    Result rc;
    /* Get base extent. */
    if (R_FAILED((rc = svcGetInfo(&extents->base, id_base, p_h, 0)))) {
        fatalSimple(rc);
    }
    
    /* Get size extent. */
    if (R_FAILED((rc = svcGetInfo(&extents->size, id_size, p_h, 0)))) {
        fatalSimple(rc);
    }
}

void DmntCheatManager::OnNewApplicationLaunch() {
    std::scoped_lock<HosMutex> lk(g_cheat_lock);
    Result rc;
    
    /* Close the current application, if it's open. */
    if (g_cheat_process_debug_hnd != 0) {
        svcCloseHandle(g_cheat_process_debug_hnd);
        g_cheat_process_debug_hnd = 0;
        g_cheat_process_metadata = (CheatProcessMetadata){0};
    }
    
    /* Get the new application's process ID. */
    if (R_FAILED((rc = pmdmntGetApplicationPid(&g_cheat_process_metadata.process_id)))) {
        fatalSimple(rc);
    }
    
    /* Get process handle, use it to learn memory extents. */
    {
        Handle proc_h = 0;
        ON_SCOPE_EXIT { if (proc_h != 0) { svcCloseHandle(proc_h); } };
        
        if (R_FAILED((rc = pmdmntAtmosphereGetProcessHandle(&proc_h, g_cheat_process_metadata.process_id)))) {
            fatalSimple(rc);
        }
        
        /* Get memory extents. */
        PopulateMemoryExtents(&g_cheat_process_metadata.heap_extents, proc_h, 4, 5);
        PopulateMemoryExtents(&g_cheat_process_metadata.alias_extents, proc_h, 2, 3);
        if (kernelAbove200()) {
            PopulateMemoryExtents(&g_cheat_process_metadata.address_space_extents, proc_h, 12, 13);
        } else {
            g_cheat_process_metadata.address_space_extents.base = 0x08000000UL;
            g_cheat_process_metadata.address_space_extents.size = 0x78000000UL;
        }
    }
    
    /* Get module information from Loader. */
    {
        LoaderModuleInfo proc_modules[2];
        u32 num_modules;
        if (R_FAILED((rc = ldrDmntGetModuleInfos(g_cheat_process_metadata.process_id, &proc_modules, 2, &num_modules)))) {
            fatalSimple(rc);
        }
        
        /* All applications must have two modules. */
        /* If we only have one, we must be e.g. mitming HBL. */
        /* We don't want to fuck with HBL. */
        if (num_modules != 2) {
            g_cheat_process_metadata.process_id = 0;
            return;
        }
        
        g_cheat_process_metadata.main_nso_extents.base = proc_modules[1].base_address;
        g_cheat_process_metadata.main_nso_extents.size = proc_modules[1].size;
        g_cheat_process_metadata.main_nso_build_id = proc_modules[1].build_id;
    }
    
    /* Open a debug handle. */
    if (R_FAILED((rc = svcDebugActiveProcess(&g_cheat_process_debug_hnd, g_cheat_process_metadata.process_id)))) {
        fatalSimple(rc);
    }
    
    /* TODO: Continue debug events, etc. */
}

void DmntCheatManager::DetectThread(void *arg) {
    auto waiter = new WaitableManager(1);
    waiter->AddWaitable(LoadReadOnlySystemEvent(PrepareDebugNextApplication(), [](u64 timeout) {
        /* Process stuff for new application. */
        DmntCheatManager::OnNewApplicationLaunch();
        
        /* Setup detection for the next application, and close the duplicate handle. */
        svcCloseHandle(PrepareDebugNextApplication());
        
        return 0x0;
    }, true));
    
    waiter->Process();
    delete waiter;
}
 
void DmntCheatManager::VmThread(void *arg) {
    while (true) {
        /* TODO */
        svcSleepThread(0x5000000ul);
    }
}

void DmntCheatManager::InitializeCheatManager() {
    /* Spawn application detection thread, spawn cheat vm thread. */
    if (R_FAILED(g_detect_thread.Initialize(&DmntCheatManager::DetectThread, nullptr, 0x4000, 28))) {
        std::abort();
    }
    if (R_FAILED(g_vm_thread.Initialize(&DmntCheatManager::VmThread, nullptr, 0x4000, 28))) {
        std::abort();
    }
    
    /* Start threads. */
    if (R_FAILED(g_detect_thread.Start()) || R_FAILED(g_vm_thread.Start())) {
        std::abort();
    }
}
