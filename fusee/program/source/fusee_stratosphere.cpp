/*
 * Copyright (c) Atmosphère-NX
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
#include "fusee_external_package.hpp"
#include "fs/fusee_fs_api.hpp"

namespace ams::nxboot {

    namespace {

        constexpr u32 MesoshereMetadataLayout0Magic = util::FourCC<'M','S','S','0'>::Code;
        constexpr u32 MesoshereMetadataLayout1Magic = util::FourCC<'M','S','S','1'>::Code;

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

        constexpr inline const u64 FsProgramId = 0x0100000000000000;

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

            FsVersion_13_0_0,
            FsVersion_13_0_0_Exfat,

            FsVersion_13_1_0,
            FsVersion_13_1_0_Exfat,

            FsVersion_14_0_0,
            FsVersion_14_0_0_Exfat,

            FsVersion_15_0_0,
            FsVersion_15_0_0_Exfat,

            FsVersion_16_0_0,
            FsVersion_16_0_0_Exfat,

            FsVersion_16_0_3,
            FsVersion_16_0_3_Exfat,

            FsVersion_17_0_0,
            FsVersion_17_0_0_Exfat,

            FsVersion_18_0_0,
            FsVersion_18_0_0_Exfat,

            FsVersion_18_1_0,
            FsVersion_18_1_0_Exfat,

            FsVersion_19_0_0,
            FsVersion_19_0_0_Exfat,

            FsVersion_20_0_0,
            FsVersion_20_0_0_Exfat,

            FsVersion_20_1_0,
            FsVersion_20_1_0_Exfat,

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

            { 0x7D, 0x20, 0x05, 0x47, 0x17, 0x8A, 0x83, 0x6A }, /* FsVersion_13_0_0 */
            { 0x51, 0xEB, 0xFA, 0x9C, 0xCF, 0x66, 0xC0, 0x9E }, /* FsVersion_13_0_0_Exfat */

            { 0x91, 0xBA, 0x65, 0xA2, 0x1C, 0x1D, 0x50, 0xAE }, /* FsVersion_13_1_0 */
            { 0x76, 0x38, 0x27, 0xEE, 0x9C, 0x20, 0x7E, 0x5B }, /* FsVersion_13_1_0_Exfat */

            { 0x88, 0x7A, 0xC1, 0x50, 0x80, 0x6C, 0x75, 0xCC }, /* FsVersion_14_0_0 */
            { 0xD4, 0x88, 0xD1, 0xF2, 0x92, 0x17, 0x35, 0x5C }, /* FsVersion_14_0_0_Exfat */

            { 0xD0, 0xD4, 0x49, 0x18, 0x14, 0xB5, 0x62, 0xAF }, /* FsVersion_15_0_0 */
            { 0x34, 0xC0, 0xD9, 0xED, 0x6A, 0xD1, 0x87, 0x3D }, /* FsVersion_15_0_0_Exfat */

            { 0x56, 0xE8, 0x56, 0x56, 0x6C, 0x38, 0xD8, 0xBE }, /* FsVersion_16_0_0 */
            { 0xCF, 0xAB, 0x45, 0x0C, 0x2C, 0x53, 0x9D, 0xA9 }, /* FsVersion_16_0_0_Exfat */

            { 0x39, 0xEE, 0x1F, 0x1E, 0x0E, 0xA7, 0x32, 0x5D }, /* FsVersion_16_0_3 */
            { 0x62, 0xC6, 0x5E, 0xFD, 0x9A, 0xBF, 0x7C, 0x43 }, /* FsVersion_16_0_3_Exfat */

            { 0x27, 0x07, 0x3B, 0xF0, 0xA1, 0xB8, 0xCE, 0x61 }, /* FsVersion_17_0_0 */
            { 0xEE, 0x0F, 0x4B, 0xAC, 0x6D, 0x1F, 0xFC, 0x4B }, /* FsVersion_17_0_0_Exfat */

            { 0x79, 0x5F, 0x5A, 0x5E, 0xB0, 0xC6, 0x77, 0x9E }, /* FsVersion_18_0_0 */
            { 0x1E, 0x2C, 0x64, 0xB1, 0xCC, 0xE2, 0x78, 0x24 }, /* FsVersion_18_0_0_Exfat */

            { 0xA3, 0x39, 0xF0, 0x1C, 0x95, 0xBF, 0xA7, 0x68 }, /* FsVersion_18_1_0 */
            { 0x20, 0x4C, 0xBA, 0x86, 0xDE, 0x08, 0x44, 0x6A }, /* FsVersion_18_1_0_Exfat */

            { 0xD9, 0x4C, 0x68, 0x15, 0xF8, 0xF5, 0x0A, 0x20 }, /* FsVersion_19_0_0 */
            { 0xED, 0xA8, 0x78, 0x68, 0xA4, 0x49, 0x07, 0x50 }, /* FsVersion_19_0_0_Exfat */

            { 0x63, 0x54, 0x96, 0x9E, 0x60, 0xA7, 0x97, 0x7B }, /* FsVersion_20_0_0 */
            { 0x47, 0x41, 0x07, 0x10, 0x65, 0x4F, 0xA4, 0x3F }, /* FsVersion_20_0_0_Exfat */

            { 0xED, 0x34, 0xB4, 0x50, 0x58, 0x4A, 0x5B, 0x43 }, /* FsVersion_20_1_0 */
            { 0xA5, 0x1A, 0xA4, 0x92, 0x6C, 0x41, 0x87, 0x59 }, /* FsVersion_20_1_0_Exfat */
        };

        const InitialProcessBinaryHeader *FindInitialProcessBinary(const pkg2::Package2Header *header, const u8 *data, ams::TargetFirmware target_firmware) {
            if (target_firmware >= ams::TargetFirmware_17_0_0) {
                const u32 *data_32 = reinterpret_cast<const u32 *>(data);
                const u32 branch_target = (data_32[0] & 0x00FFFFFF);
                for (size_t i = branch_target; i < branch_target + 0x1000 / sizeof(u32); ++i) {
                    const u32 ini_offset = (i * sizeof(u32)) + data_32[i];
                    if (data_32[i + 1] == 0 && ini_offset <= header->meta.payload_sizes[0] && std::memcmp(data + ini_offset, "INI1", 4) == 0) {
                        return reinterpret_cast<const InitialProcessBinaryHeader *>(data + ini_offset);
                    }
                }

                return nullptr;
            } else if (target_firmware >= ams::TargetFirmware_8_0_0) {
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
            /* Check kip magic. */
            if (kip->magic != InitialProcessHeader::Magic) {
                ShowFatalError("KIP seems corrupted!\n");
            }

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
                case FsVersion_13_0_0:
                case FsVersion_13_0_0_Exfat:
                    AddPatch(fs_meta, 0x159119, NogcPatch0, sizeof(NogcPatch0));
                    AddPatch(fs_meta, 0x1426D0, NogcPatch1, sizeof(NogcPatch1));
                    break;
                case FsVersion_13_1_0:
                case FsVersion_13_1_0_Exfat:
                    AddPatch(fs_meta, 0x1590B9, NogcPatch0, sizeof(NogcPatch0));
                    AddPatch(fs_meta, 0x142670, NogcPatch1, sizeof(NogcPatch1));
                    break;
                case FsVersion_14_0_0:
                    AddPatch(fs_meta, 0x18A3E9, NogcPatch0, sizeof(NogcPatch0));
                    AddPatch(fs_meta, 0x164330, NogcPatch1, sizeof(NogcPatch1));
                    break;
                case FsVersion_14_0_0_Exfat:
                    AddPatch(fs_meta, 0x195769, NogcPatch0, sizeof(NogcPatch0));
                    AddPatch(fs_meta, 0x16F6B0, NogcPatch1, sizeof(NogcPatch1));
                    break;
                case FsVersion_15_0_0:
                    AddPatch(fs_meta, 0x184259, NogcPatch0, sizeof(NogcPatch0));
                    AddPatch(fs_meta, 0x15EDE4, NogcPatch1, sizeof(NogcPatch1));
                    break;
                case FsVersion_15_0_0_Exfat:
                    AddPatch(fs_meta, 0x18F1E9, NogcPatch0, sizeof(NogcPatch0));
                    AddPatch(fs_meta, 0x169D74, NogcPatch1, sizeof(NogcPatch1));
                    break;
                case FsVersion_16_0_0:
                    AddPatch(fs_meta, 0x1866D9, NogcPatch0, sizeof(NogcPatch0));
                    AddPatch(fs_meta, 0x160C70, NogcPatch1, sizeof(NogcPatch1));
                    break;
                case FsVersion_16_0_0_Exfat:
                    AddPatch(fs_meta, 0x1913B9, NogcPatch0, sizeof(NogcPatch0));
                    AddPatch(fs_meta, 0x16B950, NogcPatch1, sizeof(NogcPatch1));
                    break;
                case FsVersion_16_0_3:
                    AddPatch(fs_meta, 0x186729, NogcPatch0, sizeof(NogcPatch0));
                    AddPatch(fs_meta, 0x160CC0, NogcPatch1, sizeof(NogcPatch1));
                    break;
                case FsVersion_16_0_3_Exfat:
                    AddPatch(fs_meta, 0x191409, NogcPatch0, sizeof(NogcPatch0));
                    AddPatch(fs_meta, 0x16B9A0, NogcPatch1, sizeof(NogcPatch1));
                    break;
                case FsVersion_17_0_0:
                    AddPatch(fs_meta, 0x18B149, NogcPatch0, sizeof(NogcPatch0));
                    AddPatch(fs_meta, 0x165200, NogcPatch1, sizeof(NogcPatch1));
                    break;
                case FsVersion_17_0_0_Exfat:
                    AddPatch(fs_meta, 0x195FA9, NogcPatch0, sizeof(NogcPatch0));
                    AddPatch(fs_meta, 0x170060, NogcPatch1, sizeof(NogcPatch1));
                    break;
                case FsVersion_18_0_0:
                    AddPatch(fs_meta, 0x18AF49, NogcPatch0, sizeof(NogcPatch0));
                    AddPatch(fs_meta, 0x164B50, NogcPatch1, sizeof(NogcPatch1));
                    break;
                case FsVersion_18_0_0_Exfat:
                    AddPatch(fs_meta, 0x195FD9, NogcPatch0, sizeof(NogcPatch0));
                    AddPatch(fs_meta, 0x16FBE0, NogcPatch1, sizeof(NogcPatch1));
                    break;
                case FsVersion_18_1_0:
                    AddPatch(fs_meta, 0x18AF49, NogcPatch0, sizeof(NogcPatch0));
                    AddPatch(fs_meta, 0x164B50, NogcPatch1, sizeof(NogcPatch1));
                    break;
                case FsVersion_18_1_0_Exfat:
                    AddPatch(fs_meta, 0x195FD9, NogcPatch0, sizeof(NogcPatch0));
                    AddPatch(fs_meta, 0x16FBE0, NogcPatch1, sizeof(NogcPatch1));
                    break;
                case FsVersion_19_0_0:
                    AddPatch(fs_meta, 0x195C75, NogcPatch0, sizeof(NogcPatch0));
                    AddPatch(fs_meta, 0x195E75, NogcPatch0, sizeof(NogcPatch0));
                    AddPatch(fs_meta, 0x16F170, NogcPatch1, sizeof(NogcPatch1));
                    break;
                case FsVersion_19_0_0_Exfat:
                    AddPatch(fs_meta, 0x1A14A5, NogcPatch0, sizeof(NogcPatch0));
                    AddPatch(fs_meta, 0x1A16A5, NogcPatch0, sizeof(NogcPatch0));
                    AddPatch(fs_meta, 0x17A9A0, NogcPatch1, sizeof(NogcPatch1));
                    break;
                case FsVersion_20_0_0:
                case FsVersion_20_1_0:
                    AddPatch(fs_meta, 0x1A7E25, NogcPatch0, sizeof(NogcPatch0));
                    AddPatch(fs_meta, 0x1A8025, NogcPatch0, sizeof(NogcPatch0));
                    AddPatch(fs_meta, 0x17C250, NogcPatch1, sizeof(NogcPatch1));
                    break;
                case FsVersion_20_0_0_Exfat:
                case FsVersion_20_1_0_Exfat:
                    AddPatch(fs_meta, 0x1B3745, NogcPatch0, sizeof(NogcPatch0));
                    AddPatch(fs_meta, 0x1B3945, NogcPatch0, sizeof(NogcPatch0));
                    AddPatch(fs_meta, 0x187B70, NogcPatch1, sizeof(NogcPatch1));
                    break;
                default:
                    break;
            }
        }

        struct BlzSegmentFlags {
            using Offset = util::BitPack16::Field<0,            12, u32>;
            using Size   = util::BitPack16::Field<Offset::Next,  4, u32>;
        };

        void BlzUncompress(void *_end) {
            /* Parse the footer, endian agnostic. */
            static_assert(sizeof(u32) == 4);
            static_assert(sizeof(u16) == 2);
            static_assert(sizeof(u8)  == 1);

            u8 *end = static_cast<u8 *>(_end);
            const u32 total_size      = (end[-12] << 0) | (end[-11] << 8) | (end[-10] << 16) | (end[- 9] << 24);
            const u32 footer_size     = (end[- 8] << 0) | (end[- 7] << 8) | (end[- 6] << 16) | (end[- 5] << 24);
            const u32 additional_size = (end[- 4] << 0) | (end[- 3] << 8) | (end[- 2] << 16) | (end[- 1] << 24);

            /* Prepare to decompress. */
            u8 *cmp_start = end - total_size;
            u32 cmp_ofs = total_size - footer_size;
            u32 out_ofs = total_size + additional_size;

            /* Decompress. */
            while (out_ofs) {
                u8 control = cmp_start[--cmp_ofs];

                /* Each bit in the control byte is a flag indicating compressed or not compressed. */
                for (size_t i = 0; i < 8 && out_ofs; ++i, control <<= 1) {
                    if (control & 0x80) {
                        /* NOTE: Nintendo does not check if it's possible to decompress. */
                        /* As such, we will leave the following as a debug assertion, and not a release assertion. */
                        AMS_AUDIT(cmp_ofs >= sizeof(u16));
                        cmp_ofs -= sizeof(u16);

                        /* Extract segment bounds. */
                        const util::BitPack16 seg_flags{static_cast<u16>((cmp_start[cmp_ofs] << 0) | (cmp_start[cmp_ofs + 1] << 8))};
                        const u32 seg_ofs  = seg_flags.Get<BlzSegmentFlags::Offset>() + 3;
                        const u32 seg_size = std::min(seg_flags.Get<BlzSegmentFlags::Size>() + 3, out_ofs);
                        AMS_AUDIT(out_ofs + seg_ofs <= total_size + additional_size);

                        /* Copy the data. */
                        out_ofs -= seg_size;
                        for (size_t j = 0; j < seg_size; j++) {
                            cmp_start[out_ofs + j] = cmp_start[out_ofs + seg_ofs + j];
                        }
                    } else {
                        /* NOTE: Nintendo does not check if it's possible to copy. */
                        /* As such, we will leave the following as a debug assertion, and not a release assertion. */
                        AMS_AUDIT(cmp_ofs >= sizeof(u8));
                        cmp_start[--out_ofs] = cmp_start[--cmp_ofs];
                    }
                }
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
            const auto &external_package = GetExternalPackage();
            for (u32 i = 0; i < external_package.header.num_kips; ++i) {
                const auto &meta = external_package.header.kip_metas[i];

                AddInitialProcess(reinterpret_cast<const InitialProcessHeader *>(external_package.kips + meta.offset), std::addressof(meta.hash));
            }
        }

        /* Get meta for FS process. */
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
        }

        /* Return the fs version we're using. */
        return static_cast<u32>(fs_version);
    }

    void RebuildPackage2(ams::TargetFirmware target_firmware, bool emummc_enabled) {
        /* Get the external package. */
        const auto &external_package = GetExternalPackage();

        /* Clear package2 header. */
        auto *package2 = secmon::MemoryRegionDramPackage2.GetPointer<pkg2::Package2Header>();
        std::memset(package2, 0, sizeof(*package2));

        /* Get payload data pointer. */
        u8 * const payload_data = reinterpret_cast<u8 *>(package2 + 1);

        /* Useful values. */
        constexpr u32 KernelPayloadBase = 0x60000;

        /* Set fields. */
        package2->meta.key_generation = pkg1::KeyGeneration_Current;
        std::memcpy(package2->meta.magic, pkg2::Package2Meta::Magic::String, sizeof(package2->meta.magic));
        package2->meta.entrypoint         = KernelPayloadBase;
        package2->meta.bootloader_version = pkg2::CurrentBootloaderVersion;
        package2->meta.package2_version   = pkg2::MinimumValidDataVersion;

        /* Load mesosphere. */
        s64 meso_size;
        if (void *sd_meso = ReadFile(std::addressof(meso_size), "sdmc:/atmosphere/mesosphere.bin"); sd_meso != nullptr) {
            std::memcpy(payload_data, sd_meso, meso_size);
        } else {
            meso_size = external_package.header.meso_size;
            std::memcpy(payload_data, external_package.mesosphere, meso_size);
        }

        /* Read emummc, if needed. */
        const InitialProcessHeader *emummc;
        s64 emummc_size;
        if (emummc_enabled) {
            emummc = static_cast<const InitialProcessHeader *>(ReadFile(std::addressof(emummc_size), "sdmc:/atmosphere/emummc.kip"));
            if (emummc == nullptr) {
                emummc      = reinterpret_cast<const InitialProcessHeader *>(external_package.kips + external_package.header.emummc_meta.offset);
                emummc_size = external_package.header.emummc_meta.size;
            }
        }

        /* Set the embedded ini pointer. */
        const u32 magic = *reinterpret_cast<const u32 *>(payload_data + 4);
        if (magic == MesoshereMetadataLayout0Magic) {
            std::memcpy(payload_data + 8, std::addressof(meso_size), sizeof(meso_size));
        } else if (magic == MesoshereMetadataLayout1Magic) {
            if (const u32 meta_offset = *reinterpret_cast<const u32 *>(payload_data + 8); meta_offset <= meso_size - sizeof(meso_size)) {
                s64 relative_offset = meso_size - meta_offset;
                std::memcpy(payload_data + meta_offset, std::addressof(relative_offset), sizeof(relative_offset));
            } else {
                ShowFatalError("Invalid mesosphere metadata layout!\n");
            }
        } else {
            ShowFatalError("Unknown mesosphere metadata version!\n");
        }


        /* Get the ini pointer. */
        InitialProcessBinaryHeader * const ini = reinterpret_cast<InitialProcessBinaryHeader *>(payload_data + meso_size);

        /* Set ini fields. */
        ini->magic         = InitialProcessBinaryHeader::Magic;
        ini->num_processes = 0;
        ini->reserved      = 0;

        /* Iterate all processes. */
        u8 * const dst_kip_start = reinterpret_cast<u8 *>(ini + 1);
        u8 *       dst_kip_cur   = dst_kip_start;

        for (InitialProcessMeta *meta = std::addressof(g_initial_process_meta); meta != nullptr; meta = meta->next) {
            /* Get the current kip. */
            const auto *src_kip = meta->kip;
                  auto *dst_kip = reinterpret_cast<InitialProcessHeader *>(dst_kip_cur);

            /* Copy the kip header */
            std::memcpy(dst_kip, src_kip, sizeof(*src_kip));

            const u8 *src_kip_data = reinterpret_cast<const u8 *>(src_kip + 1);
                  u8 *dst_kip_data = reinterpret_cast<      u8 *>(dst_kip + 1);

            /* If necessary, inject emummc. */
            u32 addl_text_offset = 0;
            if (dst_kip->program_id == FsProgramId && emummc_enabled) {
                /* Get emummc extents. */
                addl_text_offset = emummc->bss_address + emummc->bss_size;
                if ((emummc->flags & 7) || !util::IsAligned(addl_text_offset, 0x1000)) {
                    ShowFatalError("Invalid emummc kip!\n");
                }

                /* Copy emummc capabilities. */
                {
                    std::memcpy(dst_kip->capabilities, emummc->capabilities, sizeof(emummc->capabilities));

                    if (target_firmware <= ams::TargetFirmware_1_0_0) {
                        for (size_t i = 0; i < util::size(dst_kip->capabilities); ++i) {
                            if (dst_kip->capabilities[i] == 0xFFFFFFFF) {
                                dst_kip->capabilities[i] = 0x07000E7F;
                                break;
                            }
                        }
                    }
                }

                /* Update section headers. */
                dst_kip->ro_address  += addl_text_offset;
                dst_kip->rw_address  += addl_text_offset;
                dst_kip->bss_address += addl_text_offset;

                /* Get emummc sections. */
                const u8 *emummc_data = reinterpret_cast<const u8 *>(emummc + 1);

                /* Copy emummc sections. */
                std::memcpy(dst_kip_data + emummc->rx_address, emummc_data, emummc->rx_compressed_size);
                std::memcpy(dst_kip_data + emummc->ro_address, emummc_data + emummc->rx_compressed_size, emummc->ro_compressed_size);
                std::memcpy(dst_kip_data + emummc->rw_address, emummc_data + emummc->rx_compressed_size + emummc->ro_compressed_size, emummc->rw_compressed_size);
                std::memset(dst_kip_data + emummc->bss_address, 0, emummc->bss_size);

                /* Advance. */
                dst_kip_data += addl_text_offset;
            }

            /* Prepare to process segments. */
            u8 *dst_rx_data, *dst_ro_data, *dst_rw_data;

            /* Process .text. */
            {
                dst_rx_data = dst_kip_data;

                std::memcpy(dst_kip_data, src_kip_data, src_kip->rx_compressed_size);

                /* Uncompress, if necessary. */
                if ((meta->patch_segments & src_kip->flags) & (1 << 0)) {
                    BlzUncompress(dst_kip_data + dst_kip->rx_compressed_size);

                    dst_kip->rx_compressed_size = dst_kip->rx_size;
                }

                /* Advance. */
                dst_kip_data += dst_kip->rx_compressed_size;
                src_kip_data += src_kip->rx_compressed_size;

                /* Account for potential emummc. */
                dst_kip->rx_size            += addl_text_offset;
                dst_kip->rx_compressed_size += addl_text_offset;
            }

            /* Process .rodata. */
            {
                dst_ro_data = dst_kip_data;

                std::memcpy(dst_kip_data, src_kip_data, src_kip->ro_compressed_size);

                /* Uncompress, if necessary. */
                if ((meta->patch_segments & src_kip->flags) & (1 << 1)) {
                    BlzUncompress(dst_kip_data + dst_kip->ro_compressed_size);

                    dst_kip->ro_compressed_size = dst_kip->ro_size;
                }

                /* Advance. */
                dst_kip_data += dst_kip->ro_compressed_size;
                src_kip_data += src_kip->ro_compressed_size;
            }

            /* Process .rwdata. */
            {
                dst_rw_data = dst_kip_data;

                std::memcpy(dst_kip_data, src_kip_data, src_kip->rw_compressed_size);

                /* Uncompress, if necessary. */
                if ((meta->patch_segments & src_kip->flags) & (1 << 2)) {
                    BlzUncompress(dst_kip_data + dst_kip->rw_compressed_size);

                    dst_kip->rw_compressed_size = dst_kip->rw_size;
                }

                /* Advance. */
                dst_kip_data += dst_kip->rw_compressed_size;
                src_kip_data += src_kip->rw_compressed_size;
            }

            /* Adjust flags. */
            dst_kip->flags &= ~meta->patch_segments;

            /* Apply patches. */
            for (auto *patch = meta->patches_head; patch != nullptr; patch = patch->next) {
                /* Get the destination segment. */
                u8 *patch_dst_segment;
                switch (patch->start_segment) {
                    case 0:  patch_dst_segment = dst_rx_data; break;
                    case 1:  patch_dst_segment = dst_ro_data; break;
                    case 2:  patch_dst_segment = dst_rw_data; break;
                    default: ShowFatalError("Unknown patch segment %" PRIu32 "\n", patch->start_segment); break;
                }

                /* Get the destination. */
                u8 * const patch_dst = patch_dst_segment + patch->rel_offset;

                /* Apply the patch. */
                if (patch->is_memset) {
                    const u8 val = *static_cast<const u8 *>(patch->data);
                    std::memset(patch_dst, val, patch->size);
                } else {
                    std::memcpy(patch_dst, patch->data, patch->size);
                }
            }

            /* Advance. */
            dst_kip_cur += GetInitialProcessSize(dst_kip);

            /* Increment num kips. */
            ++ini->num_processes;
        }

        /* Set INI size. */
        ini->size = sizeof(*ini) + (dst_kip_cur - dst_kip_start);
        if (ini->size > 12_MB) {
            ShowFatalError("INI is too big! (0x%08" PRIx32 ")\n", ini->size);
        }

        /* Set the payload size/offset. */
        package2->meta.payload_offsets[0] = KernelPayloadBase;
        package2->meta.payload_sizes[0]   = util::AlignUp(meso_size + ini->size, 0x10);


        /* Set total size. */
        package2->meta.package2_size = sizeof(*package2) + package2->meta.payload_sizes[0];
    }

}
