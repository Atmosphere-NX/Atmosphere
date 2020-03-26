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
#include <stratosphere.hpp>

namespace ams::fssystem {

    template <typename Format>
    struct PartitionFileSystemMetaCore<Format>::PartitionFileSystemHeader {
        u32 signature;
        s32 entry_count;
        u32 name_table_size;
        u32 reserved;
    };

    template <typename Format>
    PartitionFileSystemMetaCore<Format>::~PartitionFileSystemMetaCore() {
        this->DeallocateBuffer();
    }

    template <typename Format>
    Result PartitionFileSystemMetaCore<Format>::Initialize(fs::IStorage *storage, MemoryResource *allocator) {
        /* Determine the meta data size. */
        R_TRY(this->QueryMetaDataSize(std::addressof(this->meta_data_size), storage));

        /* Deallocate any old meta buffer and allocate a new one. */
        this->DeallocateBuffer();
        this->allocator = allocator;
        this->buffer = reinterpret_cast<char *>(this->allocator->Allocate(this->meta_data_size));
        R_UNLESS(this->buffer != nullptr, fs::ResultAllocationFailureInPartitionFileSystemMetaCore());

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
        R_UNLESS(crypto::IsSameBytes(this->header, Format::VersionSignature, sizeof(Format::VersionSignature)), ResultSignatureVerificationFailed());

        /* Setup entries and name table. */
        const size_t entries_size = this->header->entry_count * sizeof(typename Format::PartitionEntry);
        this->entries = reinterpret_cast<PartitionEntry *>(reinterpret_cast<u8 *>(meta) + sizeof(PartitionFileSystemHeader));
        this->name_table = reinterpret_cast<char *>(meta) + sizeof(PartitionFileSystemHeader) + entries_size;

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
        if (this->initialized && index >= 0 && index < this->header->entry_count) {
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
        if (this->initialized) {
            for (s32 i = 0; i < this->header->entry_count; i++) {
                const auto &entry = this->entries[i];

                /* Name offset is invalid. */
                if (entry.name_offset >= this->header->name_table_size) {
                    return 0;
                }

                /* Compare to input name. */
                const s32 max_count = this->header->name_table_size - entry.name_offset;
                if (std::strncmp(std::addressof(this->name_table[entry.name_offset]), name, max_count) == 0) {
                    return i;
                }
            }

            /* Not found. */
            return -1;
        }

        return 0;
    }

    template <typename Format>
    const char *PartitionFileSystemMetaCore<Format>::GetEntryName(s32 index) const {
        if (this->initialized && index < this->header->entry_count) {
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
    Result PartitionFileSystemMetaCore<Format>::QueryMetaDataSize(size_t *out_size, fs::IStorage *storage) const {
        AMS_ABORT_UNLESS(allocator != nullptr);

        /* Read and validate the header. */
        PartitionFileSystemHeader header;
        R_TRY(storage->Read(0, std::addressof(header), sizeof(PartitionFileSystemHeader)));
        R_UNLESS(crypto::IsSameBytes(std::addressof(header), Format::VersionSignature, sizeof(Format::VersionSignature)), ResultSignatureVerificationFailed());

        /* Output size. */
        *out_size = sizeof(PartitionFileSystemHeader) + header.entry_count * sizeof(typename Format::PartitionEntry) + header.name_table_size;
        return ResultSuccess();
    }

    template class PartitionFileSystemMetaCore<impl::PartitionFileSystemFormat>;

}
