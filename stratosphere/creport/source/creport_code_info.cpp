#include <switch.h>
#include <cstring>

#include "creport_code_info.hpp"

void CodeList::ReadCodeRegionsFromProcess(Handle debug_handle, u64 pc, u64 lr) {
    u64 code_base;
    
    /* Guess that either PC or LR will point to a code region. This could be false. */
    if (!TryFindCodeRegion(debug_handle, pc, &code_base) && !TryFindCodeRegion(debug_handle, lr, &code_base)) {
        return;
    }
    
    u64 cur_ptr = code_base;
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
            GetCodeInfoName(debug_handle, mi.addr + mi.size, this->code_infos[this->code_count].name);
            GetCodeInfoBuildId(debug_handle, mi.addr + mi.size, this->code_infos[this->code_count].build_id);
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
    if (R_FAILED(svcQueryDebugProcessMemory(&mi, &pi, debug_handle, guess)) || mi.perm != Perm_Rx) {
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
        
        guess -= 4;
    }
    return false;
}

void CodeList::GetCodeInfoName(u64 debug_handle, u64 rodata_addr, char *name) {
    char name_in_proc[0x200];
    
    /* Clear name. */
    memset(name, 0, 0x20);
    
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