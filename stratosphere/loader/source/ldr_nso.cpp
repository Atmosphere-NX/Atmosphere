#include <switch.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <picosha2.hpp>
#include "lz4.h"
#include "ldr_nso.hpp"
#include "ldr_map.hpp"
#include "ldr_random.hpp"

static NsoUtils::NsoHeader g_nso_headers[NSO_NUM_MAX] = {0};
static bool g_nso_present[NSO_NUM_MAX] = {0};

static char g_nso_path[FS_MAX_PATH] = {0};

FILE *NsoUtils::OpenNsoFromExeFS(unsigned int index) {
    std::fill(g_nso_path, g_nso_path + FS_MAX_PATH, 0);
    snprintf(g_nso_path, FS_MAX_PATH, "code:/%s", NsoUtils::GetNsoFileName(index));
    return fopen(g_nso_path, "rb");
}

FILE *NsoUtils::OpenNsoFromSdCard(unsigned int index, u64 title_id) {  
    std::fill(g_nso_path, g_nso_path + FS_MAX_PATH, 0);
    snprintf(g_nso_path, FS_MAX_PATH, "sdmc:/atmosphere/titles/%016lx/exefs/%s", title_id, NsoUtils::GetNsoFileName(index));
    return fopen(g_nso_path, "rb");
}

FILE *NsoUtils::OpenNso(unsigned int index, u64 title_id) {
    FILE *f_out = OpenNsoFromSdCard(index, title_id);
    if (f_out != NULL) {
        return f_out;
    }
    return OpenNsoFromExeFS(index);
}

bool NsoUtils::IsNsoPresent(unsigned int index) {
    return g_nso_present[index];
}


unsigned char *NsoUtils::GetNsoBuildId(unsigned int index) {
    if (g_nso_present[index]) {
        return g_nso_headers[index].build_id;
    }
    return NULL;
}

Result NsoUtils::LoadNsoHeaders(u64 title_id) {
    FILE *f_nso;
    
    /* Zero out the cache. */
    std::fill(g_nso_present, g_nso_present + NSO_NUM_MAX, false);
    std::fill(g_nso_headers, g_nso_headers + NSO_NUM_MAX, (const NsoUtils::NsoHeader &){0});
    
    for (unsigned int i = 0; i < NSO_NUM_MAX; i++) {
        f_nso = OpenNso(i, title_id);
        if (f_nso != NULL) {
            if (fread(&g_nso_headers[i], sizeof(NsoUtils::NsoHeader), 1, f_nso) != sizeof(NsoUtils::NsoHeader)) {
                return 0xA09;
            }
            g_nso_present[i] = true;
            fclose(f_nso);
            continue;
        }
        if (1 < i && i < 12) {
            /* If we failed to open a subsdk, there are no more subsdks. */
            i = 11;
        }
    }
    
    return 0x0;
}

Result NsoUtils::ValidateNsoLoadSet() {
    /* We *must* have a "main" NSO. */
    if (!g_nso_present[1]) {
        return 0xA09;
    }
    
    /* Behavior switches depending on whether we have an rtld. */
    if (g_nso_present[0]) {
         /* If we have an rtld, dst offset for .text must be 0 for all other NSOs. */
        for (unsigned int i = 0; i < NSO_NUM_MAX; i++) {
            if (g_nso_present[i] && g_nso_headers[i].segments[0].dst_offset != 0) {
                return 0xA09;
            }
        }
    } else {
        /* If we don't have an rtld, we must ONLY have a main. */
        for (unsigned int i = 2; i < NSO_NUM_MAX; i++) {
            if (g_nso_present[i]) {
                return 0xA09;
            }
        }
        /* That main's .text must be at dst_offset 0. */
        if (g_nso_headers[1].segments[0].dst_offset != 0) {
            return 0xA09;
        }
    }
    
    return 0x0;
}


Result NsoUtils::CalculateNsoLoadExtents(u32 addspace_type, u32 args_size, NsoLoadExtents *extents) {
    *extents = (const NsoUtils::NsoLoadExtents){0};
    /* Calculate base offsets. */
    for (unsigned int i = 0; i < NSO_NUM_MAX; i++) {
        if (g_nso_present[i]) {
            extents->nso_addresses[i] = extents->total_size;
            u32 text_end = g_nso_headers[i].segments[0].dst_offset + g_nso_headers[i].segments[0].decomp_size;
            u32 ro_end = g_nso_headers[i].segments[1].dst_offset + g_nso_headers[i].segments[1].decomp_size;
            u32 rw_end = g_nso_headers[i].segments[2].dst_offset + g_nso_headers[i].segments[2].decomp_size + g_nso_headers[i].segments[2].align_or_total_size;
            extents->nso_sizes[i] = text_end;
            if (extents->nso_sizes[i] < ro_end) {
                extents->nso_sizes[i] = ro_end;
            }
            if (extents->nso_sizes[i] < rw_end) {
                extents->nso_sizes[i] = rw_end;
            }
            extents->nso_sizes[i] += 0xFFF;
            extents->nso_sizes[i] &= ~0xFFFULL;
            extents->total_size += extents->nso_sizes[i];
            if (args_size && !extents->args_size) {
                extents->args_address = extents->total_size;
                /* What the fuck? Where does 0x9007 come from? */
                extents->args_size = (2 * args_size + 0x9007);
                extents->args_size &= ~0xFFFULL;
            }
        }
    }
    
    /* Calculate ASLR extents for address space type. */
    u64 addspace_start, addspace_size;
    if (kernelAbove200()) {
        switch (addspace_type & 0xE) {
            case 0:
            case 4:
                addspace_start = 0x200000ULL;
                addspace_size = 0x3FE00000ULL;
                break;
            case 2:
                addspace_start = 0x8000000ULL;
                addspace_size = 0x78000000ULL;
                break;
            case 6:
                addspace_start = 0x8000000ULL;
                addspace_size = 0x7FF8000000ULL;
                break;
            default:
                /* TODO: Panic. */
                return 0xD001;
        }
    } else {
        if (addspace_type & 2) {
            addspace_start = 0x8000000ULL;
            addspace_size = 0x78000000ULL;
        } else {
            addspace_start = 0x200000ULL;
            addspace_size = 0x3FE00000ULL;
        }
    }
    if (extents->total_size > addspace_size) {
        return 0xD001;
    }
    
    u64 aslr_slide = 0;
    if (addspace_type & 0x20) {
        aslr_slide = RandomUtils::GetRandomU64((addspace_size - extents->total_size) >> 21) << 21;
    }
    
    extents->base_address = addspace_start + aslr_slide;
    for (unsigned int i = 0; i < NSO_NUM_MAX; i++) {
        if (g_nso_present[i]) {
            extents->nso_addresses[i] += extents->base_address;
        }
    }
    if (extents->args_address) {
        extents->args_address += extents->base_address;
    }
    
    return 0x0;
}


Result NsoUtils::LoadNsoSegment(unsigned int index, unsigned int segment, FILE *f_nso, u8 *map_base, u8 *map_end) {
    bool is_compressed = ((g_nso_headers[index].flags >> segment) & 1) != 0;
    bool check_hash = ((g_nso_headers[index].flags >> (segment + 3)) & 1) != 0;
    size_t out_size = g_nso_headers[index].segments[segment].decomp_size;
    size_t size = is_compressed ? g_nso_headers[index].compressed_sizes[segment] : out_size;
    if (size > out_size) {
        return 0xA09;
    }
    if ((u32)(size | out_size) >> 31) {
        return 0xA09;
    }
    
    u8 *dst_addr = map_base + g_nso_headers[index].segments[segment].dst_offset;
    u8 *load_addr = is_compressed ? map_end - size : dst_addr;
    if (fread(load_addr, 1, size, f_nso) != size) {
        return 0xA09;
    }
    
    if (is_compressed) {
        if (LZ4_decompress_safe((char *)load_addr, (char *)dst_addr, size, out_size) != (int)out_size) {
            return 0xA09;
        }
    }
    
    if (check_hash) {
        u8 hash[0x20] = {0};
        picosha2::hash256(dst_addr, dst_addr + out_size, hash, hash + sizeof(hash));
        if (std::memcmp(g_nso_headers[index].section_hashes[segment], hash, sizeof(hash))) {
            return 0xA09;
        }
    }
    
    return 0x0;
}

Result NsoUtils::LoadNsosIntoProcessMemory(Handle process_h, u64 title_id, NsoLoadExtents *extents, u8 *args, u32 args_size) {
    Result rc = 0xA09;
    for (unsigned int i = 0; i < NSO_NUM_MAX; i++) {
        if (g_nso_present[i]) {
            u64 map_addr = 0;
            if (R_FAILED((rc = MapUtils::LocateSpaceForMap(&map_addr, extents->nso_sizes[i])))) {
                return rc;
            }
            
            u8 *map_base = (u8 *)map_addr;
            
            if (R_FAILED((rc = svcMapProcessMemory(map_base, process_h, extents->nso_addresses[i], extents->nso_sizes[i])))) {
                return rc;
            }
            
            FILE *f_nso = OpenNso(i, title_id);
            if (f_nso == NULL) {
                /* Is there a better error to return here? */
                return 0xA09;
            }
            for (unsigned int seg = 0; seg < 3; seg++) {
                if (R_FAILED((rc = LoadNsoSegment(i, seg, f_nso, map_base, map_base + extents->nso_sizes[i])))) {
                    fclose(f_nso);
                    svcUnmapProcessMemory(map_base, process_h, extents->nso_addresses[i], extents->nso_sizes[i]);
                    return rc;
                }
            }
            fclose(f_nso);
            /* Zero out memory before .text. */
            u64 text_base = 0, text_start = g_nso_headers[i].segments[0].dst_offset;
            std::fill(map_base + text_base, map_base + text_start, 0);
            /* Zero out memory before .rodata. */
            u64 ro_base = text_start + g_nso_headers[i].segments[0].decomp_size, ro_start = g_nso_headers[i].segments[1].dst_offset;
            std::fill(map_base + ro_base, map_base + ro_start, 0);
            /* Zero out memory before .rwdata. */
            u64 rw_base = ro_start + g_nso_headers[i].segments[1].decomp_size, rw_start = g_nso_headers[i].segments[2].dst_offset;
            std::fill(map_base + rw_base, map_base + rw_start, 0);
            /* Zero out .bss. */
            u64 bss_base = rw_base + g_nso_headers[i].segments[2].decomp_size, bss_size = g_nso_headers[i].segments[2].align_or_total_size;
            std::fill(map_base + bss_base, map_base + bss_base + bss_size, 0);
            
            if (R_FAILED((rc = svcUnmapProcessMemory(map_base, process_h, extents->nso_addresses[i], extents->nso_sizes[i])))) {
                return rc;
            }
            
            for (unsigned int seg = 0; seg < 3; seg++) {
                u64 size = g_nso_headers[i].segments[seg].decomp_size;
                size += 0xFFF;
                size &= ~0xFFFULL;
                const static unsigned int segment_perms[3] = {5, 1, 3};
                if (R_FAILED((rc = svcSetProcessMemoryPermission(process_h, extents->nso_addresses[i] + g_nso_headers[i].segments[seg].dst_offset, size, segment_perms[seg])))) {
                    return rc;
                }
            }
        }
    }
    
    /* Map in arguments. */
    if (args != NULL && args_size) {
        u64 arg_map_addr = 0;
        if (R_FAILED((rc = MapUtils::LocateSpaceForMap(&arg_map_addr, extents->args_size)))) {
            return rc;
        }
        
        NsoArgument *arg_map_base = (NsoArgument *)arg_map_addr;
        
        if (R_FAILED((rc = svcMapProcessMemory(arg_map_base, process_h, extents->args_address, extents->args_size)))) {
            return rc;
        }
        
        arg_map_base->allocated_space = extents->args_size;
        arg_map_base->args_size = args_size;
        std::fill(arg_map_base->_0x8, arg_map_base->_0x8 + sizeof(arg_map_base->_0x8), 0);
        std::copy(args, args + args_size, arg_map_base->arguments);
        
        if (R_FAILED((rc = svcUnmapProcessMemory(arg_map_base, process_h, extents->args_address, extents->args_size)))) {
            return rc;
        }
        
        if (R_FAILED((rc = svcSetProcessMemoryPermission(process_h, extents->args_address, extents->args_size, 3)))) {
            return rc;
        }
    }
    
    return rc;
}
