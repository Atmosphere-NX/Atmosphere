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
#include "fusee_emummc.hpp"
#include "fusee_mmc.hpp"
#include "fusee_sd_card.hpp"
#include "fusee_fatal.hpp"
#include "fusee_malloc.hpp"
#include "fs/fusee_fs_api.hpp"
#include "fs/fusee_fs_storage.hpp"

namespace ams::nxboot {

    namespace {

        class SdCardStorage : public fs::IStorage {
            public:
                virtual Result Read(s64 offset, void *buffer, size_t size) override {
                    if (!util::IsAligned(offset, sdmmc::SectorSize) || !util::IsAligned(size, sdmmc::SectorSize)) {
                        ShowFatalError("SdCard: unaligned access to %" PRIx64 ", size=%" PRIx64"\n", static_cast<u64>(offset), static_cast<u64>(size));
                    }

                    return ReadSdCard(buffer, size, offset / sdmmc::SectorSize, size / sdmmc::SectorSize);
                }

                virtual Result Flush() override {
                    return ResultSuccess();
                }

                virtual Result GetSize(s64 *out) override {
                    u32 num_sectors;
                    R_TRY(GetSdCardMemoryCapacity(std::addressof(num_sectors)));

                    *out = static_cast<s64>(num_sectors) * static_cast<s64>(sdmmc::SectorSize);
                    return ResultSuccess();
                }

                virtual Result Write(s64 offset, const void *buffer, size_t size) override {
                    return fs::ResultUnsupportedOperation();
                }

                virtual Result SetSize(s64 size) override {
                    return fs::ResultUnsupportedOperation();
                }
        };

        template<sdmmc::MmcPartition Partition>
        class MmcPartitionStorage : public fs::IStorage {
            public:
                constexpr MmcPartitionStorage() { /* ... */ }

                virtual Result Read(s64 offset, void *buffer, size_t size) override {
                    if (!util::IsAligned(offset, sdmmc::SectorSize) || !util::IsAligned(size, sdmmc::SectorSize)) {
                        ShowFatalError("SdCard: unaligned access to %" PRIx64 ", size=%" PRIx64"\n", static_cast<u64>(offset), static_cast<u64>(size));
                    }

                    return ReadMmc(buffer, size, Partition, offset / sdmmc::SectorSize, size / sdmmc::SectorSize);
                }

                virtual Result Flush() override {
                    return ResultSuccess();
                }

                virtual Result GetSize(s64 *out) override {
                    u32 num_sectors;
                    R_TRY(GetMmcMemoryCapacity(std::addressof(num_sectors), Partition));

                    *out = num_sectors * sdmmc::SectorSize;
                    return ResultSuccess();
                }

                virtual Result Write(s64 offset, const void *buffer, size_t size) override {
                    return fs::ResultUnsupportedOperation();
                }

                virtual Result SetSize(s64 size) override {
                    return fs::ResultUnsupportedOperation();
                }
        };

        using MmcBoot0Storage = MmcPartitionStorage<sdmmc::MmcPartition_BootPartition1>;
        using MmcUserStorage  = MmcPartitionStorage<sdmmc::MmcPartition_UserData>;

        constinit char g_emummc_path[0x300];

        class EmummcFileStorage : public fs::IStorage {
            private:
                s64 m_file_size;
                fs::FileHandle m_handles[64];
                bool m_open[64];
                int m_file_path_ofs;
            private:
                void EnsureFile(int id) {
                    if (!m_open[id]) {
                        /* Update path. */
                        g_emummc_path[m_file_path_ofs + 1] = '0' + (id % 10);
                        g_emummc_path[m_file_path_ofs + 0] = '0' + (id / 10);

                        /* Open new file. */
                        const Result result = fs::OpenFile(m_handles + id, g_emummc_path, fs::OpenMode_Read);
                        if (R_FAILED(result)) {
                            ShowFatalError("Failed to open emummc user %02d file: 0x%08" PRIx32 "!\n", id, result.GetValue());
                        }

                        m_open[id] = true;
                    }
                }
            public:
                EmummcFileStorage(fs::FileHandle user00, int ofs) : m_file_path_ofs(ofs) {
                    const Result result = fs::GetFileSize(std::addressof(m_file_size), user00);
                    if (R_FAILED(result)) {
                        ShowFatalError("Failed to get emummc file size: 0x%08" PRIx32 "!\n", result.GetValue());
                    }

                    for (size_t i = 0; i < util::size(m_handles); ++i) {
                        m_open[i] = false;
                    }

                    m_handles[0] = user00;
                    m_open[0]    = true;
                }

                virtual Result Read(s64 offset, void *buffer, size_t size) override {
                    int file   = offset / m_file_size;
                    s64 subofs = offset % m_file_size;

                    u8 *cur_dst = static_cast<u8 *>(buffer);

                    for (/* ... */; size > 0; ++file) {
                        /* Ensure the current file is open. */
                        EnsureFile(file);

                        /* Perform the current read. */
                        const size_t cur_size = std::min<size_t>(m_file_size - subofs, size);
                        R_TRY(fs::ReadFile(m_handles[file], subofs, cur_dst, cur_size));

                        /* Advance. */
                        cur_dst += cur_size;
                        size -= cur_size;
                        subofs = 0;
                    }

                    return ResultSuccess();
                }

                virtual Result Flush() override {
                    return fs::ResultUnsupportedOperation();
                }

                virtual Result GetSize(s64 *out) override {
                    return fs::ResultUnsupportedOperation();
                }

                virtual Result Write(s64 offset, const void *buffer, size_t size) override {
                    return fs::ResultUnsupportedOperation();
                }

                virtual Result SetSize(s64 size) override {
                    return fs::ResultUnsupportedOperation();
                }
        };

        constinit SdCardStorage g_sd_card_storage;

        constinit MmcBoot0Storage g_mmc_boot0_storage;
        constinit MmcUserStorage g_mmc_user_storage;

        constinit fs::IStorage *g_boot0_storage = nullptr;
        constinit fs::IStorage *g_user_storage  = nullptr;

        constinit fs::SubStorage *g_package2_storage = nullptr;

        struct Guid {
            u32 data1;
            u16 data2;
            u16 data3;
            u8 data4[8];
        };
        static_assert(sizeof(Guid) == 0x10);

        struct GptHeader {
            char signature[8];
            u32 revision;
            u32 header_size;
            u32 header_crc32;
            u32 reserved0;
            u64 my_lba;
            u64 alt_lba;
            u64 first_usable_lba;
            u64 last_usable_lba;
            Guid disk_guid;
            u64 partition_entry_lba;
            u32 number_of_partition_entries;
            u32 size_of_partition_entry;
            u32 partition_entry_array_crc32;
            u32 reserved1;
        };
        static_assert(sizeof(GptHeader) == 0x60);

        struct GptPartitionEntry {
            Guid partition_type_guid;
            Guid unique_partition_guid;
            u64 starting_lba;
            u64 ending_lba;
            u64 attributes;
            char partition_name[0x48];
        };
        static_assert(sizeof(GptPartitionEntry) == 0x80);

        struct Gpt {
            GptHeader header;
            u8 padding[0x1A0];
            GptPartitionEntry entries[128];
        };
        static_assert(sizeof(Gpt) == 16_KB + 0x200);

        constexpr const u16 Package2PartitionName[] = {
            'B', 'C', 'P', 'K', 'G', '2', '-', '1', '-', 'N', 'o', 'r', 'm', 'a', 'l', '-', 'M', 'a', 'i', 'n', 0
        };

    }

    void InitializeEmummc(bool emummc_enabled, const secmon::EmummcConfiguration &emummc_cfg) {
        Result result;
        if (emummc_enabled) {
            /* Get sd card size. */
            s64 sd_card_size;
            if (R_FAILED((result = g_sd_card_storage.GetSize(std::addressof(sd_card_size))))) {
                ShowFatalError("Failed to get sd card size: 0x%08" PRIx32 "!\n", result.GetValue());
            }

            if (emummc_cfg.base_cfg.type == secmon::EmummcType_Partition) {
                const s64 partition_start = emummc_cfg.partition_cfg.start_sector * sdmmc::SectorSize;
                g_boot0_storage = AllocateObject<fs::SubStorage>(g_sd_card_storage, partition_start, 4_MB);
                g_user_storage  = AllocateObject<fs::SubStorage>(g_sd_card_storage, partition_start + 8_MB, sd_card_size - (partition_start + 8_MB));
            } else if (emummc_cfg.base_cfg.type == secmon::EmummcType_File) {
                /* Get the base emummc path. */
                std::memcpy(g_emummc_path, emummc_cfg.file_cfg.path.str, sizeof(emummc_cfg.file_cfg.path.str));

                /* Get path length. */
                auto len = std::strlen(g_emummc_path);

                /* Append emmc. */
                std::memcpy(g_emummc_path + len, "/eMMC", 6);
                len += 5;

                /* Open boot0. */
                fs::FileHandle boot0_file;
                std::memcpy(g_emummc_path + len, "/boot0", 7);
                if (R_FAILED((result = fs::OpenFile(std::addressof(boot0_file), g_emummc_path, fs::OpenMode_Read)))) {
                    ShowFatalError("Failed to open emummc boot0 file: 0x%08" PRIx32 "!\n", result.GetValue());
                }

                /* Open boot1. */
                g_emummc_path[len + 5] = '1';
                {
                    fs::DirectoryEntryType entry_type;
                    bool is_archive;
                    if (R_FAILED((result = fs::GetEntryType(std::addressof(entry_type), std::addressof(is_archive), g_emummc_path)))) {
                        ShowFatalError("Failed to find emummc boot1 file: 0x%08" PRIx32 "!\n", result.GetValue());
                    }

                    if (entry_type != fs::DirectoryEntryType_File) {
                        ShowFatalError("emummc boot1 file is not a file!\n");
                    }
                }

                /* Open userdata. */
                std::memcpy(g_emummc_path + len, "/00", 4);
                fs::FileHandle user00_file;
                if (R_FAILED((result = fs::OpenFile(std::addressof(user00_file), g_emummc_path, fs::OpenMode_Read)))) {
                    ShowFatalError("Failed to open emummc user %02d file: 0x%08" PRIx32 "!\n", 0, result.GetValue());
                }

                /* Create partitions. */
                g_boot0_storage = AllocateObject<fs::FileHandleStorage>(boot0_file);
                g_user_storage  = AllocateObject<EmummcFileStorage>(user00_file, len + 1);
            } else {
                ShowFatalError("Unknown emummc type %d\n", static_cast<int>(emummc_cfg.base_cfg.type));
            }
        } else {
            /* Initialize access to mmc. */
            {
                const Result result = InitializeMmc();
                if (R_FAILED(result)) {
                    ShowFatalError("Failed to initialize mmc: 0x%08" PRIx32 "\n", result.GetValue());
                }
            }

            /* Create storages. */
            g_boot0_storage = std::addressof(g_mmc_boot0_storage);
            g_user_storage  = std::addressof(g_mmc_user_storage);
        }
        if (g_boot0_storage == nullptr) {
            ShowFatalError("Failed to initialize BOOT0\n");
        }
        if (g_user_storage == nullptr) {
            ShowFatalError("Failed to initialize Raw EMMC\n");
        }

        /* Read the GPT. */
        Gpt *gpt = static_cast<Gpt *>(AllocateAligned(sizeof(Gpt), 0x200));
        {
            const Result result = g_user_storage->Read(0x200, gpt, sizeof(*gpt));
            if (R_FAILED(result)) {
                ShowFatalError("Failed to read GPT: 0x%08" PRIx32 "\n", result.GetValue());
            }
        }

        /* Check the GPT. */
        if (std::memcmp(gpt->header.signature, "EFI PART", 8) != 0) {
            ShowFatalError("Invalid GPT signature\n");
        }
        if (gpt->header.number_of_partition_entries > util::size(gpt->entries)) {
            ShowFatalError("Too many GPT entries\n");
        }

        /* Create system storage. */
        for (u32 i = 0; i < gpt->header.number_of_partition_entries; ++i) {
            if (gpt->entries[i].starting_lba < gpt->header.first_usable_lba) {
                continue;
            }

            const s64 offset =  INT64_C(0x200) * gpt->entries[i].starting_lba;
            const u64 size   = UINT64_C(0x200) * (gpt->entries[i].ending_lba + 1 - gpt->entries[i].starting_lba);

            if (std::memcmp(gpt->entries[i].partition_name, Package2PartitionName, sizeof(Package2PartitionName)) == 0) {
                g_package2_storage = AllocateObject<fs::SubStorage>(*g_user_storage, offset, size);
            }
        }

        /* Check that we created package2 storage. */
        if (g_package2_storage == nullptr) {
            ShowFatalError("Failed to initialize Package2\n");
        }
    }

    Result ReadBoot0(s64 offset, void *dst, size_t size) {
        return g_boot0_storage->Read(offset, dst, size);
    }

    Result ReadPackage2(s64 offset, void *dst, size_t size) {
        return g_package2_storage->Read(offset, dst, size);
    }

}
