#include <switch.h>

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
            /* TODO: Parse CodeInfo. */
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