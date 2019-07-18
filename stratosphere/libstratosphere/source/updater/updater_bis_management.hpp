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

#pragma once
#include <switch.h>
#include <stratosphere.hpp>
#include <stratosphere/updater/updater_types.hpp>

namespace sts::updater {

    class BisAccessor {
        public:
            static constexpr size_t SectorAlignment = 0x200;
        private:
            FsStorage storage = {};
            FsBisStorageId partition_id;
            bool active;
        public:
            BisAccessor(FsBisStorageId id) : partition_id(id), active(false) { }
            ~BisAccessor() {
                if (this->active) {
                    fsStorageClose(&storage);
                }
            }

        public:
            Result Initialize();
            void Finalize();
        protected:
            Result Read(void *dst, size_t size, u64 offset);
            Result Write(u64 offset, const void *src, size_t size);
            Result Write(u64 offset, size_t size, const char *bip_path, void *work_buffer, size_t work_buffer_size);
            Result Clear(u64 offset, u64 size, void *work_buffer, size_t work_buffer_size);
            Result GetHash(void *dst, u64 offset, u64 size, u64 hash_size, void *work_buffer, size_t work_buffer_size);
    };

    template<typename EnumType>
    struct OffsetSizeEntry {
        EnumType which;
        u64 offset;
        size_t size;
    };

    enum class Boot0Partition {
        BctNormalMain,
        BctSafeMain,
        BctNormalSub,
        BctSafeSub,
        BctSave,
        Package1NormalMain,
        Package1NormalSub,
        Eks,
        Count,
    };

    enum class Boot1Partition {
        Package1SafeMain,
        Package1SafeSub,
        Package1RepairMain,
        Package1RepairSub,
        Count,
    };

    enum class Package2Partition {
        BootConfig,
        Package2,
        Count,
    };

    struct Boot0Meta {
        using EnumType = Boot0Partition;
        using OffsetSizeType = OffsetSizeEntry<EnumType>;

        static constexpr size_t NumEntries = static_cast<size_t>(EnumType::Count);
        static constexpr OffsetSizeType Entries[NumEntries] = {
            {Boot0Partition::BctNormalMain,   0 * BctSize, BctSize},
            {Boot0Partition::BctSafeMain,     1 * BctSize, BctSize},
            {Boot0Partition::BctNormalSub,    2 * BctSize, BctSize},
            {Boot0Partition::BctSafeSub,      3 * BctSize, BctSize},
            {Boot0Partition::BctSave,        63 * BctSize, BctSize},
            {Boot0Partition::Package1NormalMain, 0x100000, 0x40000},
            {Boot0Partition::Package1NormalSub,  0x140000, 0x40000},
            {Boot0Partition::Eks,                0x180000, EksSize},
        };
    };

    struct Boot1Meta {
        using EnumType = Boot1Partition;
        using OffsetSizeType = OffsetSizeEntry<EnumType>;

        static constexpr size_t NumEntries = static_cast<size_t>(EnumType::Count);
        static constexpr OffsetSizeType Entries[NumEntries] = {
            {Boot1Partition::Package1SafeMain,   0x00000, 0x40000},
            {Boot1Partition::Package1SafeSub,    0x40000, 0x40000},
            {Boot1Partition::Package1RepairMain, 0x80000, 0x40000},
            {Boot1Partition::Package1RepairSub,  0xC0000, 0x40000},
        };
    };

    struct Package2Meta {
        using EnumType = Package2Partition;
        using OffsetSizeType = OffsetSizeEntry<EnumType>;

        static constexpr size_t NumEntries = static_cast<size_t>(EnumType::Count);
        static constexpr OffsetSizeType Entries[NumEntries] = {
            {Package2Partition::BootConfig, 0x0000, 0x004000},
            {Package2Partition::Package2,   0x4000, 0x7FC000},
        };
    };

    template<typename Meta>
    class PartitionAccessor : public BisAccessor {
        public:
            using EnumType = typename Meta::EnumType;
            using OffsetSizeType = typename Meta::OffsetSizeType;
        public:
            PartitionAccessor(FsBisStorageId id) : BisAccessor(id) { }
        private:
            constexpr const OffsetSizeType *FindEntry(EnumType which) {
                for (size_t i = 0; i < Meta::NumEntries; i++) {
                    if (Meta::Entries[i].which == which) {
                        return &Meta::Entries[i];
                    }
                }
                std::abort();
            }
        public:
            Result Read(size_t *out_size, void *dst, size_t size, EnumType which) {
                const auto entry = FindEntry(which);
                if (size < entry->size) {
                    std::abort();
                }

                R_TRY(BisAccessor::Read(dst, entry->size, entry->offset));

                *out_size = entry->size;
                return ResultSuccess;
            }

            Result Write(const void *src, size_t size, EnumType which) {
                const auto entry = FindEntry(which);
                if (size > entry->size || size % BisAccessor::SectorAlignment != 0) {
                    std::abort();
                }

                return BisAccessor::Write(entry->offset, src, size);
            }

            Result Write(const char *bip_path, void *work_buffer, size_t work_buffer_size, EnumType which) {
                const auto entry = FindEntry(which);
                return BisAccessor::Write(entry->offset, entry->size, bip_path, work_buffer, work_buffer_size);
            }

            Result Clear(void *work_buffer, size_t work_buffer_size, EnumType which) {
                const auto entry = FindEntry(which);
                return BisAccessor::Clear(entry->offset, entry->size, work_buffer, work_buffer_size);
            }

            Result GetHash(void *dst, u64 hash_size, void *work_buffer, size_t work_buffer_size, EnumType which) {
                const auto entry = FindEntry(which);
                return BisAccessor::GetHash(dst, entry->offset, entry->size, hash_size, work_buffer, work_buffer_size);
            }
    };

    enum class Package2Type {
        NormalMain,
        NormalSub,
        SafeMain,
        SafeSub,
        RepairMain,
        RepairSub,
    };

    static constexpr FsBisStorageId GetPackage2StorageId(Package2Type which) {
        switch (which) {
            case Package2Type::NormalMain:
                return FsBisStorageId_BootConfigAndPackage2NormalMain;
            case Package2Type::NormalSub:
                return FsBisStorageId_BootConfigAndPackage2NormalSub;
            case Package2Type::SafeMain:
                return FsBisStorageId_BootConfigAndPackage2SafeMain;
            case Package2Type::SafeSub:
                return FsBisStorageId_BootConfigAndPackage2SafeSub;
            case Package2Type::RepairMain:
                return FsBisStorageId_BootConfigAndPackage2RepairMain;
            case Package2Type::RepairSub:
                return FsBisStorageId_BootConfigAndPackage2RepairSub;
            default:
                std::abort();
        }
    }

    class Boot0Accessor : public PartitionAccessor<Boot0Meta> {
        public:
            static constexpr FsBisStorageId PartitionId = FsBisStorageId_Boot0;
            static constexpr size_t BctPubkOffset = 0x210;
            static constexpr size_t BctPubkSize = 0x100;
            static constexpr size_t BctEksOffset = 0x450;
            static constexpr size_t BctVersionOffset = 0x2330;
            static constexpr size_t BctVersionMax = 0x20;
        public:
            Boot0Accessor() : PartitionAccessor<Boot0Meta>(PartitionId) { }
        private:
            static size_t GetBootloaderVersion(void *bct);
            static size_t GetEksIndex(size_t bootloader_version);
            static void CopyEks(void *dst_bct, const void *src_eks, size_t eks_index);
        public:
            Result UpdateEks(void *dst_bct, void *eks_work_buffer);
            Result UpdateEksManually(void *dst_bct, const void *src_eks);
            Result PreserveAutoRcm(void *dst_bct, void *work_buffer, Boot0Partition which);
    };

    class Boot1Accessor : public PartitionAccessor<Boot1Meta> {
        public:
            static constexpr FsBisStorageId PartitionId = FsBisStorageId_Boot1;
        public:
            Boot1Accessor() : PartitionAccessor<Boot1Meta>(PartitionId) { }
    };

    class Package2Accessor : public PartitionAccessor<Package2Meta> {
        public:
            Package2Accessor(Package2Type which) : PartitionAccessor<Package2Meta>(GetPackage2StorageId(which)) { }
    };

}
