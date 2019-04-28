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
#include <algorithm>
#include <cstdio>
#include <cstring>
#include "lz4.h"
#include "ldr_nso.hpp"
#include "ldr_map.hpp"
#include "ldr_patcher.hpp"
#include "ldr_content_management.hpp"

static NsoUtils::NsoHeader g_nso_headers[NSO_NUM_MAX] = {0};
static bool g_nso_present[NSO_NUM_MAX] = {0};

static char g_nso_path[FS_MAX_PATH] = {0};

FILE *NsoUtils::OpenNsoFromECS(unsigned int index, ContentManagement::ExternalContentSource *ecs) {
    std::fill(g_nso_path, g_nso_path + FS_MAX_PATH, 0);
    snprintf(g_nso_path, FS_MAX_PATH, "%s:/%s", ecs->mountpoint, NsoUtils::GetNsoFileName(index));
    return fopen(g_nso_path, "rb");
}

FILE *NsoUtils::OpenNsoFromHBL(unsigned int index) {
    std::fill(g_nso_path, g_nso_path + FS_MAX_PATH, 0);
    snprintf(g_nso_path, FS_MAX_PATH, "hbl:/%s", NsoUtils::GetNsoFileName(index));
    return fopen(g_nso_path, "rb");
}

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

bool NsoUtils::CheckNsoStubbed(unsigned int index, u64 title_id) {
    std::fill(g_nso_path, g_nso_path + FS_MAX_PATH, 0);
    snprintf(g_nso_path, FS_MAX_PATH, "sdmc:/atmosphere/titles/%016lx/exefs/%s.stub", title_id, NsoUtils::GetNsoFileName(index));
    FILE *f = fopen(g_nso_path, "rb");
    bool ret = (f != NULL);
    if (ret) {
        fclose(f);
    }
    return ret;
}

FILE *NsoUtils::OpenNso(unsigned int index, u64 title_id) {
    ContentManagement::ExternalContentSource *ecs = nullptr;
    if ((ecs = ContentManagement::GetExternalContentSource(title_id)) != nullptr) {
        return OpenNsoFromECS(index, ecs);
    }
    
    /* First, check HBL. */
    if (ContentManagement::ShouldOverrideContentsWithHBL(title_id)) {
        return OpenNsoFromHBL(index);
    }

    /* Next, check secondary override. */
    if (ContentManagement::ShouldOverrideContentsWithSD(title_id)) {
        FILE *f_out = OpenNsoFromSdCard(index, title_id);
        if (f_out != NULL) {
            return f_out;
        } else if (CheckNsoStubbed(index, title_id)) {
            return NULL;
        }
    }
    
    /* Finally, default to exefs. */
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
    std::fill(g_nso_headers, g_nso_headers + NSO_NUM_MAX, NsoUtils::NsoHeader{});
    
    for (unsigned int i = 0; i < NSO_NUM_MAX; i++) {
        f_nso = OpenNso(i, title_id);
        if (f_nso != NULL) {
            if (fread(&g_nso_headers[i], 1, sizeof(NsoUtils::NsoHeader), f_nso) != sizeof(NsoUtils::NsoHeader)) {
                return ResultLoaderInvalidNso;
            }
            g_nso_present[i] = true;
            fclose(f_nso);
            f_nso = NULL;
            continue;
        }
        if (1 < i && i < 12) {
            /* If we failed to open a subsdk, there are no more subsdks. */
            i = 11;
        }
    }
    
    return ResultSuccess;
}

Result NsoUtils::ValidateNsoLoadSet() {
    /* We *must* have a "main" NSO. */
    if (!g_nso_present[1]) {
        return ResultLoaderInvalidNso;
    }
    
    /* Behavior switches depending on whether we have an rtld. */
    if (g_nso_present[0]) {
         /* If we have an rtld, dst offset for .text must be 0 for all other NSOs. */
        for (unsigned int i = 0; i < NSO_NUM_MAX; i++) {
            if (g_nso_present[i] && g_nso_headers[i].segments[0].dst_offset != 0) {
                return ResultLoaderInvalidNso;
            }
        }
    } else {
        /* If we don't have an rtld, we must ONLY have a main. */
        for (unsigned int i = 2; i < NSO_NUM_MAX; i++) {
            if (g_nso_present[i]) {
                return ResultLoaderInvalidNso;
            }
        }
        /* That main's .text must be at dst_offset 0. */
        if (g_nso_headers[1].segments[0].dst_offset != 0) {
            return ResultLoaderInvalidNso;
        }
    }
    
    return ResultSuccess;
}


Result NsoUtils::CalculateNsoLoadExtents(u32 addspace_type, u32 args_size, NsoLoadExtents *extents) {
    *extents = {};

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
                extents->total_size += extents->args_size;
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
                return ResultKernelOutOfMemory;
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
        return ResultKernelOutOfMemory;
    }
    
    u64 aslr_slide = 0;
    if (addspace_type & 0x20) {
        aslr_slide = StratosphereRandomUtils::GetRandomU64((addspace_size - extents->total_size) >> 21) << 21;
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
    
    return ResultSuccess;
}



Result NsoUtils::LoadNsoSegment(u64 title_id, unsigned int index, unsigned int segment, FILE *f_nso, u8 *map_base, u8 *map_end) {
    bool is_compressed = ((g_nso_headers[index].flags >> segment) & 1) != 0;
    bool check_hash = ((g_nso_headers[index].flags >> (segment + 3)) & 1) != 0;
    size_t out_size = g_nso_headers[index].segments[segment].decomp_size;
    size_t size = is_compressed ? g_nso_headers[index].compressed_sizes[segment] : out_size;
    if (size > out_size) {
        return ResultLoaderInvalidNso;
    }
    if ((u32)(size | out_size) >> 31) {
        return ResultLoaderInvalidNso;
    }
    
    u8 *dst_addr = map_base + g_nso_headers[index].segments[segment].dst_offset;
    u8 *load_addr = is_compressed ? map_end - size : dst_addr;
    
    
    fseek(f_nso, g_nso_headers[index].segments[segment].file_offset, SEEK_SET);
    if (fread(load_addr, 1, size, f_nso) != size) {
        return ResultLoaderInvalidNso;
    }
    
    
    if (is_compressed) {
        if (LZ4_decompress_safe((char *)load_addr, (char *)dst_addr, size, out_size) != (int)out_size) {
            return ResultLoaderInvalidNso;
        }
    }

    
    if (check_hash) {
        u8 hash[0x20] = {0};
        sha256CalculateHash(hash, dst_addr, out_size);

        if (std::memcmp(g_nso_headers[index].section_hashes[segment], hash, sizeof(hash))) {
            return ResultLoaderInvalidNso;
        }
    }
    
    return ResultSuccess;
}

Result NsoUtils::LoadNsosIntoProcessMemory(Handle process_h, u64 title_id, NsoLoadExtents *extents, u8 *args, u32 args_size) {
    Result rc = ResultLoaderInvalidNso;
    for (unsigned int i = 0; i < NSO_NUM_MAX; i++) {
        if (g_nso_present[i]) {
            AutoCloseMap nso_map;
            if (R_FAILED((rc = nso_map.Open(process_h, extents->nso_addresses[i], extents->nso_sizes[i])))) {
                return rc;
            }
            
            u8 *map_base = (u8 *)nso_map.GetMappedAddress();
                        
            FILE *f_nso = OpenNso(i, title_id);
            if (f_nso == NULL) {
                /* TODO: Is there a better error to return here? */
                return ResultLoaderInvalidNso;
            }
            for (unsigned int seg = 0; seg < 3; seg++) {
                if (R_FAILED((rc = LoadNsoSegment(title_id, i, seg, f_nso, map_base, map_base + extents->nso_sizes[i])))) {
                    fclose(f_nso);
                    return rc;
                }
            }
            
            fclose(f_nso);
            f_nso = NULL;
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
            u64 bss_base = rw_start + g_nso_headers[i].segments[2].decomp_size, bss_size = g_nso_headers[i].segments[2].align_or_total_size;
            std::fill(map_base + bss_base, map_base + bss_base + bss_size, 0);
            
            /* Apply patches to loaded module. */
            PatchUtils::ApplyPatches(&g_nso_headers[i], map_base, bss_base);
            
            nso_map.Close();
            
            for (unsigned int seg = 0; seg < 3; seg++) {
                u64 size = g_nso_headers[i].segments[seg].decomp_size;
                if (seg == 2) {
                    size += g_nso_headers[i].segments[2].align_or_total_size;
                }
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
        AutoCloseMap args_map;
        if (R_FAILED((rc = args_map.Open(process_h, extents->args_address, extents->args_size)))) {
            return rc;
        }
        
        NsoArgument *arg_map_base = (NsoArgument *)args_map.GetMappedAddress();
        
        arg_map_base->allocated_space = extents->args_size;
        arg_map_base->args_size = args_size;
        std::fill(arg_map_base->_0x8, arg_map_base->_0x8 + sizeof(arg_map_base->_0x8), 0);
        std::copy(args, args + args_size, arg_map_base->arguments);
        
        args_map.Close();  
        
        if (R_FAILED((rc = svcSetProcessMemoryPermission(process_h, extents->args_address, extents->args_size, 3)))) {
            return rc;
        }
    }
    
    return rc;
}
