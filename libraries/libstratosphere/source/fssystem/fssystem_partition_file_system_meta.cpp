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
#include <stratosphere.hpp>

namespace ams::fssystem {

    template <typename Format>
    struct PartitionFileSystemMetaCore<Format>::PartitionFileSystemHeader {
        char signature[sizeof(Format::VersionSignature)];
        s32 entry_count;
        u32 name_table_size;
        u32 reserved;
    };
    static_assert(util::is_pod<PartitionFileSystemMeta::PartitionFileSystemHeader>::value);
    static_assert(sizeof(PartitionFileSystemMeta::PartitionFileSystemHeader) == 0x10);

    template <typename Format>
    PartitionFileSystemMetaCore<Format>::~PartitionFileSystemMetaCore() {
        this->DeallocateBuffer();
    }

    template <typename Format>
    Result PartitionFileSystemMetaCore<Format>::Initialize(fs::IStorage *storage, MemoryResource *allocator) {
        /* Validate preconditions. */
        AMS_ASSERT(allocator != nullptr);

        /* Determine the meta data size. */
        R_TRY(this->QueryMetaDataSize(std::addressof(this->meta_data_size), storage));

        /* Deallocate any old meta buffer and allocate a new one. */
        this->DeallocateBuffer();
        this->allocator = allocator;
        this->buffer = static_cast<char *>(this->allocator->Allocate(this->meta_data_size));
        R_UNLESS(this->buffer != nullptr, fs::ResultAllocationFailureInPartitionFileSystemMetaA());

        /* Perform regular initialization. */
        return this->Initialize(storage, this->buffer, this->meta_data_size);
    }

    template <typename Format>
    Result PartitionFileSystemMetaCore<Format>::Initialize(fs::IStorage *storage, void *meta, size_t meta_size) {
        /* Validate size for header. */
        R_UNLESS(meta_size >= sizeof(PartitionFileSystemHeader), fs::ResultInvalidSize());

        /* Read the header. */
        R_TRY(storage->Read(0, meta, sizeof(PartitionFileSystemHeader)));

        /* Set and validate the header. */
        this->header = reinterpret_cast<PartitionFileSystemHeader *>(meta);
        R_UNLESS(crypto::IsSameBytes(this->header->signature, Format::VersionSignature, sizeof(Format::VersionSignature)), typename Format::ResultSignatureVerificationFailed());

        /* Setup entries and name table. */
        const size_t entries_size = this->header->entry_count * sizeof(typename Format::PartitionEntry);
        this->entries = reinterpret_cast<PartitionEntry *>(static_cast<u8 *>(meta) + sizeof(PartitionFileSystemHeader));
        this->name_table = static_cast<char *>(meta) + sizeof(PartitionFileSystemHeader) + entries_size;

        /* Validate size for header + entries + name table. */
        R_UNLESS(meta_size >= sizeof(PartitionFileSystemHeader) + entries_size + this->header->name_table_size, fs::ResultInvalidSize());

        /* Read entries and name table. */
        R_TRY(storage->Read(sizeof(PartitionFileSystemHeader), this->entries, entries_size + this->header->name_table_size));

        /* Mark as initialized. */
        this->initialized = true;
        return ResultSuccess();
    }

    template <typename Format>
    void PartitionFileSystemMetaCore<Format>::DeallocateBuffer() {
        if (this->buffer != nullptr) {
            AMS_ABORT_UNLESS(this->allocator != nullptr);
            this->allocator->Deallocate(this->buffer, this->meta_data_size);
            this->buffer = nullptr;
        }
    }

    template <typename Format>
    const typename Format::PartitionEntry *PartitionFileSystemMetaCore<Format>::GetEntry(s32 index) const {
        if (this->initialized && 0 <= index && index < static_cast<s32>(this->header->entry_count)) {
            return std::addressof(this->entries[index]);
        }
        return nullptr;
    }

    template <typename Format>
    s32 PartitionFileSystemMetaCore<Format>::GetEntryCount() const {
        if (this->initialized) {
            return this->header->entry_count;
        }
        return 0;
    }

    template <typename Format>
    s32 PartitionFileSystemMetaCore<Format>::GetEntryIndex(const char *name) const {
        /* Fail if not initialized. */
        if (!this->initialized) {
            return 0;
        }

        for (s32 i = 0; i < static_cast<s32>(this->header->entry_count); i++) {
            const auto &entry = this->entries[i];

            /* Name offset is invalid. */
            if (entry.name_offset >= this->header->name_table_size) {
                return 0;
            }

            /* Compare to input name. */
            const s32 max_name_len = this->header->name_table_size - entry.name_offset;
            if (std::strncmp(std::addressof(this->name_table[entry.name_offset]), name, max_name_len) == 0) {
                return i;
            }
        }

        /* Not found. */
        return -1;
    }

    template <typename Format>
    const char *PartitionFileSystemMetaCore<Format>::GetEntryName(s32 index) const {
        if (this->initialized && index < static_cast<s32>(this->header->entry_count)) {
            return std::addressof(this->name_table[this->GetEntry(index)->name_offset]);
        }
        return nullptr;
    }

    template <typename Format>
    size_t PartitionFileSystemMetaCore<Format>::GetHeaderSize() const {
        return sizeof(PartitionFileSystemHeader);
    }

    template <typename Format>
    size_t PartitionFileSystemMetaCore<Format>::GetMetaDataSize() const {
        return this->meta_data_size;
    }

    template <typename Format>
    Result PartitionFileSystemMetaCore<Format>::QueryMetaDataSize(size_t *out_size, fs::IStorage *storage) {
        /* Read and validate the header. */
        PartitionFileSystemHeader header;
        R_TRY(storage->Read(0, std::addressof(header), sizeof(PartitionFileSystemHeader)));
        R_UNLESS(crypto::IsSameBytes(std::addressof(header), Format::VersionSignature, sizeof(Format::VersionSignature)), typename Format::ResultSignatureVerificationFailed());

        /* Output size. */
        *out_size = sizeof(PartitionFileSystemHeader) + header.entry_count * sizeof(typename Format::PartitionEntry) + header.name_table_size;
        return ResultSuccess();
    }

    template class PartitionFileSystemMetaCore<impl::PartitionFileSystemFormat>;
    template class PartitionFileSystemMetaCore<impl::Sha256PartitionFileSystemFormat>;

    Result Sha256PartitionFileSystemMeta::Initialize(fs::IStorage *base_storage, MemoryResource *allocator, const void *hash, size_t hash_size, std::optional<u8> suffix) {
        /* Ensure preconditions. */
        R_UNLESS(hash_size == crypto::Sha256Generator::HashSize, fs::ResultPreconditionViolation());

        /* Get metadata size. */
        R_TRY(QueryMetaDataSize(std::addressof(this->meta_data_size), base_storage));

        /* Ensure we have no buffer. */
        this->DeallocateBuffer();

        /* Set allocator and allocate buffer. */
        this->allocator = allocator;
        this->buffer = static_cast<char *>(this->allocator->Allocate(this->meta_data_size));
        R_UNLESS(this->buffer != nullptr, fs::ResultAllocationFailureInPartitionFileSystemMetaB());

        /* Read metadata. */
        R_TRY(base_storage->Read(0, this->buffer, this->meta_data_size));

        /* Calculate hash. */
        char calc_hash[crypto::Sha256Generator::HashSize];
        {
            crypto::Sha256Generator generator;
            generator.Initialize();
            generator.Update(this->buffer, this->meta_data_size);
            if (suffix) {
                u8 suffix_val = *suffix;
                generator.Update(std::addressof(suffix_val), 1);
            }
            generator.GetHash(calc_hash, sizeof(calc_hash));
        }

        /* Ensure hash is valid. */
        R_UNLESS(crypto::IsSameBytes(hash, calc_hash, sizeof(calc_hash)), fs::ResultSha256PartitionHashVerificationFailed());

        /* Give access to Format */
        using Format = impl::Sha256PartitionFileSystemFormat;

        /* Set header. */
        this->header = reinterpret_cast<PartitionFileSystemHeader *>(this->buffer);
        R_UNLESS(crypto::IsSameBytes(this->header->signature, Format::VersionSignature, sizeof(Format::VersionSignature)), typename Format::ResultSignatureVerificationFailed());

        /* Validate size for entries and name table. */
        const size_t entries_size = this->header->entry_count * sizeof(typename Format::PartitionEntry);
        R_UNLESS(this->meta_data_size >= sizeof(PartitionFileSystemHeader) + entries_size + this->header->name_table_size, fs::ResultInvalidSha256PartitionMetaDataSize());

        /* Set entries and name table. */
        this->entries = reinterpret_cast<PartitionEntry *>(this->buffer + sizeof(PartitionFileSystemHeader));
        this->name_table = this->buffer + sizeof(PartitionFileSystemHeader) + entries_size;

        /* We initialized. */
        this->initialized = true;
        return ResultSuccess();
    }

}
