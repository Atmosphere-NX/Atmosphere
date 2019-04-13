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
 
#include <switch.h>
#include <cstring>

#include "creport_code_info.hpp"
#include "creport_crash_report.hpp"

void CodeList::SaveToFile(FILE *f_report) {
    fprintf(f_report, "    Number of Code Regions:      %u\n", this->code_count);
    for (unsigned int i = 0; i < this->code_count; i++) {
        fprintf(f_report, "    Code Region %02u:\n", i);
        fprintf(f_report, "        Address:                 %016lx-%016lx\n", this->code_infos[i].start_address, this->code_infos[i].end_address);
        if (this->code_infos[i].name[0]) {    
            fprintf(f_report, "        Name:                    %s\n", this->code_infos[i].name);
        }
        CrashReport::Memdump(f_report, "        Build Id:                ", this->code_infos[i].build_id, sizeof(this->code_infos[i].build_id));
    }
}

void CodeList::ReadCodeRegionsFromThreadInfo(Handle debug_handle, const ThreadInfo *thread) {
    u64 code_base;
    
    /* Try to add the thread's PC. */
    if (TryFindCodeRegion(debug_handle, thread->GetPC(), &code_base)) {
        AddCodeRegion(debug_handle, code_base);
    }
    
    /* Try to add the thread's LR. */
    if (TryFindCodeRegion(debug_handle, thread->GetLR(), &code_base)) {
        AddCodeRegion(debug_handle, code_base);
    }
    
    /* Try to add all the addresses in the thread's stacktrace. */
    for (u32 i = 0; i < thread->GetStackTraceSize(); i++) {
        if (TryFindCodeRegion(debug_handle, thread->GetStackTrace(i), &code_base)) {
            AddCodeRegion(debug_handle, code_base);
        }
    }
}

void CodeList::AddCodeRegion(u64 debug_handle, u64 code_address) {
    /* Check whether we already have this code region. */
    for (size_t i = 0; i < this->code_count; i++) {
        if (this->code_infos[i].start_address <= code_address && code_address < this->code_infos[i].end_address) {
            return;
        }
    }
    
    /* Add all contiguous code regions. */
    u64 cur_ptr = code_address;
    while (this->code_count < max_code_count) {
        MemoryInfo mi;
        u32 pi;
        if (R_FAILED(svcQueryDebugProcessMemory(&mi, &pi, debug_handle, cur_ptr))) {
            break;
        }
        
        if (mi.perm == Perm_Rx) {
            /* Parse CodeInfo. */
            this->code_infos[this->code_count].start_address = mi.addr;
            this->code_infos[this->code_count].end_address = mi.addr + mi.size; 
            GetCodeInfoName(debug_handle, mi.addr, mi.addr + mi.size, this->code_infos[this->code_count].name);
            GetCodeInfoBuildId(debug_handle, mi.addr + mi.size, this->code_infos[this->code_count].build_id);
            if (this->code_infos[this->code_count].name[0] == '\x00') {
                snprintf(this->code_infos[this->code_count].name, 0x1F, "[%02x%02x%02x%02x]", this->code_infos[this->code_count].build_id[0], 
                                                                                             this->code_infos[this->code_count].build_id[1], 
                                                                                             this->code_infos[this->code_count].build_id[2], 
                                                                                             this->code_infos[this->code_count].build_id[3]);
            }
            this->code_count++;
        }
        
        /* If we're out of readable memory, we're done reading code. */
        if (mi.type == MemType_Unmapped || mi.type == MemType_Reserved) {
            break;
        }
        
        /* Verify we're not getting stuck in an infinite loop. */
        if (mi.size == 0 || U64_MAX - mi.size <= cur_ptr) {
            break;
        }
            
        cur_ptr += mi.size;
    }
}

bool CodeList::TryFindCodeRegion(Handle debug_handle, u64 guess, u64 *address) {
    MemoryInfo mi;
    u32 pi;
    if (R_FAILED(svcQueryDebugProcessMemory(&mi, &pi, debug_handle, guess))) {
        return false;
    }
    
    if (mi.perm == Perm_Rw) {
        guess = mi.addr - 4;
        if (R_FAILED(svcQueryDebugProcessMemory(&mi, &pi, debug_handle, guess))) {
            return false;
        }
    }
    
    if (mi.perm == Perm_R) {
        guess = mi.addr - 4;
        if (R_FAILED(svcQueryDebugProcessMemory(&mi, &pi, debug_handle, guess))) {
            return false;
        }
    }
    
    if (mi.perm != Perm_Rx) {
        return false;
    }
    
    /* Iterate backwards until we find the memory before the code region. */
    while (mi.addr > 0) {
        if (R_FAILED(svcQueryDebugProcessMemory(&mi, &pi, debug_handle, guess))) {
            return false;
        }
        
        if (mi.type == MemType_Unmapped) {
            /* Code region will be at the end of the unmapped region preceding it. */
            *address = mi.addr + mi.size;
            return true;
        }
        
        guess = mi.addr - 4;
    }
    return false;
}

void CodeList::GetCodeInfoName(u64 debug_handle, u64 rx_address, u64 rodata_addr, char *name) {
    char name_in_proc[0x200];
    
    /* Clear name. */
    memset(name, 0, 0x20);
    
    /* Check whether this NSO *has* a name... */
    {
        u64 rodata_start[0x20/sizeof(u64)];
        MemoryInfo mi;
        u32 pi;
        u64 rw_address;
        
        /* Verify .rodata is read-only. */
        if (R_FAILED(svcQueryDebugProcessMemory(&mi, &pi, debug_handle, rodata_addr)) || mi.perm != Perm_R) {
            return;
        }
        
        /* rwdata is after rodata. */
        rw_address = mi.addr + mi.size;
        
        /* Read start of .rodata. */
        if (R_FAILED(svcReadDebugProcessMemory(rodata_start, debug_handle, rodata_addr, sizeof(rodata_start)))) {
            return;
        }
        
        /* Check if name section is present. */
        if (rodata_start[0] == (rw_address - rx_address)) {
            return;
        }
    }
    
    /* Read name out of .rodata. */
    if (R_FAILED(svcReadDebugProcessMemory(name_in_proc, debug_handle, rodata_addr + 8, sizeof(name_in_proc)))) {
        return;
    }
        
    /* Start after last slash in path. */
    int ofs = strnlen(name_in_proc, sizeof(name_in_proc));
    while (ofs >= 0 && name_in_proc[ofs] != '/' && name_in_proc[ofs] != '\\') {
        ofs--;
    }
    
    strncpy(name, name_in_proc + ofs + 1, 0x20);
    name[0x1F] = '\x00';
}

void CodeList::GetCodeInfoBuildId(u64 debug_handle, u64 rodata_addr, u8 *build_id) {
    MemoryInfo mi;
    u32 pi;
    
    /* Clear build id. */
    memset(build_id, 0, 0x20);
    
    /* Verify .rodata is read-only. */
    if (R_FAILED(svcQueryDebugProcessMemory(&mi, &pi, debug_handle, rodata_addr)) || mi.perm != Perm_R) {
        return;
    }
    
    /* We want to read the last two pages of .rodata. */
    u8 last_pages[0x2000];
    size_t last_pages_size = mi.size >= 0x2000 ? 0x2000 : 0x1000;
    if (R_FAILED(svcReadDebugProcessMemory(last_pages, debug_handle, mi.addr + mi.size - last_pages_size, last_pages_size))) {
        return;
    }
    
    /* Find GNU\x00 to locate start of Build ID. */
    for (int ofs = last_pages_size - 0x24; ofs >= 0; ofs--) {
        if (memcmp(last_pages + ofs, "GNU\x00", 4) == 0) {
            memcpy(build_id, last_pages + ofs + 4, 0x20);
        }
    }
}


const char *CodeList::GetFormattedAddressString(u64 address) {
    memset(this->address_str_buf, 0, sizeof(this->address_str_buf));
    for (unsigned int i = 0; i < this->code_count; i++) {
        if (this->code_infos[i].start_address <= address && address < this->code_infos[i].end_address) {
            snprintf(this->address_str_buf, sizeof(this->address_str_buf) - 1, "%016lx (%s + 0x%lx)", address, this->code_infos[i].name, address - this->code_infos[i].start_address);
            return this->address_str_buf;
        }
    }
    snprintf(this->address_str_buf, sizeof(this->address_str_buf) - 1, "%016lx", address);
    return this->address_str_buf;
}