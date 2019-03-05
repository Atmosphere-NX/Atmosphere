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
#include <cstdio>

#include "ldr_map.hpp"
#include "ldr_random.hpp"

Result MapUtils::LocateSpaceForMap(u64 *out, u64 out_size) {
    if (kernelAbove200()) {
        return LocateSpaceForMapModern(out, out_size);
    } else {
        return LocateSpaceForMapDeprecated(out, out_size);
    }
}


Result MapUtils::MapCodeMemoryForProcess(Handle process_h, bool is_64_bit_address_space, u64 base_address, u64 size, u64 *out_code_memory_address) {
    if (kernelAbove200()) {
        return MapCodeMemoryForProcessModern(process_h, base_address, size, out_code_memory_address);
    } else {
        return MapCodeMemoryForProcessDeprecated(process_h, is_64_bit_address_space, base_address, size, out_code_memory_address);
    }
}

Result MapUtils::LocateSpaceForMapModern(u64 *out, u64 out_size) {
    MemoryInfo mem_info = {0};
    AddressSpaceInfo address_space = {0};
    u32 page_info = 0;
    u64 cur_base = 0, cur_end = 0;
    Result rc;
    
    if (R_FAILED((rc = GetAddressSpaceInfo(&address_space, CUR_PROCESS_HANDLE)))) {
        return rc;
    }
    
    cur_base = address_space.addspace_base;
    
    rc = 0xD001;
    cur_end = cur_base + out_size;
    if (cur_end <= cur_base) {
        return rc;
    }
    
    while (true) {
        if (address_space.heap_size && (address_space.heap_base <= cur_end - 1 && cur_base <= address_space.heap_end - 1)) {
            /* If we overlap the heap region, go to the end of the heap region. */
            if (cur_base == address_space.heap_end) {
                return rc;
            }
            cur_base = address_space.heap_end;
        } else if (address_space.map_size && (address_space.map_base <= cur_end - 1 && cur_base <= address_space.map_end - 1)) {
            /* If we overlap the map region, go to the end of the map region. */
            if (cur_base == address_space.map_end) {
                return rc;
            }
            cur_base = address_space.map_end;
        } else {
            if (R_FAILED(svcQueryMemory(&mem_info, &page_info, cur_base))) {
                /* TODO: panic. */
            }
            if (mem_info.type == 0 && mem_info.addr - cur_base + mem_info.size >= out_size) {
                *out = cur_base;
                return 0x0;
            }
            if (mem_info.addr + mem_info.size <= cur_base) {
                return rc;
            }
            cur_base = mem_info.addr + mem_info.size;
            if (cur_base >= address_space.addspace_end) {
                return rc;
            }
        }
        cur_end = cur_base + out_size;
        if (cur_base + out_size <= cur_base) {
            return rc;
        }
    }   
}


Result MapUtils::LocateSpaceForMapDeprecated(u64 *out, u64 out_size) {
    MemoryInfo mem_info = {0};
    u32 page_info = 0;
    Result rc;
    
    u64 cur_base = 0x8000000ULL;
    if (R_FAILED((rc = svcQueryMemory(&mem_info, &page_info, cur_base)))) {
        return rc;
    }
    
    rc = 0xD001;
    while (true) {
        if (mem_info.type == 0x10) {
                return rc;
        }
        if (mem_info.type == 0 && mem_info.addr - cur_base + mem_info.size >= out_size) {
            *out = cur_base;
            return 0x0;
        }
        u64 mem_end = mem_info.addr + mem_info.size;
        if (mem_end < cur_base) {
            return rc;
        }
        if (mem_end >> 31) {
            break;
        }
        cur_base = mem_end;
        if (R_FAILED((rc = svcQueryMemory(&mem_info, &page_info, cur_base)))) {
            return rc;
        }
    }
    return rc;
}

Result MapUtils::MapCodeMemoryForProcessModern(Handle process_h, u64 base_address, u64 size, u64 *out_code_memory_address) {
    AddressSpaceInfo address_space = {0};
    Result rc;
    
    if (R_FAILED((rc = GetAddressSpaceInfo(&address_space, CUR_PROCESS_HANDLE)))) {
        return rc;
    }

    if (size > address_space.addspace_size) {
        return 0x6609;
    }
    
    u64 try_address;
    for (unsigned int i = 0; i < 0x200; i++) {
        while (true) {
            try_address = address_space.addspace_base + (RandomUtils::GetRandomU64((u64)(address_space.addspace_size - size) >> 12) << 12);
            if (address_space.heap_size && (address_space.heap_base <= try_address + size - 1 && try_address <= address_space.heap_end - 1)) {
                continue;
            }
            if (address_space.map_size && (address_space.map_base <= try_address + size - 1 && try_address <= address_space.map_end - 1)) {
                continue;
            }
            break;
        }
        rc = svcMapProcessCodeMemory(process_h, try_address, base_address, size);
        if (rc != 0xD401) {
            break;
        }
    }
    if (R_SUCCEEDED(rc)) {
        *out_code_memory_address = try_address;
    }
    return rc;
}

Result MapUtils::MapCodeMemoryForProcessDeprecated(Handle process_h, bool is_64_bit_address_space, u64 base_address, u64 size, u64 *out_code_memory_address) {
    Result rc;
    u64 addspace_base, addspace_size;
    if (is_64_bit_address_space) {
        addspace_base = 0x8000000ULL;
        addspace_size = 0x78000000ULL;
    } else {
        addspace_base = 0x200000ULL;
        addspace_size = 0x3FE0000ULL;
    }
    
    if (size > addspace_size) {
        return 0x6609;
    }
        
    u64 try_address;
    for (unsigned int i = 0; i < 0x200; i++) {
        try_address = addspace_base + (RandomUtils::GetRandomU64((u64)(addspace_size - size) >> 12) << 12);
        rc = svcMapProcessCodeMemory(process_h, try_address, base_address, size);
        if (rc != 0xD401) {
            break;
        }
    }
    if (R_SUCCEEDED(rc)) {
        *out_code_memory_address = try_address;
    }
    return rc;
}

Result MapUtils::GetAddressSpaceInfo(AddressSpaceInfo *out, Handle process_h) {
    Result rc;
    if (R_FAILED((rc = svcGetInfo(&out->heap_base, 4, CUR_PROCESS_HANDLE, 0)))) {
        return rc;
    }
    if (R_FAILED((rc = svcGetInfo(&out->heap_size, 5, CUR_PROCESS_HANDLE, 0)))) {
        return rc;
    }
    if (R_FAILED((rc = svcGetInfo(&out->map_base, 2, CUR_PROCESS_HANDLE, 0)))) {
        return rc;
    }
    if (R_FAILED((rc = svcGetInfo(&out->map_size, 3, CUR_PROCESS_HANDLE, 0)))) {
        return rc;
    }
    if (R_FAILED((rc = svcGetInfo(&out->addspace_base, 12, CUR_PROCESS_HANDLE, 0)))) {
        return rc;
    }
    if (R_FAILED((rc = svcGetInfo(&out->addspace_size, 13, CUR_PROCESS_HANDLE, 0)))) {
        return rc;
    }
    out->heap_end = out->heap_base + out->heap_size;
    out->map_end = out->map_base + out->map_size;
    out->addspace_end = out->addspace_base + out->addspace_size;
    return 0;
}