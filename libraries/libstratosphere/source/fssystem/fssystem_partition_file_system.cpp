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

    namespace {

        class PartitionFileSystemDefaultAllocator : public MemoryResource {
            private:
                virtual void *AllocateImpl(size_t size, size_t alignment) override {
                    return ::ams::fs::impl::Allocate(size);
                }

                virtual void DeallocateImpl(void *buffer, size_t size, size_t alignment) override {
                    ::ams::fs::impl::Deallocate(buffer, size);
                }

                virtual bool IsEqualImpl(const MemoryResource &rhs) const override {
                    return this == std::addressof(rhs);
                }
        };

        PartitionFileSystemDefaultAllocator g_partition_filesystem_default_allocator;

    }

    template <typename MetaType>
    class PartitionFileSystemCore<MetaType>::PartitionFile : public fs::fsa::IFile, public fs::impl::Newable {
        private:
            const typename MetaType::PartitionEntry *partition_entry;
            const PartitionFileSystemCore<MetaType> *parent;
            const fs::OpenMode mode;
        public:
            PartitionFile(PartitionFileSystemCore<MetaType> *parent, const typename MetaType::PartitionEntry *partition_entry, fs::OpenMode mode) : partition_entry(partition_entry), parent(parent), mode(mode) { /* ... */ }
        private:
            virtual Result DoRead(size_t *out, s64 offset, void *buffer, size_t size, const fs::ReadOption &option) override final;

            virtual Result DoGetSize(s64 *out) override final {
                *out = this->partition_entry->size;
                return ResultSuccess();
            }

            virtual Result DoFlush() override final {
                /* Nothing to do if writing disallowed. */
                R_SUCCEED_IF((this->mode & fs::OpenMode_Write) == 0);

                /* Flush base storage. */
                return this->parent->base_storage->Flush();
            }

            virtual Result DoWrite(s64 offset, const void *buffer, size_t size, const fs::WriteOption &option) override final {
                /* Ensure appending is not required. */
                bool needs_append;
                R_TRY(this->DryWrite(std::addressof(needs_append), offset, size, option, this->mode));
                R_UNLESS(!needs_append, fs::ResultUnsupportedOperationInPartitionFileA());

                /* Appending is prohibited. */
                AMS_ASSERT((this->mode & fs::OpenMode_AllowAppend) == 0);

                /* Validate offset and size. */
                R_UNLESS(offset                          <= static_cast<s64>(this->partition_entry->size), fs::ResultOutOfRange());
                R_UNLESS(static_cast<s64>(offset + size) <= static_cast<s64>(this->partition_entry->size), fs::ResultInvalidSize());

                /* Write to the base storage. */
                return this->parent->base_storage->Write(this->parent->meta_data_size + this->partition_entry->offset + offset, buffer, size);
            }

            virtual Result DoSetSize(s64 size) override final {
                R_TRY(this->DrySetSize(size, this->mode));
                return fs::ResultUnsupportedOperationInPartitionFileA();
            }

            virtual Result DoOperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override final {
                /* Validate preconditions for operation. */
                switch (op_id) {
                    case fs::OperationId::InvalidateCache:
                        R_UNLESS((this->mode & fs::OpenMode_Read)  != 0, fs::ResultReadNotPermitted());
                        R_UNLESS((this->mode & fs::OpenMode_Write) == 0, fs::ResultUnsupportedOperationInPartitionFileB());
                        break;
                    case fs::OperationId::QueryRange:
                        break;
                    default:
                        return fs::ResultUnsupportedOperationInPartitionFileB();
                }

                /* Validate offset and size. */
                R_UNLESS(offset                          >= 0,                                             fs::ResultOutOfRange());
                R_UNLESS(offset                          <= static_cast<s64>(this->partition_entry->size), fs::ResultOutOfRange());
                R_UNLESS(static_cast<s64>(offset + size) <= static_cast<s64>(this->partition_entry->size), fs::ResultInvalidSize());
                R_UNLESS(static_cast<s64>(offset + size) >= offset,                                        fs::ResultInvalidSize());

                return this->parent->base_storage->OperateRange(dst, dst_size, op_id, this->parent->meta_data_size + this->partition_entry->offset + offset, size, src, src_size);
            }
        public:
            virtual sf::cmif::DomainObjectId GetDomainObjectId() const override {
                /* TODO: How should this be handled? */
                return sf::cmif::InvalidDomainObjectId;
            }
    };

    template<>
    Result PartitionFileSystemCore<PartitionFileSystemMeta>::PartitionFile::DoRead(size_t *out, s64 offset, void *dst, size_t dst_size, const fs::ReadOption &option) {
        /* Perform a dry read. */
        size_t read_size = 0;
        R_TRY(this->DryRead(std::addressof(read_size), offset, dst_size, option, this->mode));

        /* Read from the base storage. */
        R_TRY(this->parent->base_storage->Read(this->parent->meta_data_size + this->partition_entry->offset + offset, dst, read_size));

        /* Set output size. */
        *out = read_size;
        return ResultSuccess();
    }

    template<>
    Result PartitionFileSystemCore<Sha256PartitionFileSystemMeta>::PartitionFile::DoRead(size_t *out, s64 offset, void *dst, size_t dst_size, const fs::ReadOption &option) {
        /* Perform a dry read. */
        size_t read_size = 0;
        R_TRY(this->DryRead(std::addressof(read_size), offset, dst_size, option, this->mode));

        const s64 entry_start = this->parent->meta_data_size + this->partition_entry->offset;
        const s64 read_end    = static_cast<s64>(offset + read_size);
        const s64 hash_start  = static_cast<s64>(this->partition_entry->hash_target_offset);
        const s64 hash_end    = hash_start + this->partition_entry->hash_target_size;

        if (read_end <= hash_start || hash_end <= offset) {
            /* We aren't reading hashed data, so we can just read from the base storage. */
            R_TRY(this->parent->base_storage->Read(entry_start + offset, dst, read_size));
        } else {
            /* Only hash target offset == 0 is supported. */
            R_UNLESS(hash_start == 0, fs::ResultInvalidSha256PartitionHashTarget());

            /* Ensure that the hash region is valid. */
            R_UNLESS(this->partition_entry->hash_target_offset + this->partition_entry->hash_target_size <= this->partition_entry->size, fs::ResultInvalidSha256PartitionHashTarget());

            /* Validate our read offset. */
            const s64 read_offset = entry_start + offset;
            R_UNLESS(read_offset >= offset, fs::ResultOutOfRange());

            /* Prepare a buffer for our calculated hash. */
            char hash[crypto::Sha256Generator::HashSize];
            crypto::Sha256Generator generator;

            /* Ensure we can perform our read. */
            const bool hash_in_read = offset <= hash_start && hash_end <= read_end;
            const bool read_in_hash = hash_start <= offset && read_end <= hash_end;
            R_UNLESS(hash_in_read || read_in_hash, fs::ResultInvalidSha256PartitionHashTarget());

            /* Initialize the generator. */
            generator.Initialize();

            if (hash_in_read) {
                /* Easy case: hash region is contained within the bounds. */
                R_TRY(this->parent->base_storage->Read(entry_start + offset, dst, read_size));
                generator.Update(static_cast<u8 *>(dst) + hash_start - offset, this->partition_entry->hash_target_size);
            } else /* if (read_in_hash) */ {
                /* We're reading a portion of what's hashed. */
                s64 remaining_hash_size = this->partition_entry->hash_target_size;
                s64 hash_offset         = entry_start + hash_start;
                s64 remaining_size      = read_size;
                s64 copy_offset         = 0;
                while (remaining_hash_size > 0) {
                    /* Read some portion of data into the buffer. */
                    constexpr size_t HashBufferSize = 0x200;
                    char hash_buffer[HashBufferSize];
                    size_t cur_size = static_cast<size_t>(std::min(static_cast<s64>(HashBufferSize), remaining_hash_size));
                    R_TRY(this->parent->base_storage->Read(hash_offset, hash_buffer, cur_size));

                    /*  Update the hash. */
                    generator.Update(hash_buffer, cur_size);

                    /* If we need to copy, do so. */
                    if (read_offset <= (hash_offset + static_cast<s64>(cur_size)) && remaining_size > 0) {
                        const s64 hash_buffer_offset = std::max<s64>(read_offset - hash_offset, 0);
                        const size_t copy_size = static_cast<size_t>(std::min<s64>(cur_size - hash_buffer_offset, remaining_size));
                        std::memcpy(static_cast<u8 *>(dst) + copy_offset, hash_buffer + hash_buffer_offset, copy_size);
                        remaining_size -= copy_size;
                        copy_offset    += copy_size;
                    }

                    /* Update offsets. */
                    remaining_hash_size -= cur_size;
                    hash_offset         += cur_size;
                }
            }

            /* Get the hash. */
            generator.GetHash(hash, sizeof(hash));

            /* Validate the hash. */
            auto hash_guard = SCOPE_GUARD { std::memset(dst, 0, read_size); };
            R_UNLESS(crypto::IsSameBytes(this->partition_entry->hash, hash, sizeof(hash)), fs::ResultSha256PartitionHashVerificationFailed());

            /* We successfully completed our read. */
            hash_guard.Cancel();
        }

        /* Set output size. */
        *out = read_size;
        return ResultSuccess();
    }

    template <typename MetaType>
    class PartitionFileSystemCore<MetaType>::PartitionDirectory : public fs::fsa::IDirectory, public fs::impl::Newable {
        private:
            u32 cur_index;
            const PartitionFileSystemCore<MetaType> *parent;
            const fs::OpenDirectoryMode mode;
        public:
            PartitionDirectory(PartitionFileSystemCore<MetaType> *parent, fs::OpenDirectoryMode mode) : cur_index(0), parent(parent), mode(mode) { /* ... */ }
        public:
            virtual Result DoRead(s64 *out_count, fs::DirectoryEntry *out_entries, s64 max_entries) override final {
                /* There are no subdirectories. */
                if ((this->mode & fs::OpenDirectoryMode_File) == 0) {
                    *out_count = 0;
                    return ResultSuccess();
                }

                /* Calculate number of entries. */
                const s64 entry_count = std::min(max_entries, static_cast<s64>(this->parent->meta_data->GetEntryCount() - this->cur_index));

                /* Populate output directory entries. */
                for (auto i = 0; i < entry_count; i++, this->cur_index++) {
                    fs::DirectoryEntry &dir_entry = out_entries[i];

                    /* Setup the output directory entry. */
                    dir_entry.type = fs::DirectoryEntryType_File;
                    dir_entry.file_size = this->parent->meta_data->GetEntry(this->cur_index)->size;
                    std::strncpy(dir_entry.name, this->parent->meta_data->GetEntryName(this->cur_index), sizeof(dir_entry.name) - 1);
                    dir_entry.name[sizeof(dir_entry.name) - 1] = fs::StringTraits::NullTerminator;
                }

                *out_count = entry_count;
                return ResultSuccess();
            }

            virtual Result DoGetEntryCount(s64 *out) override final {
                /* Output the parent meta data entry count for files, otherwise 0. */
                if (this->mode & fs::OpenDirectoryMode_File) {
                    *out = this->parent->meta_data->GetEntryCount();
                } else {
                    *out = 0;
                }

                return ResultSuccess();
            }

            virtual sf::cmif::DomainObjectId GetDomainObjectId() const override {
                /* TODO: How should this be handled? */
                return sf::cmif::InvalidDomainObjectId;
            }
    };

    template <typename MetaType>
    PartitionFileSystemCore<MetaType>::PartitionFileSystemCore() : initialized(false) {
        /* ... */
    }

    template <typename MetaType>
    PartitionFileSystemCore<MetaType>::~PartitionFileSystemCore() {
        /* ... */
    }

    template <typename MetaType>
    Result PartitionFileSystemCore<MetaType>::Initialize(fs::IStorage *base_storage, MemoryResource *allocator) {
        /* Validate preconditions. */
        R_UNLESS(!this->initialized, fs::ResultPreconditionViolation());

        /* Allocate meta data. */
        this->unique_meta_data = std::make_unique<MetaType>();
        R_UNLESS(this->unique_meta_data != nullptr, fs::ResultAllocationFailureInPartitionFileSystemA());

        /* Initialize meta data. */
        R_TRY(this->unique_meta_data->Initialize(base_storage, allocator));

        /* Initialize members. */
        this->meta_data = this->unique_meta_data.get();
        this->base_storage = base_storage;
        this->meta_data_size = this->meta_data->GetMetaDataSize();
        this->initialized = true;
        return ResultSuccess();
    }

    template <typename MetaType>
    Result PartitionFileSystemCore<MetaType>::Initialize(std::unique_ptr<MetaType> &&meta_data, std::shared_ptr<fs::IStorage> base_storage) {
        this->unique_meta_data = std::move(meta_data);
        return this->Initialize(this->unique_meta_data.get(), base_storage);
    }

    template <typename MetaType>
    Result PartitionFileSystemCore<MetaType>::Initialize(MetaType *meta_data, std::shared_ptr<fs::IStorage> base_storage) {
        /* Validate preconditions. */
        R_UNLESS(!this->initialized, fs::ResultPreconditionViolation());

        /* Initialize members. */
        this->shared_storage = std::move(base_storage);
        this->base_storage = this->shared_storage.get();
        this->meta_data = meta_data;
        this->meta_data_size = this->meta_data->GetMetaDataSize();
        this->initialized = true;
        return ResultSuccess();
    }

    template <typename MetaType>
    Result PartitionFileSystemCore<MetaType>::Initialize(fs::IStorage *base_storage) {
        return this->Initialize(base_storage, std::addressof(g_partition_filesystem_default_allocator));
    }

    template <typename MetaType>
    Result PartitionFileSystemCore<MetaType>::Initialize(std::shared_ptr<fs::IStorage> base_storage) {
        this->shared_storage = std::move(base_storage);
        return this->Initialize(this->shared_storage.get());
    }

    template <typename MetaType>
    Result PartitionFileSystemCore<MetaType>::Initialize(std::shared_ptr<fs::IStorage> base_storage, MemoryResource *allocator) {
        this->shared_storage = std::move(base_storage);
        return this->Initialize(this->shared_storage.get(), allocator);
    }

    template <typename MetaType>
    Result PartitionFileSystemCore<MetaType>::GetFileBaseOffset(s64 *out_offset, const char *path) {
        /* Validate preconditions. */
        R_UNLESS(this->initialized, fs::ResultPreconditionViolation());

        /* Obtain and validate the entry index. */
        const s32 entry_index = this->meta_data->GetEntryIndex(path + 1);
        R_UNLESS(entry_index >= 0, fs::ResultPathNotFound());

        /* Output offset. */
        *out_offset = this->meta_data_size + this->meta_data->GetEntry(entry_index)->offset;
        return ResultSuccess();
    }

    template <typename MetaType>
    Result PartitionFileSystemCore<MetaType>::DoGetEntryType(fs::DirectoryEntryType *out, const char *path) {
        /* Validate preconditions. */
        R_UNLESS(this->initialized,                        fs::ResultPreconditionViolation());
        R_UNLESS(fs::PathNormalizer::IsSeparator(path[0]), fs::ResultInvalidPathFormat());

        /* Check if the path is for a directory. */
        if (std::strncmp(path, fs::PathNormalizer::RootPath, sizeof(fs::PathNormalizer::RootPath)) == 0) {
            *out = fs::DirectoryEntryType_Directory;
            return ResultSuccess();
        }

        /* Ensure that path is for a file. */
        R_UNLESS(this->meta_data->GetEntryIndex(path + 1) >= 0, fs::ResultPathNotFound());

        *out = fs::DirectoryEntryType_File;
        return ResultSuccess();
    }

    template <typename MetaType>
    Result PartitionFileSystemCore<MetaType>::DoOpenFile(std::unique_ptr<fs::fsa::IFile> *out_file, const char *path, fs::OpenMode mode) {
        /* Validate preconditions. */
        R_UNLESS(this->initialized, fs::ResultPreconditionViolation());

        /* Obtain and validate the entry index. */
        const s32 entry_index = this->meta_data->GetEntryIndex(path + 1);
        R_UNLESS(entry_index >= 0, fs::ResultPathNotFound());

        /* Create and output the file directory. */
        std::unique_ptr file = std::make_unique<PartitionFile>(this, this->meta_data->GetEntry(entry_index), mode);
        R_UNLESS(file != nullptr, fs::ResultAllocationFailureInPartitionFileSystemB());
        *out_file = std::move(file);
        return ResultSuccess();
    }

    template <typename MetaType>
    Result PartitionFileSystemCore<MetaType>::DoOpenDirectory(std::unique_ptr<fs::fsa::IDirectory> *out_dir, const char *path, fs::OpenDirectoryMode mode) {
        /* Validate preconditions. */
        R_UNLESS(this->initialized,                                                                           fs::ResultPreconditionViolation());
        R_UNLESS(std::strncmp(path, fs::PathNormalizer::RootPath, sizeof(fs::PathNormalizer::RootPath)) == 0, fs::ResultPathNotFound());

        /* Create and output the partition directory. */
        std::unique_ptr directory = std::make_unique<PartitionDirectory>(this, mode);
        R_UNLESS(directory != nullptr, fs::ResultAllocationFailureInPartitionFileSystemC());
        *out_dir = std::move(directory);
        return ResultSuccess();
    }

    template <typename MetaType>
    Result PartitionFileSystemCore<MetaType>::DoCommit() {
        return ResultSuccess();
    }

    template <typename MetaType>
    Result PartitionFileSystemCore<MetaType>::DoCleanDirectoryRecursively(const char *path) {
        return fs::ResultUnsupportedOperationInPartitionFileSystemA();
    }

    template <typename MetaType>
    Result PartitionFileSystemCore<MetaType>::DoCreateDirectory(const char *path) {
        return fs::ResultUnsupportedOperationInPartitionFileSystemA();
    }

    template <typename MetaType>
    Result PartitionFileSystemCore<MetaType>::DoCreateFile(const char *path, s64 size, int option) {
        return fs::ResultUnsupportedOperationInPartitionFileSystemA();
    }

    template <typename MetaType>
    Result PartitionFileSystemCore<MetaType>::DoDeleteDirectory(const char *path) {
        return fs::ResultUnsupportedOperationInPartitionFileSystemA();
    }

    template <typename MetaType>
    Result PartitionFileSystemCore<MetaType>::DoDeleteDirectoryRecursively(const char *path) {
        return fs::ResultUnsupportedOperationInPartitionFileSystemA();
    }

    template <typename MetaType>
    Result PartitionFileSystemCore<MetaType>::DoDeleteFile(const char *path) {
        return fs::ResultUnsupportedOperationInPartitionFileSystemA();
    }

    template <typename MetaType>
    Result PartitionFileSystemCore<MetaType>::DoRenameDirectory(const char *old_path, const char *new_path) {
        return fs::ResultUnsupportedOperationInPartitionFileSystemA();
    }

    template <typename MetaType>
    Result PartitionFileSystemCore<MetaType>::DoRenameFile(const char *old_path, const char *new_path) {
        return fs::ResultUnsupportedOperationInPartitionFileSystemA();
    }

    template <typename MetaType>
    Result PartitionFileSystemCore<MetaType>::DoCommitProvisionally(s64 counter) {
        return fs::ResultUnsupportedOperationInPartitionFileSystemB();
    }

    template class PartitionFileSystemCore<PartitionFileSystemMeta>;
    template class PartitionFileSystemCore<Sha256PartitionFileSystemMeta>;

}
