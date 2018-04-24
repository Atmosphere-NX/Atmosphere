#include <switch.h>

#include "ldr_map.hpp"

Result MapUtils::LocateSpaceForMap(u64 *out, u64 out_size) {
    if (kernelAbove200()) {
        return LocateSpaceForMapModern(out, out_size);
    } else {
        return LocateSpaceForMapDeprecated(out, out_size);
    }
}


Result MapUtils::LocateSpaceForMapModern(u64 *out, u64 out_size) {
    MemoryInfo mem_info = {0};
    u32 page_info = 0;
    u64 heap_base = 0, heap_size = 0, heap_end = 0;
    u64 map_base = 0, map_size = 0, map_end = 0;
    u64 addspace_base = 0, addspace_size = 0, addspace_end = 0;
    u64 cur_base = 0, cur_end = 0;
    Result rc;
    
    if (R_FAILED((rc = svcGetInfo(&heap_base, 4, CUR_PROCESS_HANDLE, 0)))) {
        return rc;
    }
    if (R_FAILED((rc = svcGetInfo(&heap_size, 5, CUR_PROCESS_HANDLE, 0)))) {
        return rc;
    }
    if (R_FAILED((rc = svcGetInfo(&map_base, 2, CUR_PROCESS_HANDLE, 0)))) {
        return rc;
    }
    if (R_FAILED((rc = svcGetInfo(&map_size, 3, CUR_PROCESS_HANDLE, 0)))) {
        return rc;
    }
    if (R_FAILED((rc = svcGetInfo(&addspace_base, 12, CUR_PROCESS_HANDLE, 0)))) {
        return rc;
    }
    if (R_FAILED((rc = svcGetInfo(&addspace_size, 13, CUR_PROCESS_HANDLE, 0)))) {
        return rc;
    }
    heap_end = heap_base + heap_size;
    map_end = map_base + map_size;
    addspace_end = addspace_base + addspace_size;
    
    cur_base = addspace_base;
    
    rc = 0xD001;
    cur_end = cur_base + out_size;
    if (cur_end <= cur_base) {
        return rc;
    }
    
    if (heap_size) {
        if (map_size) {
            while (true) {
                if (cur_end - 1 < heap_base || heap_end - 1 < cur_base) {
                    if (cur_end - 1 < map_base || map_end - 1 < cur_base) {
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
                        if (cur_base >= addspace_end) {
                            return rc;
                        }
                    } else {
                        if (map_end == cur_base) {
                            return rc;
                        }
                        cur_base = map_end;
                    }
                } else {
                    if (heap_end == cur_base) {
                        return rc;
                    }
                    cur_base = heap_end;
                }
                cur_end = cur_base + out_size;
                if (cur_base + out_size <= cur_base) {
                    return rc;
                }
            }
        } else {
            while (true) {
                if (cur_end - 1 < heap_base || heap_end - 1 < cur_base) {
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
                    if (cur_base >= addspace_end) {
                        return rc;
                    }
                } else {
                    if (heap_end == cur_base) {
                        return rc;
                    }
                    cur_base = heap_end;
                }
                cur_end = cur_base + out_size;
                if (cur_base + out_size <= cur_base) {
                    return rc;
                }
            }
        }
    } else {
        while (cur_end > cur_base) {
            if (map_size && cur_end - 1 >= map_base && map_end - 1 >= cur_base) {
                if (cur_base == map_end) {
                    return rc;
                }
                cur_base = map_end;
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
                if (cur_base >= addspace_end) {
                    return rc;
                }
            }
            cur_end = cur_base + out_size;
        }
    }
    return rc;
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