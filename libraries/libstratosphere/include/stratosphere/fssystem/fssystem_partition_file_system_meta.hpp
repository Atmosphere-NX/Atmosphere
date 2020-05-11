/*
 * Copyright (c) 2018-2020 Adubbz, Atmosphère-NX
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
#include <stratosphere/fs/fs_common.hpp>
#include <stratosphere/fs/impl/fs_newable.hpp>

namespace ams::fssystem {

    namespace impl {

        struct PartitionFileSystemFormat {
            #pragma pack(push, 1)
            struct PartitionEntry {
                u64 offset;
                u64 size;
                u32 name_offset;
                u32 reserved;
            };
            static_assert(util::is_pod<PartitionEntry>::value);
            #pragma pack(pop)

            static constexpr const char VersionSignature[] = { 'P', 'F', 'S', '0' };

            static constexpr size_t EntryNameLengthMax = ::ams::fs::EntryNameLengthMax;
            static constexpr size_t FileDataAlignmentSize = 0x20;

            using ResultSignatureVerificationFailed = fs::ResultPartitionSignatureVerificationFailed;
        };

        struct Sha256PartitionFileSystemFormat {
            static constexpr size_t HashSize = ::ams::crypto::Sha256Generator::HashSize;

            #pragma pack(push, 1)
            struct PartitionEntry {
                u64 offset;
                u64 size;
                u32 name_offset;
                u32 hash_target_size;
                u64 hash_target_offset;
                char hash[HashSize];
            };
            static_assert(util::is_pod<PartitionEntry>::value);
            #pragma pack(pop)

            static constexpr const char VersionSignature[] = { 'H', 'F', 'S', '0' };

            static constexpr size_t EntryNameLengthMax = ::ams::fs::EntryNameLengthMax;
            static constexpr size_t FileDataAlignmentSize = 0x200;

            using ResultSignatureVerificationFailed = fs::ResultSha256PartitionSignatureVerificationFailed;
        };

    }

    template<typename Format>
    class PartitionFileSystemMetaCore : public fs::impl::Newable {
        public:
            static constexpr size_t EntryNameLengthMax    = Format::EntryNameLengthMax;
            static constexpr size_t FileDataAlignmentSize = Format::FileDataAlignmentSize;

            /* Forward declare header. */
            struct PartitionFileSystemHeader;

            using PartitionEntry = typename Format::PartitionEntry;
        protected:
            bool initialized;
            PartitionFileSystemHeader *header;
            PartitionEntry *entries;
            char *name_table;
            size_t meta_data_size;
            MemoryResource *allocator;
            char *buffer;
        public:
            PartitionFileSystemMetaCore() : initialized(false), allocator(nullptr), buffer(nullptr) { /* ... */ }
            ~PartitionFileSystemMetaCore();

            Result Initialize(fs::IStorage *storage, MemoryResource *allocator);
            Result Initialize(fs::IStorage *storage, void *header, size_t header_size);

            const PartitionEntry *GetEntry(s32 index) const;
            s32 GetEntryCount() const;
            s32 GetEntryIndex(const char *name) const;
            const char *GetEntryName(s32 index) const;
            size_t GetHeaderSize() const;
            size_t GetMetaDataSize() const;
        public:
            static Result QueryMetaDataSize(size_t *out_size, fs::IStorage *storage);
        protected:
            void DeallocateBuffer();
    };

    using PartitionFileSystemMeta = PartitionFileSystemMetaCore<impl::PartitionFileSystemFormat>;

    class Sha256PartitionFileSystemMeta : public PartitionFileSystemMetaCore<impl::Sha256PartitionFileSystemFormat> {
        public:
            using PartitionFileSystemMetaCore<impl::Sha256PartitionFileSystemFormat>::Initialize;
            Result Initialize(fs::IStorage *base_storage, MemoryResource *allocator, const void *hash, size_t hash_size, std::optional<u8> suffix = std::nullopt);
    };

}
