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
#include <cstdio>

#include "ldr_map.hpp"

Result MapUtils::LocateSpaceForMap(u64 *out, u64 out_size) {
    if (kernelAbove200()) {
        return LocateSpaceForMapModern(out, out_size);
    } else {
        return LocateSpaceForMapDeprecated(out, out_size);
    }
}

Result MapUtils::LocateSpaceForMapModern(u64 *out, u64 out_size) {
    MemoryInfo mem_info = {};
    AddressSpaceInfo address_space = {};
    u32 page_info = 0;
    u64 cur_base = 0, cur_end = 0;
    Result rc;
    
    if (R_FAILED((rc = GetAddressSpaceInfo(&address_space, CUR_PROCESS_HANDLE)))) {
        return rc;
    }
    
    cur_base = address_space.addspace_base;
    
    rc = ResultKernelOutOfMemory;
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
                std::abort();
            }
            if (mem_info.type == 0 && mem_info.addr - cur_base + mem_info.size >= out_size) {
                *out = cur_base;
                return ResultSuccess;
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
    MemoryInfo mem_info = {};
    u32 page_info = 0;
    Result rc;
    
    u64 cur_base = 0x8000000ULL;
    if (R_FAILED((rc = svcQueryMemory(&mem_info, &page_info, cur_base)))) {
        return rc;
    }
    
    rc = ResultKernelOutOfMemory;
    while (true) {
        if (mem_info.type == 0x10) {
            return rc;
        }
        if (mem_info.type == 0 && mem_info.addr - cur_base + mem_info.size >= out_size) {
            *out = cur_base;
            return ResultSuccess;
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

Result MapUtils::GetAddressSpaceInfo(AddressSpaceInfo *out, Handle process_h) {
    Result rc;
    if (R_FAILED((rc = svcGetInfo(&out->heap_base, 4, process_h, 0)))) {
        return rc;
    }
    if (R_FAILED((rc = svcGetInfo(&out->heap_size, 5, process_h, 0)))) {
        return rc;
    }
    if (R_FAILED((rc = svcGetInfo(&out->map_base, 2, process_h, 0)))) {
        return rc;
    }
    if (R_FAILED((rc = svcGetInfo(&out->map_size, 3, process_h, 0)))) {
        return rc;
    }
    if (R_FAILED((rc = svcGetInfo(&out->addspace_base, 12, process_h, 0)))) {
        return rc;
    }
    if (R_FAILED((rc = svcGetInfo(&out->addspace_size, 13, process_h, 0)))) {
        return rc;
    }
    out->heap_end = out->heap_base + out->heap_size;
    out->map_end = out->map_base + out->map_size;
    out->addspace_end = out->addspace_base + out->addspace_size;
    return ResultSuccess;
}