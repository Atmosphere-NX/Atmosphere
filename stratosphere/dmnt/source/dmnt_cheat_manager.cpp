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
#include "dmnt_cheat_vm.hpp"
#include "dmnt_config.hpp"
#include "pm_shim.h"

static HosMutex g_cheat_lock;
static HosThread g_detect_thread, g_vm_thread;

static IEvent *g_cheat_process_event;
static DmntCheatVm *g_cheat_vm;

static CheatProcessMetadata g_cheat_process_metadata = {0};
static Handle g_cheat_process_debug_hnd = 0;

void DmntCheatManager::CloseActiveCheatProcess() {
    if (g_cheat_process_debug_hnd != 0) {
        svcCloseHandle(g_cheat_process_debug_hnd);
        g_cheat_process_debug_hnd = 0;
        g_cheat_process_metadata = (CheatProcessMetadata){0};
        g_cheat_process_event->Signal();
    }
}

bool DmntCheatManager::HasActiveCheatProcess() {
    u64 tmp;
    bool has_cheat_process = g_cheat_process_debug_hnd != 0;
    
    if (has_cheat_process) {
        has_cheat_process &= R_SUCCEEDED(svcGetProcessId(&tmp, g_cheat_process_debug_hnd));
    }
    
    if (has_cheat_process) {
        has_cheat_process &= R_SUCCEEDED(pmdmntGetApplicationPid(&tmp));
    }
    
    if (has_cheat_process) {
        has_cheat_process &= tmp == g_cheat_process_metadata.process_id;
    }
    
    if (!has_cheat_process) {
        CloseActiveCheatProcess();
    }
    
    return has_cheat_process;
}

void DmntCheatManager::ContinueCheatProcess() {
    if (HasActiveCheatProcess()) {
        /* Loop getting debug events. */
        u64 debug_event_buf[0x50];
        while (R_SUCCEEDED(svcGetDebugEvent((u8 *)debug_event_buf, g_cheat_process_debug_hnd))) {
            /* ... */
        }
        
        /* Continue the process. */
        if (kernelAbove300()) {
            svcContinueDebugEvent(g_cheat_process_debug_hnd, 5, nullptr, 0);
        } else {
            svcLegacyContinueDebugEvent(g_cheat_process_debug_hnd, 5, 0);
        }
    }
}

Result DmntCheatManager::ReadCheatProcessMemoryForVm(u64 proc_addr, void *out_data, size_t size) {
    if (HasActiveCheatProcess()) {
        return svcReadDebugProcessMemory(out_data, g_cheat_process_debug_hnd, proc_addr, size);
    }
    
    return ResultDmntCheatNotAttached;
}

Result DmntCheatManager::WriteCheatProcessMemoryForVm(u64 proc_addr, const void *data, size_t size) {
    if (HasActiveCheatProcess()) {
        return svcWriteDebugProcessMemory(g_cheat_process_debug_hnd, data, proc_addr, size);
    }
    
    return ResultDmntCheatNotAttached;
}


Result DmntCheatManager::GetCheatProcessMappingCount(u64 *out_count) {
    std::scoped_lock<HosMutex> lk(g_cheat_lock);
    
    if (!HasActiveCheatProcess()) {
        return ResultDmntCheatNotAttached;
    }
    
    MemoryInfo mem_info;
    
    u64 address = 0;
    *out_count = 0;
    do {
        mem_info.perm = 0;
        u32 tmp;
        if (R_FAILED(svcQueryDebugProcessMemory(&mem_info, &tmp, g_cheat_process_debug_hnd, address))) {
            break;
        }
        
        if (mem_info.perm != 0) {
            *out_count += 1;
        }
        
        address = mem_info.addr + mem_info.size;
    } while (address != 0);
    
    return 0;
}

Result DmntCheatManager::GetCheatProcessMappings(MemoryInfo *mappings, size_t max_count, u64 *out_count, u64 offset) {
    std::scoped_lock<HosMutex> lk(g_cheat_lock);
    
    if (!HasActiveCheatProcess()) {
        return ResultDmntCheatNotAttached;
    }
    
    MemoryInfo mem_info;
    u64 address = 0;
    u64 count = 0;
    *out_count = 0;
    do {
        mem_info.perm = 0;
        u32 tmp;
        if (R_FAILED(svcQueryDebugProcessMemory(&mem_info, &tmp, g_cheat_process_debug_hnd, address))) {
            break;
        }
        
        if (mem_info.perm != 0) {
            count++;
            if (count > offset) {
                mappings[(*out_count)++] = mem_info;
            }
        }
        
        address = mem_info.addr + mem_info.size;
    } while (address != 0 && *out_count < max_count);
    
    return 0;
}

Result DmntCheatManager::ReadCheatProcessMemory(u64 proc_addr, void *out_data, size_t size) {
    std::scoped_lock<HosMutex> lk(g_cheat_lock);
    
    return ReadCheatProcessMemoryForVm(proc_addr, out_data, size);
}

Result DmntCheatManager::WriteCheatProcessMemory(u64 proc_addr, const void *data, size_t size) {
    std::scoped_lock<HosMutex> lk(g_cheat_lock);
    
    return WriteCheatProcessMemoryForVm(proc_addr, data, size);
}

Result DmntCheatManager::QueryCheatProcessMemory(MemoryInfo *mapping, u64 address) {
    std::scoped_lock<HosMutex> lk(g_cheat_lock);
    
    if (HasActiveCheatProcess()) {
        u32 tmp;
        return svcQueryDebugProcessMemory(mapping, &tmp, g_cheat_process_debug_hnd, address);
    }
    
    return ResultDmntCheatNotAttached;
}

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

static void StartDebugProcess(u64 pid) {
    Result rc = pmdmntStartProcess(pid);
    if (R_FAILED(rc)) {
        fatalSimple(rc);
    }
}

Result DmntCheatManager::ForceOpenCheatProcess() {
    std::scoped_lock<HosMutex> lk(g_cheat_lock);
    Result rc;
    
    if (HasActiveCheatProcess()) {
        return 0;
    }
    
    /* Get the current application process ID. */
    if (R_FAILED((rc = pmdmntGetApplicationPid(&g_cheat_process_metadata.process_id)))) {
        return rc;
    }
    ON_SCOPE_EXIT { if (R_FAILED(rc)) { g_cheat_process_metadata.process_id = 0; } };
    
    /* Get process handle, use it to learn memory extents. */
    {
        Handle proc_h = 0;
        ON_SCOPE_EXIT { if (proc_h != 0) { svcCloseHandle(proc_h); } };
        
        if (R_FAILED((rc = pmdmntAtmosphereGetProcessInfo(&proc_h, &g_cheat_process_metadata.title_id, nullptr, g_cheat_process_metadata.process_id)))) {
            return rc;
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
        if (R_FAILED((rc = ldrDmntGetModuleInfos(g_cheat_process_metadata.process_id, proc_modules, sizeof(proc_modules), &num_modules)))) {
            return rc;
        }
        
        /* All applications must have two modules. */
        /* However, this is a force-open, so we will accept one module. */
        /* Poor HBL, I guess... */
        LoaderModuleInfo *proc_module;
        if (num_modules == 2) {
            proc_module = &proc_modules[1];
        } else if (num_modules == 1) {
            proc_module = &proc_modules[0];
        } else {
            rc = ResultDmntCheatNotAttached;
            return rc;
        }
        
        g_cheat_process_metadata.main_nso_extents.base = proc_module->base_address;
        g_cheat_process_metadata.main_nso_extents.size = proc_module->size;
        memcpy(g_cheat_process_metadata.main_nso_build_id, proc_module->build_id, sizeof(g_cheat_process_metadata.main_nso_build_id));
    }
    
    /* TODO: Read cheats off the SD. */
    
    /* Open a debug handle. */
    if (R_FAILED((rc = svcDebugActiveProcess(&g_cheat_process_debug_hnd, g_cheat_process_metadata.process_id)))) {
        return rc;
    }
        
    /* Continue debug events, etc. */
    ContinueCheatProcess();
    
    /* Signal to our fans. */
    g_cheat_process_event->Signal();
    
    return rc;
}

void DmntCheatManager::OnNewApplicationLaunch() {
    std::scoped_lock<HosMutex> lk(g_cheat_lock);
    Result rc;
    
    /* Close the current application, if it's open. */
    CloseActiveCheatProcess();
    
    /* Get the new application's process ID. */
    if (R_FAILED((rc = pmdmntGetApplicationPid(&g_cheat_process_metadata.process_id)))) {
        fatalSimple(rc);
    }
    
    /* Get process handle, use it to learn memory extents. */
    {
        Handle proc_h = 0;
        ON_SCOPE_EXIT { if (proc_h != 0) { svcCloseHandle(proc_h); } };
        
        if (R_FAILED((rc = pmdmntAtmosphereGetProcessInfo(&proc_h, &g_cheat_process_metadata.title_id, nullptr, g_cheat_process_metadata.process_id)))) {
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
    
    /* Check if we should skip based on keys down. */
    if (!DmntConfigManager::HasCheatEnableButton(g_cheat_process_metadata.title_id)) {
        StartDebugProcess(g_cheat_process_metadata.process_id);
        g_cheat_process_metadata.process_id = 0;
        return;
    }
    
    /* Get module information from Loader. */
    {
        LoaderModuleInfo proc_modules[2];
        u32 num_modules;
        if (R_FAILED((rc = ldrDmntGetModuleInfos(g_cheat_process_metadata.process_id, proc_modules, sizeof(proc_modules), &num_modules)))) {
            fatalSimple(rc);
        }
        
        /* All applications must have two modules. */
        /* If we only have one, we must be e.g. mitming HBL. */
        /* We don't want to fuck with HBL. */
        if (num_modules != 2) {
            StartDebugProcess(g_cheat_process_metadata.process_id);
            g_cheat_process_metadata.process_id = 0;
            return;
        }
        
        g_cheat_process_metadata.main_nso_extents.base = proc_modules[1].base_address;
        g_cheat_process_metadata.main_nso_extents.size = proc_modules[1].size;
        memcpy(g_cheat_process_metadata.main_nso_build_id, proc_modules[1].build_id, sizeof(g_cheat_process_metadata.main_nso_build_id));
    }
        
    /* TODO: Read cheats off the SD. */
    
    /* Open a debug handle. */
    if (R_FAILED((rc = svcDebugActiveProcess(&g_cheat_process_debug_hnd, g_cheat_process_metadata.process_id)))) {
        fatalSimple(rc);
    }
    
    /* Start the process. */
    StartDebugProcess(g_cheat_process_metadata.process_id);
    
    /* Continue debug events, etc. */
    ContinueCheatProcess();
    
    /* Signal to our fans. */
    g_cheat_process_event->Signal();
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
        /* Execute Cheat VM. */
        {
            /* Acquire lock. */
            std::scoped_lock<HosMutex> lk(g_cheat_lock);
            
            if (HasActiveCheatProcess()) {
                /* Handle any pending debug events. */
                ContinueCheatProcess();
                
                /* Execute VM. */
                if (g_cheat_vm->GetProgramSize() != 0) {
                    g_cheat_vm->Execute(&g_cheat_process_metadata);
                }
            }
        }
        svcSleepThread(0x5000000ul);
    }
}

bool DmntCheatManager::GetHasActiveCheatProcess() {
    std::scoped_lock<HosMutex> lk(g_cheat_lock);
    
    return HasActiveCheatProcess();
}

Handle DmntCheatManager::GetCheatProcessEventHandle() {
    return g_cheat_process_event->GetHandle();
}

Result DmntCheatManager::GetCheatProcessMetadata(CheatProcessMetadata *out) {
    std::scoped_lock<HosMutex> lk(g_cheat_lock);
    
    if (HasActiveCheatProcess()) {
        *out = g_cheat_process_metadata;
        return 0;
    }
    
    return ResultDmntCheatNotAttached;
}

void DmntCheatManager::InitializeCheatManager() {
    /* Create cheat process detection event. */
    g_cheat_process_event = CreateWriteOnlySystemEvent();
    
    /* Create cheat vm. */
    g_cheat_vm = new DmntCheatVm();
    
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
