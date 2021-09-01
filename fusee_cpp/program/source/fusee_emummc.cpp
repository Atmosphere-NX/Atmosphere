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
                fs::FileHandle m_handle;
                int m_id;
                int m_file_path_ofs;
            private:
                void SwitchFile(int id) {
                    if (m_id != id) {
                        fs::CloseFile(m_handle);

                        /* Update path. */
                        g_emummc_path[m_file_path_ofs + 1] = '0' + (id % 10);
                        g_emummc_path[m_file_path_ofs + 0] = '0' + (id / 10);

                        /* Open new file. */
                        const Result result = fs::OpenFile(std::addressof(m_handle), g_emummc_path, fs::OpenMode_Read);
                        if (R_FAILED(result)) {
                            ShowFatalError("Failed to open emummc user %02d file: 0x%08" PRIx32 "!\n", id, result.GetValue());
                        }

                        m_id = id;
                    }
                }
            public:
                EmummcFileStorage(fs::FileHandle user00, int ofs) : m_handle(user00), m_id(0), m_file_path_ofs(ofs) {
                    const Result result = fs::GetFileSize(std::addressof(m_file_size), m_handle);
                    if (R_FAILED(result)) {
                        ShowFatalError("Failed to get emummc file size: 0x%08" PRIx32 "!\n", result.GetValue());
                    }
                }

                virtual Result Read(s64 offset, void *buffer, size_t size) override {
                    int file   = offset / m_file_size;
                    s64 subofs = offset % m_file_size;

                    u8 *cur_dst = static_cast<u8 *>(buffer);

                    for (/* ... */; size > 0; ++file) {
                        /* Switch to the current file. */
                        SwitchFile(file);

                        /* Perform the current read. */
                        const size_t cur_size = std::min<size_t>(m_file_size - subofs, size);
                        R_TRY(fs::ReadFile(m_handle, subofs, cur_dst, cur_size));

                        /* Advance. */
                        cur_dst += cur_size;
                        size -= cur_size;
                        subofs = 0;
                    }

                    return ResultSuccess();
                }

                virtual Result Flush() override {
                    return fs::FlushFile(m_handle);
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

        class SystemPartitionStorage : public fs::IStorage {
            private:
                static constexpr size_t CacheEntries = BITSIZEOF(u32);
                static constexpr size_t SectorSize   = 0x4000;
            private:
                fs::SubStorage m_storage;
                u8 *m_sector_cache;
                u32 *m_sector_ids;
                u32 m_sector_flags;
                u32 m_next_idx;
            private:
                Result LoadSector(u8 *sector, u32 sector_id) {
                    /* Read the sector data. */
                    R_TRY(m_storage.Read(static_cast<s64>(sector_id) * SectorSize, sector, SectorSize));

                    /* Decrypt the sector. */
                    se::DecryptAes128Xts(sector, SectorSize, pkg1::AesKeySlot_BootloaderSystem0, pkg1::AesKeySlot_BootloaderSystem1, sector, SectorSize, sector_id);

                    /* Mark the sector as freshly loaded. */
                    m_sector_flags &= ~(1u << (sector_id % CacheEntries));

                    return ResultSuccess();
                }

                Result GetSector(u8 **out_sector, u32 sector_id) {
                    /* Try to find in the cache. */
                    for (size_t i = 0; i < CacheEntries; ++i) {
                        if (m_sector_ids[i] == sector_id) {
                            m_sector_flags &= ~(1u << i);
                            *out_sector = m_sector_cache + SectorSize * i;
                            return ResultSuccess();
                        }
                    }

                    /* Find a sector to evict. */
                    while ((m_sector_flags & (1u << m_next_idx)) == 0) {
                        m_sector_flags |= (1u << m_next_idx);
                        m_next_idx = (m_next_idx + 1) % CacheEntries;
                    }

                    /* Get the chosen sector. */
                    *out_sector = m_sector_cache + SectorSize * m_next_idx;
                    m_next_idx = (m_next_idx + 1) % CacheEntries;

                    /* Load the sector. */
                    return this->LoadSector(*out_sector, sector_id);
                }
            public:
                SystemPartitionStorage(s64 ofs, s64 size) : m_storage(*g_user_storage, ofs, size) {
                    /* Allocate sector cache. */
                    m_sector_cache = static_cast<u8 *>(AllocateAligned(CacheEntries * SectorSize, SectorSize));

                    /* Allocate sector ids. */
                    m_sector_ids   = static_cast<u32 *>(AllocateAligned(CacheEntries * sizeof(u32), alignof(u32)));
                    for (size_t i = 0; i < CacheEntries; ++i) {
                        m_sector_ids[i] = std::numeric_limits<u32>::max();
                    }

                    /* All sectors are dirty. */
                    m_sector_flags = ~0u;

                    /* Next sector is 0. */
                    m_next_idx = 0;
                }

                virtual Result Read(s64 offset, void *buffer, size_t size) override {
                    u32 sector_id = offset / SectorSize;
                    s64 subofs    = offset % SectorSize;

                    u8 *cur_dst = static_cast<u8 *>(buffer);
                    while (size > 0) {
                        /* Get the current sector. */
                        u8 *sector;
                        R_TRY(this->GetSector(std::addressof(sector), sector_id++));

                        /* Copy the data. */
                        const size_t cur_size = std::min<size_t>(SectorSize - subofs, size);
                        std::memcpy(cur_dst, sector + subofs, cur_size);

                        /* Advance. */
                        cur_dst += cur_size;
                        size -= cur_size;
                        subofs = 0;
                    }

                    return ResultSuccess();
                }

                virtual Result Flush() override {
                    return m_storage.Flush();
                }

                virtual Result GetSize(s64 *out) override {
                    return m_storage.GetSize(out);
                }

                virtual Result Write(s64 offset, const void *buffer, size_t size) override {
                    return fs::ResultUnsupportedOperation();
                }

                virtual Result SetSize(s64 size) override {
                    return fs::ResultUnsupportedOperation();
                }
        };

        constinit SystemPartitionStorage *g_system_storage = nullptr;
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

        constexpr const u16 SystemPartitionName[] = {
            'S', 'Y', 'S', 'T', 'E', 'M', 0
        };

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
                len += 6;

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

            const s64 offset = 0x200 * gpt->entries[i].starting_lba;
            const u64 size   = 0x200 * (gpt->entries[i].ending_lba + 1 - gpt->entries[i].starting_lba);

            if (std::memcmp(gpt->entries[i].partition_name, SystemPartitionName, sizeof(SystemPartitionName)) == 0) {
                g_system_storage = AllocateObject<SystemPartitionStorage>(offset, size);
            } else if (std::memcmp(gpt->entries[i].partition_name, Package2PartitionName, sizeof(Package2PartitionName)) == 0) {
                g_package2_storage = AllocateObject<fs::SubStorage>(*g_user_storage, offset, size);
            }
        }

        /* Check that we created system storage. */
        if (g_system_storage == nullptr) {
            ShowFatalError("Failed to initialize SYSTEM\n");
        }

        /* Check that we created package2 storage. */
        if (g_package2_storage == nullptr) {
            ShowFatalError("Failed to initialize Package2\n");
        }

        /* Mount system. */
        if (!fs::MountSystem()) {
            ShowFatalError("Failed to mount SYSTEM\n");
        }
    }

    Result ReadBoot0(s64 offset, void *dst, size_t size) {
        return g_boot0_storage->Read(offset, dst, size);
    }

    Result ReadPackage2(s64 offset, void *dst, size_t size) {
        return g_package2_storage->Read(offset, dst, size);
    }

    Result ReadSystem(s64 offset, void *dst, size_t size) {
        return g_system_storage->Read(offset, dst, size);
    }

}