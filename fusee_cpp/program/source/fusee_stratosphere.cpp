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
            u32 offset;
            void *data;
            u32 size;
        };

        struct alignas(0x10) InitialProcessMeta {
            InitialProcessMeta *next = nullptr;
            const InitialProcessHeader *kip;
            u32 kip_size;
            PatchMeta *patches;
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
            meta->patches = nullptr;
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

        const InitialProcessMeta *FindInitialProcess(u64 program_id) {
            for (const InitialProcessMeta *cur = std::addressof(g_initial_process_meta); cur != nullptr; cur = cur->next) {
                if (cur->program_id == program_id) {
                    return cur;
                }
            }
            return nullptr;
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

                    /* Check that file is a file. */
                    if (fs::GetEntryType(entries[0]) != fs::DirectoryEntryType_File) {
                        continue;
                    }

                    /* Open the kip. */
                    fs::FileHandle kip_file;
                    if (R_SUCCEEDED(fs::OpenFile(std::addressof(kip_file), kip_path, fs::OpenMode_Read))) {
                        ON_SCOPE_EXIT { fs::CloseFile(kip_file); };

                        Result result;

                        /* Get the kip size. */
                        s64 file_size;
                        if (R_FAILED((result = fs::GetFileSize(std::addressof(file_size), kip_file)))) {
                            ShowFatalError("Failed to get size (0x%08" PRIx32 ") of %s!\n", result.GetValue(), kip_path);
                        }

                        /* Allocate kip. */
                        InitialProcessHeader *kip = static_cast<InitialProcessHeader *>(AllocateAligned(file_size, alignof(InitialProcessHeader)));

                        /* Read the kip. */
                        if (R_FAILED((result = fs::ReadFile(kip_file, 0, kip, file_size)))) {
                            ShowFatalError("Failed to read (0x%08" PRIx32 ") %s!\n", result.GetValue(), kip_path);
                        }

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
        const auto *fs_meta = FindInitialProcess(FsProgramId);
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

        /* TODO: Parse/prepare relevant nogc/kip patches. */

        /* Return the fs version we're using. */
        return static_cast<u32>(fs_version);
    }

    void RebuildPackage2(ams::TargetFirmware target_firmware, bool emummc_enabled) {
        /* TODO */
    }

}
