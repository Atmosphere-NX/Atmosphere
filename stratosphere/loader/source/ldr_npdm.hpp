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
 
#pragma once
#include <switch.h>
#include <cstdio>

#include "ldr_registration.hpp"
#include "ldr_content_management.hpp" /* for ExternalContentSource */

#define MAGIC_META 0x4154454D
#define MAGIC_ACI0 0x30494341
#define MAGIC_ACID 0x44494341

class NpdmUtils { 
    public:
        struct NpdmHeader {
            u32 magic;
            u32 _0x4;
            u32 _0x8;
            u8 mmu_flags;
            u8 _0xD;
            u8 main_thread_prio;
            u8 default_cpuid;
            u32 _0x10;
            u32 system_resource_size;
            u32 process_category;
            u32 main_stack_size;
            char title_name[0x50];
            u32 aci0_offset;
            u32 aci0_size;
            u32 acid_offset;
            u32 acid_size;
        };
        struct NpdmAcid {
            u8 signature[0x100];
            u8 modulus[0x100];
            u32 magic;
            u32 size;
            u32 _0x208;
            u32 flags;
            u64 title_id_range_min;
            u64 title_id_range_max;
            u32 fac_offset;
            u32 fac_size;
            u32 sac_offset;
            u32 sac_size;
            u32 kac_offset;
            u32 kac_size;
            u64 padding;
        };
        struct NpdmAci0 {
            u32 magic;
            u8 _0x4[0xC];
            u64 title_id;
            u64 _0x18;
            u32 fah_offset;
            u32 fah_size;
            u32 sac_offset;
            u32 sac_size;
            u32 kac_offset;
            u32 kac_size;
            u64 padding;
        };
        struct NpdmInfo {
            NpdmHeader *header;
            NpdmAcid *acid;
            NpdmAci0 *aci0;
            void *acid_fac;
            void *acid_sac;
            void *acid_kac;
            void *aci0_fah;
            void *aci0_sac;
            void *aci0_kac;
            u64 title_id;
        };
        struct NpdmCache {
            NpdmInfo info;
            u8 buffer[0x8000];
        };
        
        static_assert(sizeof(NpdmHeader) == 0x80, "Incorrectly defined NpdmHeader!");
        static_assert(sizeof(NpdmAcid) == 0x240, "Incorrectly defined NpdmAcid!");
        static_assert(sizeof(NpdmAci0) == 0x40, "Incorrectly defined NpdmAci0!");
        
        static u32 GetApplicationType(u32 *caps, size_t num_caps);
        static u32 GetApplicationTypeRaw(u32 *caps, size_t num_caps);
        
        static Result ValidateCapabilityAgainstRestrictions(u32 *restrict_caps, size_t num_restrict_caps, u32 *&cur_cap, size_t &caps_remaining);
        static Result ValidateCapabilities(u32 *acid_caps, size_t num_acid_caps, u32 *aci0_caps, size_t num_aci0_caps);
        
        static FILE *OpenNpdmFromECS(ContentManagement::ExternalContentSource *ecs);
        static FILE *OpenNpdmFromHBL();
        static FILE *OpenNpdmFromExeFS();
        static FILE *OpenNpdmFromSdCard(u64 tid);
        static FILE *OpenNpdm(u64 tid);
        static Result LoadNpdm(u64 tid, NpdmInfo *out);
        static Result LoadNpdmFromCache(u64 tid, NpdmInfo *out);

        static void InvalidateCache(u64 tid);
    private:
        static Result LoadNpdmInternal(FILE *f_npdm, NpdmCache *cache);
};
