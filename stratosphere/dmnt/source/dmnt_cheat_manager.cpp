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
 
#include <map>
#include <switch.h>
#include "dmnt_cheat_manager.hpp"
#include "dmnt_cheat_vm.hpp"
#include "dmnt_config.hpp"
#include "pm_shim.h"

static HosMutex g_cheat_lock;
static HosThread g_detect_thread, g_vm_thread, g_debug_events_thread;

static IEvent *g_cheat_process_event;
static DmntCheatVm *g_cheat_vm;

static CheatProcessMetadata g_cheat_process_metadata = {0};
static Handle g_cheat_process_debug_hnd = 0;

/* Should we enable cheats by default? */
static bool g_enable_cheats_by_default = true;

/* For debug event thread management. */
static HosMutex g_debug_event_thread_lock;
static bool g_has_debug_events_thread = false;

/* To save some copying. */
static bool g_needs_reload_vm_program = false;

/* Global cheat entry storage. */
static CheatEntry g_cheat_entries[DmntCheatManager::MaxCheatCount];

/* Global frozen address storage. */
static std::map<u64, FrozenAddressValue> g_frozen_addresses_map;

void DmntCheatManager::StartDebugEventsThread() {
    std::scoped_lock<HosMutex> lk(g_debug_event_thread_lock);
    
    /* Spawn the debug events thread. */
    if (!g_has_debug_events_thread) {
        Result rc;
        
        if (R_FAILED((rc = g_debug_events_thread.Initialize(&DmntCheatManager::DebugEventsThread, nullptr, 0x4000, 48)))) {
            return fatalSimple(rc);
        }
        
        if (R_FAILED((rc = g_debug_events_thread.Start()))) {
            return fatalSimple(rc);
        }
        
        g_has_debug_events_thread = true;
    }
}

void DmntCheatManager::WaitDebugEventsThread() {
    std::scoped_lock<HosMutex> lk(g_debug_event_thread_lock);
    
    /* Wait for the thread to exit. */
    if (g_has_debug_events_thread) {
        g_debug_events_thread.CancelSynchronization();
        g_debug_events_thread.Join();
        
        g_has_debug_events_thread = false;
    }
}

void DmntCheatManager::CloseActiveCheatProcess() {
    if (g_cheat_process_debug_hnd != 0) {
        /* Close process resources. */
        svcCloseHandle(g_cheat_process_debug_hnd);
        g_cheat_process_debug_hnd = 0;
        g_cheat_process_metadata = (CheatProcessMetadata){0};
        
        /* Clear cheat list. */
        ResetAllCheatEntries();
        
        /* Clear frozen addresses. */
        ResetFrozenAddresses();
        
        /* Signal to our fans. */
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
        Result rc = svcWriteDebugProcessMemory(g_cheat_process_debug_hnd, data, proc_addr, size);
        
        /* We might have a frozen address. Update it if we do! */
        if (R_SUCCEEDED(rc)) {
            for (auto & [address, value] : g_frozen_addresses_map) {
                /* Map is in order, so break here. */
                if (address >= proc_addr + size) {
                    break;
                }
                
                /* Check if we need to write. */
                if (proc_addr <= address) {
                    const size_t offset = (address - proc_addr);
                    const size_t size_to_copy = size - offset;
                    memcpy(&value.value, (void *)((uintptr_t)data + offset), size_to_copy < sizeof(value.value) ? size_to_copy : sizeof(value.value));
                }
            }
        }
        
        return rc;
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

void DmntCheatManager::ResetFrozenAddresses() {
    /* Just clear the map. */
    g_frozen_addresses_map.clear();
}

void DmntCheatManager::ResetCheatEntry(size_t i) {
    if (i < DmntCheatManager::MaxCheatCount) {
        g_cheat_entries[i].enabled = false;
        g_cheat_entries[i].cheat_id = i;
        g_cheat_entries[i].definition = {0};
        
        /* Trigger a VM reload. */
        g_needs_reload_vm_program = true;
    }
}

void DmntCheatManager::ResetAllCheatEntries() {
    for (size_t i = 0; i < DmntCheatManager::MaxCheatCount; i++) {
        ResetCheatEntry(i);
    }
}

CheatEntry *DmntCheatManager::GetFreeCheatEntry() {
    /* Check all non-master cheats for availability. */
    for (size_t i = 1; i < DmntCheatManager::MaxCheatCount; i++) {
        if (g_cheat_entries[i].definition.num_opcodes == 0) {
            return &g_cheat_entries[i];
        }
    }
    
    return nullptr;
}

CheatEntry *DmntCheatManager::GetCheatEntryById(size_t i) {
    if (i < DmntCheatManager::MaxCheatCount) {
        return &g_cheat_entries[i];
    }
    
    return nullptr;
}

bool DmntCheatManager::ParseCheats(const char *s, size_t len) {
    size_t i = 0;
    CheatEntry *cur_entry = NULL;
        
    /* Trigger a VM reload. */
    g_needs_reload_vm_program = true;
    
    while (i < len) {
        if (isspace(s[i])) {
            /* Just ignore space. */
            i++;
        } else if (s[i] == '[') {
            /* Parse a readable cheat name. */
            cur_entry = GetFreeCheatEntry();
            if (cur_entry == NULL) {
                return false;
            }
            
            /* Extract name bounds. */
            size_t j = i + 1;
            while (s[j] != ']') {
                j++;
                if (j >= len || (j - i - 1) >= sizeof(cur_entry->definition.readable_name)) {
                    return false;
                }
            }
            
            /* s[i+1:j] is cheat name. */
            const size_t cheat_name_len = (j - i - 1);
            memcpy(cur_entry->definition.readable_name, &s[i+1], cheat_name_len);
            cur_entry->definition.readable_name[cheat_name_len] = 0;
            
            /* Skip onwards. */
            i = j + 1;
        } else if (s[i] == '{') {
            /* We're parsing a master cheat. */
            cur_entry = &g_cheat_entries[0];
            
            /* There can only be one master cheat. */
            if (cur_entry->definition.num_opcodes > 0) {
                return false;
            }
            
            /* Extract name bounds */
            size_t j = i + 1;
            while (s[j] != '}') {
                j++;
                if (j >= len || (j - i - 1) >= sizeof(cur_entry->definition.readable_name)) {
                    return false;
                }
            }
            
            /* s[i+1:j] is cheat name. */
            const size_t cheat_name_len = (j - i - 1);
            memcpy(cur_entry->definition.readable_name, &s[i+1], cheat_name_len);
            cur_entry->definition.readable_name[cheat_name_len] = 0;
            
            /* Skip onwards. */
            i = j + 1;
        } else if (isxdigit(s[i])) {
            /* Make sure that we have a cheat open. */
            if (cur_entry == NULL) {
                return false;
            }
            
            /* Bounds check the opcode count. */
            if (cur_entry->definition.num_opcodes >= sizeof(cur_entry->definition.opcodes)/sizeof(cur_entry->definition.opcodes[0])) {
                return false;
            }
            
            /* We're parsing an instruction, so validate it's 8 hex digits. */
            for (size_t j = 1; j < 8; j++) {
                /* Validate 8 hex chars. */
                if (i + j >= len || !isxdigit(s[i+j])) {
                    return false;
                }
            }
           
            /* Parse the new opcode. */
            char hex_str[9] = {0};
            memcpy(hex_str, &s[i], 8);
            cur_entry->definition.opcodes[cur_entry->definition.num_opcodes++] = strtoul(hex_str, NULL, 16);
            
            
            /* Skip onwards. */
            i += 8;
        } else {
            /* Unexpected character encountered. */
            return false;
        }
    }
    
    /* Master cheat can't be disabled. */
    if (g_cheat_entries[0].definition.num_opcodes > 0) {
        g_cheat_entries[0].enabled = true;
    }
    
    /* Enable all entries we parsed. */
    for (size_t i = 1; i < DmntCheatManager::MaxCheatCount; i++) {
        if (g_cheat_entries[i].definition.num_opcodes > 0) {
            g_cheat_entries[i].enabled = g_enable_cheats_by_default;
        }
    }
    
    return true;
}

bool DmntCheatManager::LoadCheats(u64 title_id, const u8 *build_id) {
    /* Reset existing entries. */
    ResetAllCheatEntries();
        
    FILE *f_cht = NULL;
    /* Open the file for title/build_id. */
    {
        char path[FS_MAX_PATH+1] = {0};
        snprintf(path, FS_MAX_PATH, "sdmc:/atmosphere/titles/%016lx/cheats/%02x%02x%02x%02x%02x%02x%02x%02x.txt", title_id,
                build_id[0], build_id[1], build_id[2], build_id[3], build_id[4], build_id[5], build_id[6], build_id[7]); 

        f_cht = fopen(path, "rb");
    }
    
    /* Check for NULL */
    if (f_cht == NULL) {
        return false;
    }
    ON_SCOPE_EXIT { fclose(f_cht); };
    
    /* Get file size. */
    fseek(f_cht, 0L, SEEK_END);
    const size_t cht_sz = ftell(f_cht);
    fseek(f_cht, 0L, SEEK_SET);
        
    /* Allocate cheat txt buffer. */
    char *cht_txt = (char *)malloc(cht_sz + 1);
    if (cht_txt == NULL) {
        return false;
    }
    ON_SCOPE_EXIT { free(cht_txt); };
    
    /* Read cheats into buffer. */
    if (fread(cht_txt, 1, cht_sz, f_cht) != cht_sz) {
        return false;
    }
    cht_txt[cht_sz] = 0;
        
    /* Parse cheat buffer. */
    return ParseCheats(cht_txt, strlen(cht_txt));
}

Result DmntCheatManager::GetCheatCount(u64 *out_count) {
    std::scoped_lock<HosMutex> lk(g_cheat_lock);
    
    if (!HasActiveCheatProcess()) {
        return ResultDmntCheatNotAttached;
    }
    
    *out_count = 0;
    for (size_t i = 0; i < DmntCheatManager::MaxCheatCount; i++) {
        if (g_cheat_entries[i].definition.num_opcodes > 0) {
            *out_count += 1;
        }
    }
    
    return 0;
}

Result DmntCheatManager::GetCheats(CheatEntry *cheats, size_t max_count, u64 *out_count, u64 offset) {
    std::scoped_lock<HosMutex> lk(g_cheat_lock);
    
    if (!HasActiveCheatProcess()) {
        return ResultDmntCheatNotAttached;
    }
    
    u64 count = 0;
    *out_count = 0;
    for (size_t i = 0; i < DmntCheatManager::MaxCheatCount && (*out_count) < max_count; i++) {
        if (g_cheat_entries[i].definition.num_opcodes > 0) {
            count++;
            if (count > offset) {
                cheats[(*out_count)++] = g_cheat_entries[i];
            }
        }
    }
    
    return 0;
}

Result DmntCheatManager::GetCheatById(CheatEntry *out_cheat, u32 cheat_id) {
    std::scoped_lock<HosMutex> lk(g_cheat_lock);
    
    if (!HasActiveCheatProcess()) {
        return ResultDmntCheatNotAttached;
    }
    
    const CheatEntry *entry = GetCheatEntryById(cheat_id);
    if (entry == nullptr || entry->definition.num_opcodes == 0) {
        return ResultDmntCheatUnknownChtId;
    }
    
    *out_cheat = *entry;
    return 0;
}

Result DmntCheatManager::ToggleCheat(u32 cheat_id) {
    std::scoped_lock<HosMutex> lk(g_cheat_lock);
    
    if (!HasActiveCheatProcess()) {
        return ResultDmntCheatNotAttached;
    }
    
    CheatEntry *entry = GetCheatEntryById(cheat_id);
    if (entry == nullptr || entry->definition.num_opcodes == 0) {
        return ResultDmntCheatUnknownChtId;
    }
    
    if (cheat_id == 0) {
        return ResultDmntCheatCannotDisableMasterCheat;
    }
    
    entry->enabled = !entry->enabled;
    
    /* Trigger a VM reload. */
    g_needs_reload_vm_program = true;
    return 0;
}

Result DmntCheatManager::AddCheat(u32 *out_id, CheatDefinition *def, bool enabled) {
    std::scoped_lock<HosMutex> lk(g_cheat_lock);
    
    if (!HasActiveCheatProcess()) {
        return ResultDmntCheatNotAttached;
    }
    
    if (def->num_opcodes == 0 || def->num_opcodes > sizeof(def->opcodes)/sizeof(def->opcodes[0])) {
        return ResultDmntCheatInvalidCheat;
    }
    
    CheatEntry *new_entry = GetFreeCheatEntry();
    if (new_entry == nullptr) {
        return ResultDmntCheatOutOfCheats;
    }
    
    new_entry->enabled = enabled;
    new_entry->definition = *def;
    
    /* Trigger a VM reload. */
    g_needs_reload_vm_program = true;
    return 0;
}

Result DmntCheatManager::RemoveCheat(u32 cheat_id) {
    std::scoped_lock<HosMutex> lk(g_cheat_lock);
    
    if (!HasActiveCheatProcess()) {
        return ResultDmntCheatNotAttached;
    }
    
    if (cheat_id >= DmntCheatManager::MaxCheatCount) {
        return ResultDmntCheatUnknownChtId;
    }
    
    ResetCheatEntry(cheat_id);
    
    /* Trigger a VM reload. */
    g_needs_reload_vm_program = true;
    return 0;
}

Result DmntCheatManager::GetFrozenAddressCount(u64 *out_count) {
    std::scoped_lock<HosMutex> lk(g_cheat_lock);
    
    if (!HasActiveCheatProcess()) {
        return ResultDmntCheatNotAttached;
    }
    
    *out_count = g_frozen_addresses_map.size();
    return 0;
}

Result DmntCheatManager::GetFrozenAddresses(FrozenAddressEntry *frz_addrs, size_t max_count, u64 *out_count, u64 offset) {
    std::scoped_lock<HosMutex> lk(g_cheat_lock);
    
    if (!HasActiveCheatProcess()) {
        return ResultDmntCheatNotAttached;
    }
    
    u64 count = 0;
    *out_count = 0;
    for (auto const& [address, value] : g_frozen_addresses_map) {
        if ((*out_count) >= max_count) {
            break;
        }
        
        count++;
        if (count > offset) {
            const u64 cur_ind = (*out_count)++;
            frz_addrs[cur_ind].address = address;
            frz_addrs[cur_ind].value = value;
        }
    }
    
    return 0;
}

Result  DmntCheatManager::GetFrozenAddress(FrozenAddressEntry *frz_addr, u64 address) {
    std::scoped_lock<HosMutex> lk(g_cheat_lock);
    
    if (!HasActiveCheatProcess()) {
        return ResultDmntCheatNotAttached;
    }
    
    const auto it = g_frozen_addresses_map.find(address);
    if (it == g_frozen_addresses_map.end()) {
        return ResultDmntCheatAddressNotFrozen;
    }
    
    frz_addr->address = it->first;
    frz_addr->value = it->second;
    return 0;
}

Result DmntCheatManager::EnableFrozenAddress(u64 *out_value, u64 address, u64 width) {
    std::scoped_lock<HosMutex> lk(g_cheat_lock);
    
    if (!HasActiveCheatProcess()) {
        return ResultDmntCheatNotAttached;
    }
    
    if (g_frozen_addresses_map.size() >= DmntCheatManager::MaxFrozenAddressCount) {
        return ResultDmntCheatTooManyFrozenAddresses;
    }
    
    const auto it = g_frozen_addresses_map.find(address);
    if (it != g_frozen_addresses_map.end()) {
        return ResultDmntCheatAddressAlreadyFrozen;
    }
    
    Result rc;
    FrozenAddressValue value = {0};
    value.width = width;
    if (R_FAILED((rc = ReadCheatProcessMemoryForVm(address, &value.value, width)))) {
        return rc;
    }
    
    g_frozen_addresses_map[address] = value;
    *out_value = value.value;
    return 0;
}

Result DmntCheatManager::DisableFrozenAddress(u64 address) {
    std::scoped_lock<HosMutex> lk(g_cheat_lock);
    
    if (!HasActiveCheatProcess()) {
        return ResultDmntCheatNotAttached;
    }
    
    const auto it = g_frozen_addresses_map.find(address);
    if (it == g_frozen_addresses_map.end()) {
        return ResultDmntCheatAddressNotFrozen;
    }
    
    g_frozen_addresses_map.erase(address);
    return 0;
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
    
    /* Close the current application, if it's open. */
    CloseActiveCheatProcess();
    /* Wait to not have debug events thread. */
    WaitDebugEventsThread();
    
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
    
    /* Read cheats off the SD. */
    /* This is allowed to fail. We may not have any cheats. */
    LoadCheats(g_cheat_process_metadata.title_id, g_cheat_process_metadata.main_nso_build_id);
    
    /* Open a debug handle. */
    if (R_FAILED((rc = svcDebugActiveProcess(&g_cheat_process_debug_hnd, g_cheat_process_metadata.process_id)))) {
        return rc;
    }

    /* Start debug events thread. */
    StartDebugEventsThread();
        
    /* Signal to our fans. */
    g_cheat_process_event->Signal();
    
    return rc;
}

void DmntCheatManager::OnNewApplicationLaunch() {
    std::scoped_lock<HosMutex> lk(g_cheat_lock);
    Result rc;
    
    /* Close the current application, if it's open. */
    CloseActiveCheatProcess();
    
    /* Wait to not have debug events thread. */
    WaitDebugEventsThread();
    
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
        
    /* Read cheats off the SD. */
    if (!LoadCheats(g_cheat_process_metadata.title_id, g_cheat_process_metadata.main_nso_build_id)) {
        /* If we don't have cheats, or cheats are malformed, don't attach. */
        StartDebugProcess(g_cheat_process_metadata.process_id);
        g_cheat_process_metadata.process_id = 0;
        return;
    }
    
    /* Open a debug handle. */
    if (R_FAILED((rc = svcDebugActiveProcess(&g_cheat_process_debug_hnd, g_cheat_process_metadata.process_id)))) {
        fatalSimple(rc);
    }
    
    /* Start the process. */
    StartDebugProcess(g_cheat_process_metadata.process_id);

    /* Start debug events thread. */
    StartDebugEventsThread();
        
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
                /* Execute VM. */
                if (!g_needs_reload_vm_program || (g_cheat_vm->LoadProgram(g_cheat_entries, DmntCheatManager::MaxCheatCount))) {
                    /* Program: reloaded. */
                    g_needs_reload_vm_program = false;
                    
                    /* Execute program if it's present. */
                    if (g_cheat_vm->GetProgramSize() != 0) {
                        g_cheat_vm->Execute(&g_cheat_process_metadata);
                    }
                }
                
                /* Apply frozen addresses. */
                for (auto const& [address, value] : g_frozen_addresses_map) {
                    WriteCheatProcessMemoryForVm(address, &value.value, value.width);
                }
            }
        }
        
        constexpr u64 ONE_SECOND = 1000000000ul;
        constexpr u64 NUM_TIMES = 12;
        constexpr u64 DELAY = ONE_SECOND / NUM_TIMES;
        svcSleepThread(DELAY);
    }
}

void DmntCheatManager::DebugEventsThread(void *arg) {
    while (R_SUCCEEDED(svcWaitSynchronizationSingle(g_cheat_process_debug_hnd, U64_MAX))) {
        std::scoped_lock<HosMutex> lk(g_cheat_lock);
        
        /* Handle any pending debug events. */
        if (HasActiveCheatProcess()) {
            ContinueCheatProcess();
        }
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
    
    /* Learn whether we should enable cheats by default. */
    {
        u8 en;
        if (R_SUCCEEDED(setsysGetSettingsItemValue("atmosphere", "dmnt_cheats_enabled_by_default", &en, sizeof(en)))) {
            g_enable_cheats_by_default = (en != 0);
        }
    }
    
    /* Spawn application detection thread, spawn cheat vm thread. */
    if (R_FAILED(g_detect_thread.Initialize(&DmntCheatManager::DetectThread, nullptr, 0x4000, 39))) {
        std::abort();
    }
    
    if (R_FAILED(g_vm_thread.Initialize(&DmntCheatManager::VmThread, nullptr, 0x4000, 48))) {
        std::abort();
    }
    
    /* Start threads. */
    if (R_FAILED(g_detect_thread.Start()) || R_FAILED(g_vm_thread.Start())) {
        std::abort();
    }
}
