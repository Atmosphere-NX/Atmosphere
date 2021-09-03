/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
#include <exosphere.hpp>
#include "fusee_stratosphere.hpp"
#include "fusee_fatal.hpp"
#include "fusee_malloc.hpp"
#include "fusee_secondary_archive.hpp"
#include "fs/fusee_fs_api.hpp"

namespace ams::nxboot {

    namespace {

        struct InitialProcessBinaryHeader {
            static constexpr u32 Magic = util::FourCC<'I','N','I','1'>::Code;

            u32 magic;
            u32 size;
            u32 num_processes;
            u32 reserved;
        };

        struct InitialProcessHeader {
            static constexpr u32 Magic = util::FourCC<'K','I','P','1'>::Code;

            u32 magic;
            u8 name[12];
            u64 program_id;
            u32 version;
            u8 priority;
            u8 ideal_core_id;
            u8 _1E;
            u8 flags;
            u32 rx_address;
            u32 rx_size;
            u32 rx_compressed_size;
            u32 affinity_mask;
            u32 ro_address;
            u32 ro_size;
            u32 ro_compressed_size;
            u32 stack_size;
            u32 rw_address;
            u32 rw_size;
            u32 rw_compressed_size;
            u32 _4C;
            u32 bss_address;
            u32 bss_size;
            u32 pad[(0x80 - 0x58) / sizeof(u32)];
            u32 capabilities[0x80 / sizeof(u32)];
        };
        static_assert(sizeof(InitialProcessHeader) == 0x100);

        struct PatchMeta {
            PatchMeta *next;
            bool is_memset;
            u32 start_segment;
            u32 rel_offset;
            const void *data;
            u32 size;
        };

        struct alignas(0x10) InitialProcessMeta {
            InitialProcessMeta *next = nullptr;
            const InitialProcessHeader *kip;
            u32 kip_size;
            PatchMeta *patches_head;
            PatchMeta *patches_tail;
            u32 patch_segments;
            u64 program_id;
            se::Sha256Hash kip_hash;
        };
        static_assert(sizeof(InitialProcessMeta) == 0x40);
        static_assert(alignof(InitialProcessMeta) == 0x10);

        enum FsVersion {
            FsVersion_1_0_0 = 0,

            FsVersion_2_0_0,
            FsVersion_2_0_0_Exfat,

            FsVersion_2_1_0,
            FsVersion_2_1_0_Exfat,

            FsVersion_3_0_0,
            FsVersion_3_0_0_Exfat,

            FsVersion_3_0_1,
            FsVersion_3_0_1_Exfat,

            FsVersion_4_0_0,
            FsVersion_4_0_0_Exfat,

            FsVersion_4_1_0,
            FsVersion_4_1_0_Exfat,

            FsVersion_5_0_0,
            FsVersion_5_0_0_Exfat,

            FsVersion_5_1_0,
            FsVersion_5_1_0_Exfat,

            FsVersion_6_0_0,
            FsVersion_6_0_0_Exfat,

            FsVersion_7_0_0,
            FsVersion_7_0_0_Exfat,

            FsVersion_8_0_0,
            FsVersion_8_0_0_Exfat,

            FsVersion_8_1_0,
            FsVersion_8_1_0_Exfat,

            FsVersion_9_0_0,
            FsVersion_9_0_0_Exfat,

            FsVersion_9_1_0,
            FsVersion_9_1_0_Exfat,

            FsVersion_10_0_0,
            FsVersion_10_0_0_Exfat,

            FsVersion_10_2_0,
            FsVersion_10_2_0_Exfat,

            FsVersion_11_0_0,
            FsVersion_11_0_0_Exfat,

            FsVersion_12_0_0,
            FsVersion_12_0_0_Exfat,

            FsVersion_12_0_3,
            FsVersion_12_0_3_Exfat,

            FsVersion_Count,
        };

        constexpr const u8 FsHashes[FsVersion_Count][8] = {
            { 0xDE, 0x9F, 0xDD, 0xA4, 0x08, 0x5D, 0xD5, 0xFE }, /* FsVersion_1_0_0 */

            { 0xCD, 0x7B, 0xBE, 0x18, 0xD6, 0x13, 0x0B, 0x28 }, /* FsVersion_2_0_0 */
            { 0xE7, 0x66, 0x92, 0xDF, 0xAA, 0x04, 0x20, 0xE9 }, /* FsVersion_2_0_0_Exfat */

            { 0x0D, 0x70, 0x05, 0x62, 0x7B, 0x07, 0x76, 0x7C }, /* FsVersion_2_1_0 */
            { 0xDB, 0xD8, 0x5F, 0xCA, 0xCC, 0x19, 0x3D, 0xA8 }, /* FsVersion_2_1_0_Exfat */

            { 0xA8, 0x6D, 0xA5, 0xE8, 0x7E, 0xF1, 0x09, 0x7B }, /* FsVersion_3_0_0 */
            { 0x98, 0x1C, 0x57, 0xE7, 0xF0, 0x2F, 0x70, 0xF7 }, /* FsVersion_3_0_0_Exfat */

            { 0x57, 0x39, 0x7C, 0x06, 0x3F, 0x10, 0xB6, 0x31 }, /* FsVersion_3_0_1 */
            { 0x07, 0x30, 0x99, 0xD7, 0xC6, 0xAD, 0x7D, 0x89 }, /* FsVersion_3_0_1_Exfat */

            { 0x06, 0xE9, 0x07, 0x19, 0x59, 0x5A, 0x01, 0x0C }, /* FsVersion_4_0_0 */
            { 0x54, 0x9B, 0x0F, 0x8D, 0x6F, 0x72, 0xC4, 0xE9 }, /* FsVersion_4_0_0_Exfat */

            { 0x80, 0x96, 0xAF, 0x7C, 0x6A, 0x35, 0xAA, 0x82 }, /* FsVersion_4_1_0 */
            { 0x02, 0xD5, 0xAB, 0xAA, 0xFD, 0x20, 0xC8, 0xB0 }, /* FsVersion_4_1_0_Exfat */

            { 0xA6, 0xF2, 0x7A, 0xD9, 0xAC, 0x7C, 0x73, 0xAD }, /* FsVersion_5_0_0 */
            { 0xCE, 0x3E, 0xCB, 0xA2, 0xF2, 0xF0, 0x62, 0xF5 }, /* FsVersion_5_0_0_Exfat */

            { 0x76, 0xF8, 0x74, 0x02, 0xC9, 0x38, 0x7C, 0x0F }, /* FsVersion_5_1_0 */
            { 0x10, 0xB2, 0xD8, 0x16, 0x05, 0x48, 0x85, 0x99 }, /* FsVersion_5_1_0_Exfat */

            { 0x3A, 0x57, 0x4D, 0x43, 0x61, 0x86, 0x19, 0x1D }, /* FsVersion_6_0_0 */
            { 0x33, 0x05, 0x53, 0xF6, 0xB5, 0xFB, 0x55, 0xC4 }, /* FsVersion_6_0_0_Exfat */

            { 0x2A, 0xDB, 0xE9, 0x7E, 0x9B, 0x5F, 0x41, 0x77 }, /* FsVersion_7_0_0 */
            { 0x2C, 0xCE, 0x65, 0x9C, 0xEC, 0x53, 0x6A, 0x8E }, /* FsVersion_7_0_0_Exfat */

            { 0xB2, 0xF5, 0x17, 0x6B, 0x35, 0x48, 0x36, 0x4D }, /* FsVersion_8_0_0 */
            { 0xDB, 0xD9, 0x41, 0xC0, 0xC5, 0x3C, 0x52, 0xCC }, /* FsVersion_8_0_0_Exfat */

            { 0x6B, 0x09, 0xB6, 0x7B, 0x29, 0xC0, 0x20, 0x24 }, /* FsVersion_8_1_0 */
            { 0xB4, 0xCA, 0xE1, 0xF2, 0x49, 0x65, 0xD9, 0x2E }, /* FsVersion_8_1_0_Exfat */

            { 0x46, 0x87, 0x40, 0x76, 0x1E, 0x19, 0x3E, 0xB7 }, /* FsVersion_9_0_0 */
            { 0x7C, 0x95, 0x13, 0x76, 0xE5, 0xC1, 0x2D, 0xF8 }, /* FsVersion_9_0_0_Exfat */

            { 0xB5, 0xE7, 0xA6, 0x4C, 0x6F, 0x5C, 0x4F, 0xE3 }, /* FsVersion_9_1_0 */
            { 0xF1, 0x96, 0xD1, 0x44, 0xD0, 0x44, 0x45, 0xB6 }, /* FsVersion_9_1_0_Exfat */

            { 0x3E, 0xEB, 0xD9, 0xB7, 0xBC, 0xD1, 0xB5, 0xE0 }, /* FsVersion_10_0_0 */
            { 0x81, 0x7E, 0xA2, 0xB0, 0xB7, 0x02, 0xC1, 0xF3 }, /* FsVersion_10_0_0_Exfat */

            { 0xA9, 0x52, 0xB6, 0x57, 0xAD, 0xF9, 0xC2, 0xBA }, /* FsVersion_10_2_0 */
            { 0x16, 0x0D, 0x3E, 0x10, 0x4E, 0xAD, 0x61, 0x76 }, /* FsVersion_10_2_0_Exfat */

            { 0xE3, 0x99, 0x15, 0x6E, 0x84, 0x4E, 0xB0, 0xAA }, /* FsVersion_11_0_0 */
            { 0x0B, 0xA1, 0x5B, 0xB3, 0x04, 0xB5, 0x05, 0x63 }, /* FsVersion_11_0_0_Exfat */

            { 0xDC, 0x2A, 0x08, 0x49, 0x96, 0xBB, 0x3C, 0x01 }, /* FsVersion_12_0_0 */
            { 0xD5, 0xA5, 0xBF, 0x36, 0x64, 0x0C, 0x49, 0xEA }, /* FsVersion_12_0_0_Exfat */

            { 0xC8, 0x67, 0x62, 0xBE, 0x19, 0xA5, 0x1F, 0xA0 }, /* FsVersion_12_0_3 */
            { 0xE1, 0xE8, 0xD3, 0xD6, 0xA2, 0xFE, 0x0B, 0x10 }, /* FsVersion_12_0_3_Exfat */
        };

        const InitialProcessBinaryHeader *FindInitialProcessBinary(const pkg2::Package2Header *header, const u8 *data, ams::TargetFirmware target_firmware) {
            if (target_firmware >= ams::TargetFirmware_8_0_0) {
                /* Try to find initial process binary. */
                const u32 *data_32 = reinterpret_cast<const u32 *>(data);
                for (size_t i = 0; i < 0x1000 / sizeof(u32); ++i) {
                    if (data_32[i] == 0 && data_32[i + 8] <= header->meta.payload_sizes[0] && std::memcmp(data + data_32[i + 8], "INI1", 4) == 0) {
                        return reinterpret_cast<const InitialProcessBinaryHeader *>(data + data_32[i + 8]);
                    }
                }

                return nullptr;
            } else {
                return reinterpret_cast<const InitialProcessBinaryHeader *>(data + header->meta.payload_sizes[0]);
            }
        }

        constexpr size_t GetInitialProcessSize(const InitialProcessHeader *kip) {
            return sizeof(*kip) + kip->rx_compressed_size + kip->ro_compressed_size + kip->rw_compressed_size;
        }

        const InitialProcessHeader *FindInitialProcessInBinary(const InitialProcessBinaryHeader *ini, u64 program_id) {
            const u8 *data = reinterpret_cast<const u8 *>(ini + 1);
            for (u32 i = 0; i < ini->num_processes; ++i) {
                const InitialProcessHeader *kip = reinterpret_cast<const InitialProcessHeader *>(data);
                if (kip->magic != InitialProcessHeader::Magic) {
                    return nullptr;
                }

                if (kip->program_id == program_id) {
                    return kip;
                }

                data += GetInitialProcessSize(kip);
            }

            return nullptr;
        }

        FsVersion GetFsVersion(const se::Sha256Hash &fs_hash) {
            for (size_t i = 0; i < util::size(FsHashes); ++i) {
                if (std::memcmp(fs_hash.bytes, FsHashes[i], sizeof(FsHashes[i])) == 0) {
                    return static_cast<FsVersion>(i);
                }
            }

            return FsVersion_Count;
        }

        constinit InitialProcessMeta g_initial_process_meta = {};
        constinit size_t g_initial_process_binary_size = 0;

        void AddInitialProcessImpl(InitialProcessMeta *meta, const InitialProcessHeader *kip, const se::Sha256Hash *hash) {
            /* Set the meta's fields. */
            meta->next       = nullptr;
            meta->program_id = kip->program_id;
            meta->kip        = kip;
            meta->kip_size   = GetInitialProcessSize(kip);

            /* Copy or calculate hash. */
            if (hash != nullptr) {
                std::memcpy(std::addressof(meta->kip_hash), hash, sizeof(meta->kip_hash));
            } else {
                se::CalculateSha256(std::addressof(meta->kip_hash), kip, meta->kip_size);
            }

            /* Clear patches. */
            meta->patches_head   = nullptr;
            meta->patches_tail   = nullptr;
            meta->patch_segments = 0;

            /* Increase the initial process binary's size. */
            g_initial_process_binary_size += meta->kip_size;
        }

        bool AddInitialProcess(const InitialProcessHeader *kip, const se::Sha256Hash *hash = nullptr) {
            /* Handle the initial case. */
            if (g_initial_process_binary_size == 0) {
                AddInitialProcessImpl(std::addressof(g_initial_process_meta), kip, hash);
                return true;
            }

            /* Check if we've already added the program id. */
            InitialProcessMeta *cur = std::addressof(g_initial_process_meta);
            while (true) {
                if (cur->program_id == kip->program_id) {
                    return false;
                }

                if (cur->next != nullptr) {
                    cur = cur->next;
                } else {
                    break;
                }
            }

            /* Allocate an initial process meta. */
            auto *new_meta = static_cast<InitialProcessMeta *>(AllocateAligned(sizeof(InitialProcessMeta), alignof(InitialProcessMeta)));

            /* Insert the new meta. */
            cur->next = new_meta;
            AddInitialProcessImpl(new_meta, kip, hash);
            return true;
        }

        InitialProcessMeta *FindInitialProcess(u64 program_id) {
            for (InitialProcessMeta *cur = std::addressof(g_initial_process_meta); cur != nullptr; cur = cur->next) {
                if (cur->program_id == program_id) {
                    return cur;
                }
            }
            return nullptr;
        }

        InitialProcessMeta *FindInitialProcess(const se::Sha256Hash &hash) {
            for (InitialProcessMeta *cur = std::addressof(g_initial_process_meta); cur != nullptr; cur = cur->next) {
                if (std::memcmp(std::addressof(cur->kip_hash), std::addressof(hash), sizeof(hash)) == 0) {
                    return cur;
                }
            }
            return nullptr;
        }

        u32 GetPatchSegments(const InitialProcessHeader *kip, u32 offset, size_t size) {
            /* Create segment mask. */
            u32 segments = 0;

            /* Get the segment extents. */
            const u32 rx_start = kip->rx_address;
            const u32 ro_start = kip->ro_address;
            const u32 rw_start = kip->rw_address;
            const u32 rx_end   = ro_start;
            const u32 ro_end   = rw_start;
            const u32 rw_end   = rw_start + kip->rw_size;

            /* If the offset is below the kip header, ignore it. */
            if (offset < sizeof(*kip)) {
                return segments;
            }

            /* Adjust the offset in bounds. */
            offset -= sizeof(*kip);

            /* Check if the offset strays out of bounds. */
            if (offset + size > rw_end) {
                return segments;
            }

            /* Set bits for the affected segments. */
            if (util::HasOverlap(offset, size, rx_start, rx_end - rx_start)) {
                segments |= (1 << 0);
            }
            if (util::HasOverlap(offset, size, ro_start, ro_end - ro_start)) {
                segments |= (1 << 1);
            }
            if (util::HasOverlap(offset, size, rw_start, rw_end - rw_start)) {
                segments |= (1 << 2);
            }

            return segments;
        }

        void AddPatch(InitialProcessMeta *meta, u32 offset, const void *data, size_t data_size, bool is_memset = false) {
            /* Determine the segment. */
            const u32 segments = GetPatchSegments(meta->kip, offset, data_size);

            /* If the patch hits no segments, we don't need it. */
            if (segments == 0) {
                return;
            }

            /* Update patch segments. */
            meta->patch_segments |= segments;

            /* Adjust offset. */
            const u32 start_segment = util::CountTrailingZeros(segments);
            offset -= sizeof(*meta->kip);
            switch (start_segment) {
                case 0: offset -= meta->kip->rx_address; break;
                case 1: offset -= meta->kip->ro_address; break;
                case 2: offset -= meta->kip->rw_address; break;
            }

            /* Create patch. */
            auto *new_patch = static_cast<PatchMeta *>(AllocateAligned(sizeof(PatchMeta), alignof(PatchMeta)));

            new_patch->next          = nullptr;
            new_patch->is_memset     = is_memset;
            new_patch->start_segment = start_segment;
            new_patch->rel_offset    = offset;
            new_patch->data          = data;
            new_patch->size          = data_size;

            /* Add the patch. */
            if (meta->patches_head == nullptr) {
                meta->patches_head = new_patch;
            } else {
                meta->patches_tail->next = new_patch;
            }

            meta->patches_tail = new_patch;
        }

        void AddIps24PatchToKip(InitialProcessMeta *meta, const u8 *ips, s32 size) {
            while (size > 0) {
                /* Read offset, stopping at EOF */
                const u32 offset = (static_cast<u32>(ips[0]) << 16) | (static_cast<u32>(ips[1]) <<  8) | (static_cast<u32>(ips[2]) <<  0);
                if (offset == 0x454F46) {
                    break;
                }

                /* Read size. */
                const u16 cur_size = (static_cast<u32>(ips[3]) << 8) | (static_cast<u32>(ips[4]) << 0);

                if (cur_size > 0) {
                    /* Add patch. */
                    AddPatch(meta, offset, ips + 5, cur_size, false);

                    /*  Advance. */
                    ips  += (5 + cur_size);
                    size -= (5 + cur_size);
                } else {
                    /* Read RLE size */
                    const u16 rle_size = (static_cast<u32>(ips[5]) << 8) | (static_cast<u32>(ips[6]) << 0);

                    /* Add patch. */
                    AddPatch(meta, offset, ips + 7, rle_size, true);

                    /*  Advance. */
                    ips  += 8;
                    size -= 8;
                }
            }
        }

        void AddIps32PatchToKip(InitialProcessMeta *meta, const u8 *ips, s32 size) {
            while (size > 0) {
                /* Read offset, stopping at EOF */
                const u32 offset = (static_cast<u32>(ips[0]) << 24) | (static_cast<u32>(ips[1]) << 16) | (static_cast<u32>(ips[2]) <<  8) | (static_cast<u32>(ips[3]) <<  0);
                if (offset == 0x45454F46) {
                    break;
                }

                /* Read size. */
                const u16 cur_size = (static_cast<u32>(ips[4]) << 8) | (static_cast<u32>(ips[5]) << 0);

                if (cur_size > 0) {
                    /* Add patch. */
                    AddPatch(meta, offset, ips + 6, cur_size, false);

                    /*  Advance. */
                    ips  += (6 + cur_size);
                    size -= (6 + cur_size);
                } else {
                    /* Read RLE size */
                    const u16 rle_size = (static_cast<u32>(ips[6]) << 8) | (static_cast<u32>(ips[7]) << 0);

                    /* Add patch. */
                    AddPatch(meta, offset, ips + 8, rle_size, true);

                    /*  Advance. */
                    ips  += 9;
                    size -= 9;
                }
            }
        }

        void AddIpsPatchToKip(InitialProcessMeta *meta, const u8 *ips, s32 size) {
            if (std::memcmp(ips, "PATCH", 5) == 0) {
                AddIps24PatchToKip(meta, ips + 5, size - 5);
            } else if (std::memcmp(ips, "IPS32", 5) == 0) {
                AddIps32PatchToKip(meta, ips + 5, size - 5);
            }
        }

        constexpr const u8 NogcPatch0[] = {
            0x80
        };

        constexpr const u8 NogcPatch1[] = {
            0xE0, 0x03, 0x1F, 0x2A, 0xC0, 0x03, 0x5F, 0xD6,
        };

        void AddNogcPatches(InitialProcessMeta *fs_meta, FsVersion fs_version) {
            switch (fs_version) {
                case FsVersion_1_0_0:
                case FsVersion_2_0_0:
                case FsVersion_2_0_0_Exfat:
                case FsVersion_2_1_0:
                case FsVersion_2_1_0_Exfat:
                case FsVersion_3_0_0:
                case FsVersion_3_0_0_Exfat:
                case FsVersion_3_0_1:
                case FsVersion_3_0_1_Exfat:
                    /* There were no lotus firmware updates prior to 4.0.0. */
                    /* TODO: Implement patches, regardless? */
                    break;
                case FsVersion_4_0_0:
                case FsVersion_4_0_0_Exfat:
                    AddPatch(fs_meta, 0x0A3539, NogcPatch0, sizeof(NogcPatch0));
                    AddPatch(fs_meta, 0x0AAC44, NogcPatch1, sizeof(NogcPatch1));
                    break;
                case FsVersion_4_1_0:
                case FsVersion_4_1_0_Exfat:
                    AddPatch(fs_meta, 0x0A35BD, NogcPatch0, sizeof(NogcPatch0));
                    AddPatch(fs_meta, 0x0AACA8, NogcPatch1, sizeof(NogcPatch1));
                    break;
                case FsVersion_5_0_0:
                case FsVersion_5_0_0_Exfat:
                    AddPatch(fs_meta, 0x0CF4C5, NogcPatch0, sizeof(NogcPatch0));
                    AddPatch(fs_meta, 0x0D74A0, NogcPatch1, sizeof(NogcPatch1));
                    break;
                case FsVersion_5_1_0:
                case FsVersion_5_1_0_Exfat:
                    AddPatch(fs_meta, 0x0CF895, NogcPatch0, sizeof(NogcPatch0));
                    AddPatch(fs_meta, 0x0D7870, NogcPatch1, sizeof(NogcPatch1));
                    break;
                case FsVersion_6_0_0:
                    AddPatch(fs_meta, 0x1539F5, NogcPatch0, sizeof(NogcPatch0));
                    AddPatch(fs_meta, 0x12CD20, NogcPatch1, sizeof(NogcPatch1));
                    break;
                case FsVersion_6_0_0_Exfat:
                    AddPatch(fs_meta, 0x15F0F5, NogcPatch0, sizeof(NogcPatch0));
                    AddPatch(fs_meta, 0x138420, NogcPatch1, sizeof(NogcPatch1));
                    break;
                case FsVersion_7_0_0:
                    AddPatch(fs_meta, 0x15C005, NogcPatch0, sizeof(NogcPatch0));
                    AddPatch(fs_meta, 0x134260, NogcPatch1, sizeof(NogcPatch1));
                    break;
                case FsVersion_7_0_0_Exfat:
                    AddPatch(fs_meta, 0x1675B5, NogcPatch0, sizeof(NogcPatch0));
                    AddPatch(fs_meta, 0x13F810, NogcPatch1, sizeof(NogcPatch1));
                    break;
                case FsVersion_8_0_0:
                case FsVersion_8_1_0:
                    AddPatch(fs_meta, 0x15EC95, NogcPatch0, sizeof(NogcPatch0));
                    AddPatch(fs_meta, 0x136900, NogcPatch1, sizeof(NogcPatch1));
                    break;
                case FsVersion_8_0_0_Exfat:
                case FsVersion_8_1_0_Exfat:
                    AddPatch(fs_meta, 0x16A245, NogcPatch0, sizeof(NogcPatch0));
                    AddPatch(fs_meta, 0x141EB0, NogcPatch1, sizeof(NogcPatch1));
                    break;
                case FsVersion_9_0_0:
                case FsVersion_9_0_0_Exfat:
                    AddPatch(fs_meta, 0x143369, NogcPatch0, sizeof(NogcPatch0));
                    AddPatch(fs_meta, 0x129520, NogcPatch1, sizeof(NogcPatch1));
                    break;
                case FsVersion_9_1_0:
                case FsVersion_9_1_0_Exfat:
                    AddPatch(fs_meta, 0x143379, NogcPatch0, sizeof(NogcPatch0));
                    AddPatch(fs_meta, 0x129530, NogcPatch1, sizeof(NogcPatch1));
                    break;
                case FsVersion_10_0_0:
                case FsVersion_10_0_0_Exfat:
                    AddPatch(fs_meta, 0x14DF09, NogcPatch0, sizeof(NogcPatch0));
                    AddPatch(fs_meta, 0x13BF90, NogcPatch1, sizeof(NogcPatch1));
                    break;
                case FsVersion_10_2_0:
                case FsVersion_10_2_0_Exfat:
                    AddPatch(fs_meta, 0x14E369, NogcPatch0, sizeof(NogcPatch0));
                    AddPatch(fs_meta, 0x13C3F0, NogcPatch1, sizeof(NogcPatch1));
                    break;
                case FsVersion_11_0_0:
                case FsVersion_11_0_0_Exfat:
                    AddPatch(fs_meta, 0x156FB9, NogcPatch0, sizeof(NogcPatch0));
                    AddPatch(fs_meta, 0x1399B4, NogcPatch1, sizeof(NogcPatch1));
                    break;
                case FsVersion_12_0_0:
                case FsVersion_12_0_0_Exfat:
                    AddPatch(fs_meta, 0x155469, NogcPatch0, sizeof(NogcPatch0));
                    AddPatch(fs_meta, 0x13EB24, NogcPatch1, sizeof(NogcPatch1));
                    break;
                case FsVersion_12_0_3:
                case FsVersion_12_0_3_Exfat:
                    AddPatch(fs_meta, 0x155579, NogcPatch0, sizeof(NogcPatch0));
                    AddPatch(fs_meta, 0x13EC34, NogcPatch1, sizeof(NogcPatch1));
                    break;
                default:
                    break;
            }
        }

        void *ReadFile(s64 *out_size, const char *path, size_t align = 0x10) {
            fs::FileHandle file;
            if (R_SUCCEEDED(fs::OpenFile(std::addressof(file), path, fs::OpenMode_Read))) {
                ON_SCOPE_EXIT { fs::CloseFile(file); };

                Result result;

                /* Get the kip size. */
                if (R_FAILED((result = fs::GetFileSize(out_size, file)))) {
                    ShowFatalError("Failed to get size (0x%08" PRIx32 ") of %s!\n", result.GetValue(), path);
                }

                /* Allocate file. */
                void *data = AllocateAligned(*out_size, std::max<size_t>(align, 0x10));

                /* Read the file. */
                if (R_FAILED((result = fs::ReadFile(file, 0, data, *out_size)))) {
                    ShowFatalError("Failed to read (0x%08" PRIx32 ") %s!\n", result.GetValue(), path);
                }

                return data;
            } else {
                return nullptr;
            }
        }

    }

    u32 ConfigureStratosphere(const u8 *nn_package2, ams::TargetFirmware target_firmware, bool emummc_enabled, bool nogc_enabled) {
        /* Load KIPs off the SD card. */
        {
            /* Create kip dir path. */
            char kip_path[0x120];
            std::memcpy(kip_path, "sdmc:/atmosphere/kips", 0x16);

            fs::DirectoryHandle kip_dir;
            if (R_SUCCEEDED(fs::OpenDirectory(std::addressof(kip_dir), kip_path))) {
                ON_SCOPE_EXIT { fs::CloseDirectory(kip_dir); };

                s64 count;
                fs::DirectoryEntry entries[1];
                while (R_SUCCEEDED(fs::ReadDirectory(std::addressof(count), entries, kip_dir, util::size(entries))) && count > 0) {
                    /* Check that file is a file. */
                    if (fs::GetEntryType(entries[0]) != fs::DirectoryEntryType_File) {
                        continue;
                    }

                    /* Get filename length. */
                    const int name_len = std::strlen(entries[0].file_name);

                    /* Adjust kip path. */
                    kip_path[0x15] = '/';
                    std::memcpy(kip_path + 0x16, entries[0].file_name, name_len + 1);

                    /* Check that file is ".kip" or ".kip1" file. */
                    const int path_len = 0x16 + name_len;
                    if (std::memcmp(kip_path + path_len - 4, ".kip", 5) != 0 && std::memcmp(kip_path + path_len - 5, ".kip1", 6) != 0) {
                        continue;
                    }

                    /* Read the kip. */
                    s64 file_size;
                    if (InitialProcessHeader *kip = static_cast<InitialProcessHeader *>(ReadFile(std::addressof(file_size), kip_path, alignof(InitialProcessHeader))); kip != nullptr) {
                        /* If the kip is valid, add it. */
                        if (kip->magic == InitialProcessHeader::Magic && file_size == GetInitialProcessSize(kip)) {
                            AddInitialProcess(kip);
                        }
                    }
                }
            }
        }

        /* Add the stratosphere kips. */
        {
            const auto &secondary_archive = GetSecondaryArchive();
            for (u32 i = 0; i < secondary_archive.header.num_kips; ++i) {
                const auto &meta = secondary_archive.header.kip_metas[i];

                AddInitialProcess(reinterpret_cast<const InitialProcessHeader *>(secondary_archive.kips + meta.offset), std::addressof(meta.hash));
            }
        }

        /* Get meta for FS process. */
        constexpr u64 FsProgramId = 0x0100000000000000;
        auto *fs_meta = FindInitialProcess(FsProgramId);
        if (fs_meta == nullptr) {
            /* Get nintendo header/data. */
            const pkg2::Package2Header *nn_header = reinterpret_cast<const pkg2::Package2Header *>(nn_package2);
            const u8 *nn_data = nn_package2 + sizeof(*nn_header);

            /* Get Nintendo INI1. */
            const InitialProcessBinaryHeader *nn_ini = FindInitialProcessBinary(nn_header, nn_data, target_firmware);
            if (nn_ini == nullptr || nn_ini->magic != InitialProcessBinaryHeader::Magic) {
                ShowFatalError("Failed to find INI1!\n");
            }

            /* Find FS KIP. */
            const InitialProcessHeader *nn_fs_kip = FindInitialProcessInBinary(nn_ini, FsProgramId);
            if (nn_fs_kip == nullptr) {
                ShowFatalError("Failed to find FS!\n");
            }

            /* Add to binary. */
            AddInitialProcess(nn_fs_kip);

            /* Re-find meta. */
            fs_meta = FindInitialProcess(FsProgramId);
        }

        /* Check that we found FS. */
        if (fs_meta == nullptr) {
            ShowFatalError("Failed to find FS!\n");
        }

        /* Get FS version. */
        const auto fs_version = GetFsVersion(fs_meta->kip_hash);
        if (fs_version >= FsVersion_Count) {
            if (emummc_enabled || nogc_enabled) {
                ShowFatalError("Failed to identify FS!\n");
            }
        }

        /* If emummc is enabled, we need to decompress fs .text. */
        if (emummc_enabled) {
            fs_meta->patch_segments |= (1 << 0);
        }

        /* Parse/prepare relevant nogc/kip patches. */
        {
            /* Add nogc patches. */
            if (nogc_enabled) {
                AddNogcPatches(fs_meta, fs_version);
            }

            /* TODO ams.tma2: add mount_host patches. */

            /* Add generic patches. */
            {
                /* Create patch path. */
                char patch_path[0x220];
                std::memcpy(patch_path, "sdmc:/atmosphere/kip_patches", 0x1D);

                fs::DirectoryHandle patch_root_dir;
                if (R_SUCCEEDED(fs::OpenDirectory(std::addressof(patch_root_dir), patch_path))) {
                    ON_SCOPE_EXIT { fs::CloseDirectory(patch_root_dir); };

                    s64 count;
                    fs::DirectoryEntry entries[1];
                    while (R_SUCCEEDED(fs::ReadDirectory(std::addressof(count), entries, patch_root_dir, util::size(entries))) && count > 0) {
                        /* Check that dir is a dir. */
                        if (fs::GetEntryType(entries[0]) != fs::DirectoryEntryType_Directory) {
                            continue;
                        }

                        /* For compatibility, ignore the old "default_nogc" patches. */
                        if (std::strcmp(entries[0].file_name, "default_nogc") == 0) {
                            continue;
                        }

                        /* Get filename length. */
                        const int dir_len = std::strlen(entries[0].file_name);

                        /* Adjust patch path. */
                        patch_path[0x1C] = '/';
                        std::memcpy(patch_path + 0x1D, entries[0].file_name, dir_len + 1);

                        /* Try to open the patch subdirectory. */
                        fs::DirectoryHandle patch_dir;
                        if (R_SUCCEEDED(fs::OpenDirectory(std::addressof(patch_dir), patch_path))) {
                            ON_SCOPE_EXIT { fs::CloseDirectory(patch_dir); };

                            /* Read patches. */
                            while (R_SUCCEEDED(fs::ReadDirectory(std::addressof(count), entries, patch_dir, util::size(entries))) && count > 0) {
                                /* Check that file is a file. */
                                if (fs::GetEntryType(entries[0]) != fs::DirectoryEntryType_File) {
                                    continue;
                                }

                                /* Get filename length. */
                                const int name_len = std::strlen(entries[0].file_name);

                                /* Adjust patch path. */
                                patch_path[0x1D + dir_len] = '/';
                                std::memcpy(patch_path + 0x1D + dir_len + 1, entries[0].file_name, name_len + 1);

                                /* Check that file is "{hex}.ips" file. */
                                const int path_len = 0x1D + dir_len + 1 + name_len;
                                if (name_len != 0x44 || std::memcmp(patch_path + path_len - 4, ".ips", 5) != 0) {
                                    continue;
                                }

                                /* Check that the filename is hex. */
                                bool valid_name = true;
                                se::Sha256Hash patch_name = {};
                                u32 shift = 4;
                                for (int i = 0; i < name_len - 4; ++i) {
                                    const char c = entries[0].file_name[i];

                                    u8 val;
                                    if ('0' <= c && c <= '9') {
                                        val = (c - '0');
                                    } else if ('a' <= c && c <= 'f') {
                                        val = (c - 'a') + 10;
                                    } else if ('A' <= c && c <= 'F') {
                                        val = (c - 'A') + 10;
                                    } else {
                                        valid_name = false;
                                        break;
                                    }

                                    patch_name.bytes[i >> 1] |= val << shift;
                                    shift ^= 4;
                                }

                                /* Ignore invalid patches. */
                                if (!valid_name) {
                                    continue;
                                }

                                /* Find kip for the patch. */
                                auto *kip_meta = FindInitialProcess(patch_name);
                                if (kip_meta == nullptr) {
                                    continue;
                                }

                                /* Read the ips patch. */
                                s64 file_size;
                                if (u8 *ips = static_cast<u8 *>(ReadFile(std::addressof(file_size), patch_path)); ips != nullptr) {
                                    AddIpsPatchToKip(kip_meta, ips, static_cast<s32>(file_size));
                                }
                            }
                        }
                    }
                }
            }
        }

        /* Return the fs version we're using. */
        return static_cast<u32>(fs_version);
    }

    void RebuildPackage2(ams::TargetFirmware target_firmware, bool emummc_enabled) {
        /* TODO */
    }

}
