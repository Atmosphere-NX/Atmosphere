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
#include "ldr_npdm.hpp"
#include "ldr_registration.hpp"
#include "ldr_content_management.hpp"

static NpdmUtils::NpdmCache g_npdm_cache = {0};
static NpdmUtils::NpdmCache g_original_npdm_cache = {0};
static char g_npdm_path[FS_MAX_PATH] = {0};

Result NpdmUtils::LoadNpdmFromCache(u64 tid, NpdmInfo *out) {
    if (g_npdm_cache.info.title_id != tid) {
        return LoadNpdm(tid, out);
    }
    *out = g_npdm_cache.info;
    return ResultSuccess;
}

FILE *NpdmUtils::OpenNpdmFromECS(ContentManagement::ExternalContentSource *ecs) {
    std::fill(g_npdm_path, g_npdm_path + FS_MAX_PATH, 0);
    snprintf(g_npdm_path, FS_MAX_PATH, "%s:/main.npdm", ecs->mountpoint);
    return fopen(g_npdm_path, "rb");
}

FILE *NpdmUtils::OpenNpdmFromHBL() {
    std::fill(g_npdm_path, g_npdm_path + FS_MAX_PATH, 0);
    snprintf(g_npdm_path, FS_MAX_PATH, "hbl:/main.npdm");
    return fopen(g_npdm_path, "rb");
}

FILE *NpdmUtils::OpenNpdmFromExeFS() {
    std::fill(g_npdm_path, g_npdm_path + FS_MAX_PATH, 0);
    snprintf(g_npdm_path, FS_MAX_PATH, "code:/main.npdm");
    return fopen(g_npdm_path, "rb");
}

FILE *NpdmUtils::OpenNpdmFromSdCard(u64 title_id) {  
    std::fill(g_npdm_path, g_npdm_path + FS_MAX_PATH, 0);
    snprintf(g_npdm_path, FS_MAX_PATH, "sdmc:/atmosphere/titles/%016lx/exefs/main.npdm", title_id);
    return fopen(g_npdm_path, "rb");
}


FILE *NpdmUtils::OpenNpdm(u64 title_id) {
    ContentManagement::ExternalContentSource *ecs = nullptr;
    if ((ecs = ContentManagement::GetExternalContentSource(title_id)) != nullptr) {
        return OpenNpdmFromECS(ecs);
    }
    
    /* First, check HBL. */
    if (ContentManagement::ShouldOverrideContentsWithHBL(title_id)) {
        return OpenNpdmFromHBL();
    }
    
    /* Next, check other override. */
    if (ContentManagement::ShouldOverrideContentsWithSD(title_id)) {
        FILE *f_out = OpenNpdmFromSdCard(title_id);
        if (f_out != NULL) {
            return f_out;
        }
    }
    
    /* Last resort: real exefs. */
    return OpenNpdmFromExeFS();
}

Result NpdmUtils::LoadNpdmInternal(FILE *f_npdm, NpdmUtils::NpdmCache *cache) {
    Result rc;
    
    cache->info = {};

    rc = ResultFsPathNotFound;
    if (f_npdm == NULL) {
        /* For generic "Couldn't open the file" error, just say the file doesn't exist. */
        return rc;
    }
    
    fseek(f_npdm, 0, SEEK_END);
    size_t npdm_size = ftell(f_npdm);
    fseek(f_npdm, 0, SEEK_SET);
    
    rc = ResultLoaderTooLargeMeta;
    if ((npdm_size > sizeof(cache->buffer)) || (fread(cache->buffer, 1, npdm_size, f_npdm) != npdm_size)) {
        fclose(f_npdm);
        return rc;
    }
    
    fclose(f_npdm);
    
    rc = ResultLoaderInvalidMeta;
    if (npdm_size < sizeof(NpdmUtils::NpdmHeader)) {
        return rc;
    }
    
    /* For ease of access... */
    cache->info.header = (NpdmUtils::NpdmHeader *)(cache->buffer);
    NpdmInfo *info = &cache->info;
    
    if (info->header->magic != MAGIC_META) {
        return rc;
    }
    
    /* 7.0.0 added 0x10 as a valid bit to NPDM flags. */
    if (GetRuntimeFirmwareVersion() >= FirmwareVersion_700) {
        if (info->header->mmu_flags > 0x1F) {
            return rc;
        }
    } else {
        if (info->header->mmu_flags > 0xF) {
            return rc;
        }
    }
    
    if (info->header->aci0_offset < sizeof(NpdmUtils::NpdmHeader) || info->header->aci0_size < sizeof(NpdmUtils::NpdmAci0) || info->header->aci0_offset + info->header->aci0_size > npdm_size) {
        return rc;
    }
    
    info->aci0 = (NpdmAci0 *)(cache->buffer + info->header->aci0_offset);
    
    if (info->aci0->magic != MAGIC_ACI0) {
        return rc;
    }
    
    if (info->aci0->fah_size > info->header->aci0_size || info->aci0->fah_offset < sizeof(NpdmUtils::NpdmAci0) || info->aci0->fah_offset + info->aci0->fah_size > info->header->aci0_size) {
        return rc;
    }
    
    info->aci0_fah = (void *)((uintptr_t)info->aci0 + info->aci0->fah_offset);
    
    if (info->aci0->sac_size > info->header->aci0_size || info->aci0->sac_offset < sizeof(NpdmUtils::NpdmAci0) || info->aci0->sac_offset + info->aci0->sac_size > info->header->aci0_size) {
        return rc;
    }
    
    info->aci0_sac = (void *)((uintptr_t)info->aci0 + info->aci0->sac_offset);
    
    if (info->aci0->kac_size > info->header->aci0_size || info->aci0->kac_offset < sizeof(NpdmUtils::NpdmAci0) || info->aci0->kac_offset + info->aci0->kac_size > info->header->aci0_size) {
        return rc;
    }
    
    info->aci0_kac = (void *)((uintptr_t)info->aci0 + info->aci0->kac_offset);
    
    if (info->header->acid_offset < sizeof(NpdmUtils::NpdmHeader) || info->header->acid_size < sizeof(NpdmUtils::NpdmAcid) || info->header->acid_offset + info->header->acid_size > npdm_size) {
        return rc;
    }
    
    info->acid = (NpdmAcid *)(cache->buffer + info->header->acid_offset);
    
    if (info->acid->magic != MAGIC_ACID) {
        return rc;
    }
    
    /* TODO: Check if retail flag is set if not development hardware. */
    
    if (info->acid->fac_size > info->header->acid_size || info->acid->fac_offset < sizeof(NpdmUtils::NpdmAcid) || info->acid->fac_offset + info->acid->fac_size > info->header->acid_size) {
        return rc;
    }
    
    info->acid_fac = (void *)((uintptr_t)info->acid + info->acid->fac_offset);
    
    if (info->acid->sac_size > info->header->acid_size || info->acid->sac_offset < sizeof(NpdmUtils::NpdmAcid) || info->acid->sac_offset + info->acid->sac_size > info->header->acid_size) {
        return rc;
    }
    
    info->acid_sac = (void *)((uintptr_t)info->acid + info->acid->sac_offset);
    
    if (info->acid->kac_size > info->header->acid_size || info->acid->kac_offset < sizeof(NpdmUtils::NpdmAcid) || info->acid->kac_offset + info->acid->kac_size > info->header->acid_size) {
        return rc;
    }
    
    info->acid_kac = (void *)((uintptr_t)info->acid + info->acid->kac_offset);
    
    rc = ResultSuccess;
    return rc;
}

Result NpdmUtils::LoadNpdm(u64 tid, NpdmInfo *out) {
    Result rc;
        
    rc = LoadNpdmInternal(OpenNpdm(tid), &g_npdm_cache);
    
    if (R_FAILED(rc)) {
        return rc;
    }
    
    NpdmInfo *info = &g_npdm_cache.info;
    /* Override the ACID/ACI0 title ID, in order to facilitate HBL takeover of any title. */
    info->acid->title_id_range_min = tid;
    info->acid->title_id_range_max = tid;
    info->aci0->title_id = tid;
    
    if (ContentManagement::ShouldOverrideContentsWithHBL(tid) && R_SUCCEEDED(LoadNpdmInternal(OpenNpdmFromExeFS(), &g_original_npdm_cache))) {
        NpdmInfo *original_info = &g_original_npdm_cache.info;
        /* Fix pool partition. */
        if (kernelAbove500()) {
            info->acid->flags = (info->acid->flags & 0xFFFFFFC3) | (original_info->acid->flags & 0x0000003C);
        }
        /* Fix application type. */
        const u32 original_application_type = GetApplicationTypeRaw((u32 *)original_info->aci0_kac, original_info->aci0->kac_size/sizeof(u32)) & 7;
        u32 *caps = (u32 *)info->aci0_kac;
        for (unsigned int i = 0; i < info->aci0->kac_size/sizeof(u32); i++) {
            if ((caps[i] & 0x3FFF) == 0x1FFF) {
                caps[i] = (caps[i] & 0xFFFE3FFF) | (original_application_type << 14);
            }
        }
        caps = (u32 *)info->acid_kac;
        for (unsigned int i = 0; i < info->acid->kac_size/sizeof(u32); i++) {
            if ((caps[i] & 0x3FFF) == 0x1FFF) {
                caps[i] = (caps[i] & 0xFFFE3FFF) | (original_application_type << 14);
            }
        }
    }
        
    /* We validated! */
    info->title_id = tid;
    *out = *info;
    rc = ResultSuccess;
    
    return rc;
}

Result NpdmUtils::ValidateCapabilityAgainstRestrictions(u32 *restrict_caps, size_t num_restrict_caps, u32 *&cur_cap, size_t &caps_remaining) {
    Result rc = ResultSuccess;
    u32 desc = *cur_cap++;
    caps_remaining--;
    unsigned int low_bits = 0;
    while (desc & 1) {
        desc >>= 1;
        low_bits++;
    }
    desc >>= 1;
    u32 r_desc = 0;
    switch (low_bits) {
        case 3: /* Kernel flags. */
            rc = ResultLoaderInvalidCapabilityKernelFlags;
            for (size_t i = 0; i < num_restrict_caps; i++) {
                if ((restrict_caps[i] & 0xF) == 0x7) {
                    r_desc = restrict_caps[i] >> 4;
                    u32 highest_thread_prio = desc & 0x3F;
                    u32 r_highest_thread_prio = r_desc & 0x3F;
                    desc >>= 6;
                    r_desc >>= 6;
                    u32 lowest_thread_prio = desc & 0x3F;
                    u32 r_lowest_thread_prio = r_desc & 0x3F;
                    desc >>= 6;
                    r_desc >>= 6;
                    u32 lowest_cpu_id = desc & 0xFF;
                    u32 r_lowest_cpu_id = r_desc & 0xFF;
                    desc >>= 8;
                    r_desc >>= 8;
                    u32 highest_cpu_id = desc & 0xFF;
                    u32 r_highest_cpu_id = r_desc & 0xFF;
                    if (highest_thread_prio > r_highest_thread_prio) {
                       break;
                    }
                    if (lowest_thread_prio > highest_thread_prio) {
                       break;
                    }
                    if (lowest_thread_prio < r_lowest_thread_prio) {
                       break;
                    }
                    if (lowest_cpu_id < r_lowest_cpu_id) {
                       break;
                    }
                    if (lowest_cpu_id > r_highest_cpu_id) {
                       break;
                    }
                    if (highest_cpu_id > r_highest_cpu_id) {
                       break;
                    }       
                    /* Valid! */
                    rc = ResultSuccess;
                    break;
                }
            }
            break;
        case 4: /* Syscall mask. */
            rc = ResultLoaderInvalidCapabilitySyscallMask;
            for (size_t i = 0; i < num_restrict_caps; i++) {
                if ((restrict_caps[i] & 0x1F) == 0xF) {
                    r_desc = restrict_caps[i] >> 5;              
                    u32 syscall_base = (desc >> 24);
                    u32 r_syscall_base = (r_desc >> 24);
                    if (syscall_base != r_syscall_base) {
                        continue;
                    }
                    u32 syscall_mask = desc & 0xFFFFFF;
                    u32 r_syscall_mask = r_desc & 0xFFFFFF;
                    if ((r_syscall_mask & syscall_mask) != syscall_mask) {
                        break;
                    }
                    /* Valid! */
                    rc = ResultSuccess;
                    break;
                }
            }
            break;
        case 6: /* Map IO/Normal. */ {
                rc = ResultLoaderInvalidCapabilityMapRange;
                if (caps_remaining == 0) {
                    break;
                }
                u32 next_cap = *cur_cap++;
                caps_remaining--;
                if ((next_cap & 0x7F) != 0x3F) {
                    break;
                }
                u32 next_desc = next_cap >> 7;
                u32 base_addr = desc & 0xFFFFFF;
                u32 base_size = next_desc & 0xFFFFFF;
                /* Size check the mapping. */
                if (base_size >> 20) {
                    break;
                }
                u32 base_end = base_addr + base_size;
                /* Validate it's possible to validate this mapping. */
                if (num_restrict_caps < 2) {
                    break;
                }
                for (size_t i = 0; i < num_restrict_caps - 1; i++) {
                    if ((restrict_caps[i] & 0x7F) == 0x3F) {
                        r_desc = restrict_caps[i] >> 7;
                        if ((restrict_caps[i+1] & 0x7F) != 0x3F) {
                            break;
                        }
                        u32 r_next_desc = restrict_caps[++i] >> 7;
                        u32 r_base_addr = r_desc & 0xFFFFFF;
                        u32 r_base_size = r_next_desc & 0xFFFFFF;
                        /* Size check the mapping. */
                        if (r_base_size >> 20) {
                            break;
                        }
                        u32 r_base_end = r_base_addr + r_base_size;
                        /* Validate is_io matches. */
                        if (((r_desc >> 24) & 1) ^ ((desc >> 24) & 1)) {
                            continue;
                        }
                        /* Validate is_ro matches. */
                        if (((r_next_desc >> 24) & 1) ^ ((next_desc >> 24) & 1)) {
                            continue;
                        }
                        /* Validate bounds. */
                        if (base_addr < r_base_addr || base_end > r_base_end) {
                            continue;
                        }
                        /* Valid! */
                        rc = ResultSuccess;
                        break;
                    }
                }
            }
            break;
        case 7: /* Map Normal Page. */
            rc = ResultLoaderInvalidCapabilityMapPage;
            for (size_t i = 0; i < num_restrict_caps; i++) {
                if ((restrict_caps[i] & 0xFF) == 0x7F) {
                    r_desc = restrict_caps[i] >> 8;              
                    if (r_desc != desc) {
                        continue;
                    }
                    /* Valid! */
                    rc = ResultSuccess;
                    break;
                }
            }
            break;
        case 11: /* IRQ Pair. */ 
            rc = ResultSuccess;
            for (unsigned int irq_i = 0; irq_i < 2; irq_i++) {
                u32 irq = desc & 0x3FF;
                desc >>= 10;
                if (irq != 0x3FF) {
                    bool found = false;
                    for (size_t i = 0; i < num_restrict_caps && !found; i++) {
                        if ((restrict_caps[i] & 0xFFF) == 0x7FF) {
                            r_desc = restrict_caps[i] >> 12;
                            u32 r_irq_0 = r_desc & 0x3FF;
                            r_desc >>= 10;
                            u32 r_irq_1 = r_desc & 0x3FF;
                            found |= irq == r_irq_0 || irq == r_irq_1;
                            found |= r_irq_0 == 0x3FF && r_irq_1 == 0x3FF;
                        }
                    }
                    if (!found) {
                        rc = ResultLoaderInvalidCapabilityInterruptPair;
                        break;
                    }
                }
            }
            break;
        case 13: /* App Type. */
            rc = ResultLoaderInvalidCapabilityApplicationType;
            if (num_restrict_caps) {
                for (size_t i = 0; i < num_restrict_caps; i++) {
                    if ((restrict_caps[i] & 0x3FFF) == 0x1FFF) {
                        r_desc = restrict_caps[i] >> 14;              
                        break;
                    }
                }
            } else {
                r_desc = 0;
            }
            if (desc == r_desc) {
                /* Valid! */
                rc = ResultSuccess;
            }
            break;
        case 14: /* Kernel Release Version. */
            rc = ResultLoaderInvalidCapabilityKernelVersion;
            if (num_restrict_caps) {
                for (size_t i = 0; i < num_restrict_caps; i++) {
                    if ((restrict_caps[i] & 0x7FFF) == 0x3FFF) {
                        r_desc = restrict_caps[i] >> 15;              
                        break;
                    }
                }
            } else {
                r_desc = 0;
            }
            if (desc == r_desc) {
                /* Valid! */
                rc = ResultSuccess;
            }
            break;
        case 15: /* Handle Table Size. */
            rc = ResultLoaderInvalidCapabilityHandleTable;
            for (size_t i = 0; i < num_restrict_caps; i++) {
                if ((restrict_caps[i] & 0xFFFF) == 0x7FFF) {
                    r_desc = restrict_caps[i] >> 16;    
                    desc &= 0x3FF;
                    r_desc &= 0x3FF;
                    if (desc > r_desc) {
                        break;
                    }
                    /* Valid! */
                    rc = ResultSuccess;
                    break;
                }
            }
            break;
        case 16: /* Debug Flags. */
            rc = ResultLoaderInvalidCapabilityDebugFlags;
            if (num_restrict_caps) {
                for (size_t i = 0; i < num_restrict_caps; i++) {
                    if ((restrict_caps[i] & 0x1FFFF) == 0xFFFF) {
                        r_desc = restrict_caps[i] >> 17;              
                        break;
                    }
                }
            } else {
                r_desc = 0;
            }
            if ((desc & ~r_desc) == 0) {
                /* Valid! */
                rc = ResultSuccess;
            }
            break;
        case 32: /* Empty Descriptor. */
            rc = ResultSuccess;
            break;
        default: /* Unrecognized Descriptor. */
            rc = ResultLoaderUnknownCapability;
            break;
    }
    return rc;
}

Result NpdmUtils::ValidateCapabilities(u32 *acid_caps, size_t num_acid_caps, u32 *aci0_caps, size_t num_aci0_caps) {
    Result rc = ResultSuccess;
    size_t remaining = num_aci0_caps;
    u32 *cur_cap = aci0_caps;
    while (remaining) {
        if (R_FAILED((rc = ValidateCapabilityAgainstRestrictions(acid_caps, num_acid_caps, cur_cap, remaining)))) {
            break;
        }
    }

    return rc;
}

u32 NpdmUtils::GetApplicationType(u32 *caps, size_t num_caps) {
    u32 application_type = 0;
    for (unsigned int i = 0; i < num_caps; i++) {
        if ((caps[i] & 0x3FFF) == 0x1FFF) {
            u16 app_type = (caps[i] >> 14) & 7;
            if (app_type == 1) {
                application_type |= 1;
            } else if (app_type == 2) {
                application_type |= 2;
            }
        }
        /* After 1.0.0, allow_debug is used as bit 4. */
        if (kernelAbove200() && (caps[i] & 0x1FFFF) == 0xFFFF) {
            application_type |= (caps[i] >> 15) & 4;
        }
    }
    return application_type;
}

/* Like GetApplicationType, except this returns the raw kac descriptor value. */
u32 NpdmUtils::GetApplicationTypeRaw(u32 *caps, size_t num_caps) {
    u32 application_type = 0;
    for (unsigned int i = 0; i < num_caps; i++) {
        if ((caps[i] & 0x3FFF) == 0x1FFF) {
            return (caps[i] >> 14) & 7;
        }
    }
    return application_type;
}

void NpdmUtils::InvalidateCache(u64 tid) {
    if (g_npdm_cache.info.title_id == tid) {
        g_npdm_cache.info = {};
    }
}
