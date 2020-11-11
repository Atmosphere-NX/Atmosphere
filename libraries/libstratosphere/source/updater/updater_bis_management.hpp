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

#pragma once
#include <stratosphere.hpp>

namespace ams::updater {

    class BisAccessor {
        NON_COPYABLE(BisAccessor);
        public:
            static constexpr size_t SectorAlignment = 0x200;
        private:
            std::unique_ptr<fs::IStorage> storage;
            const fs::BisPartitionId partition_id;
        public:
            explicit BisAccessor(fs::BisPartitionId id) : partition_id(id) { /* ... */ }

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
        using EnumType       = Boot0Partition;
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
        using EnumType       = Boot1Partition;
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
        using EnumType       = Package2Partition;
        using OffsetSizeType = OffsetSizeEntry<EnumType>;

        static constexpr size_t NumEntries = static_cast<size_t>(EnumType::Count);
        static constexpr OffsetSizeType Entries[NumEntries] = {
            {Package2Partition::BootConfig, 0x0000, 0x004000},
            {Package2Partition::Package2,   0x4000, 0x7FC000},
        };
    };

    template<typename Meta>
    class PartitionAccessor : public BisAccessor {
        NON_COPYABLE(PartitionAccessor);
        public:
            using EnumType       = typename Meta::EnumType;
            using OffsetSizeType = typename Meta::OffsetSizeType;
        public:
            explicit PartitionAccessor(fs::BisPartitionId id) : BisAccessor(id) { /* ... */ }
        private:
            constexpr const OffsetSizeType *FindEntry(EnumType which) {
                const OffsetSizeType *entry = nullptr;
                for (size_t i = 0; i < Meta::NumEntries; i++) {
                    if (Meta::Entries[i].which == which) {
                        entry = &Meta::Entries[i];
                        break;
                    }
                }

                AMS_ABORT_UNLESS(entry != nullptr);
                return entry;
            }
        public:
            Result Read(size_t *out_size, void *dst, size_t size, EnumType which) {
                const auto entry = FindEntry(which);
                AMS_ABORT_UNLESS(size >= entry->size);

                R_TRY(BisAccessor::Read(dst, entry->size, entry->offset));

                *out_size = entry->size;
                return ResultSuccess();
            }

            Result Write(const void *src, size_t size, EnumType which) {
                const auto entry = FindEntry(which);
                AMS_ABORT_UNLESS(size <= entry->size);
                AMS_ABORT_UNLESS((size % BisAccessor::SectorAlignment) == 0);
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

    static constexpr fs::BisPartitionId GetPackage2StorageId(Package2Type which) {
        switch (which) {
            case Package2Type::NormalMain:
                return fs::BisPartitionId::BootConfigAndPackage2Part1;
            case Package2Type::NormalSub:
                return fs::BisPartitionId::BootConfigAndPackage2Part2;
            case Package2Type::SafeMain:
                return fs::BisPartitionId::BootConfigAndPackage2Part3;
            case Package2Type::SafeSub:
                return fs::BisPartitionId::BootConfigAndPackage2Part4;
            case Package2Type::RepairMain:
                return fs::BisPartitionId::BootConfigAndPackage2Part5;
            case Package2Type::RepairSub:
                return fs::BisPartitionId::BootConfigAndPackage2Part6;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    class Boot0Accessor : public PartitionAccessor<Boot0Meta> {
        public:
            static constexpr fs::BisPartitionId PartitionId = fs::BisPartitionId::BootPartition1Root;
            static constexpr size_t BctPubkOffsetErista = 0x210;
            static constexpr size_t BctPubkOffsetMariko = 0x10;
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

            static size_t GetBctPubkOffset(BootImageUpdateType boot_image_update_type) {
                switch (boot_image_update_type) {
                    case BootImageUpdateType::Erista:
                        return BctPubkOffsetErista;
                    case BootImageUpdateType::Mariko:
                        return BctPubkOffsetMariko;
                    AMS_UNREACHABLE_DEFAULT_CASE();
                }
            }
        public:
            Result UpdateEks(void *dst_bct, void *eks_work_buffer);
            Result UpdateEksManually(void *dst_bct, const void *src_eks);
            Result PreserveAutoRcm(void *dst_bct, void *work_buffer, Boot0Partition which);

            Result DetectCustomPublicKey(bool *out, void *work_buffer, BootImageUpdateType boot_image_update_type);
    };

    class Boot1Accessor : public PartitionAccessor<Boot1Meta> {
        public:
            static constexpr fs::BisPartitionId PartitionId = fs::BisPartitionId::BootPartition2Root;
        public:
            Boot1Accessor() : PartitionAccessor<Boot1Meta>(PartitionId) { }
    };

    class Package2Accessor : public PartitionAccessor<Package2Meta> {
        public:
            Package2Accessor(Package2Type which) : PartitionAccessor<Package2Meta>(GetPackage2StorageId(which)) { }
    };

}
