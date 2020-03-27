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
            PartitionFileSystemCore<MetaType> *parent;
            fs::OpenMode mode;
        public:
            PartitionFile(PartitionFileSystemCore<MetaType> *parent, const typename MetaType::PartitionEntry *partition_entry, fs::OpenMode mode) : partition_entry(partition_entry), parent(parent), mode(mode) { /* ... */ }

            virtual ~PartitionFile() { /* ... */ }
        private:
            virtual Result ReadImpl(size_t *out, s64 offset, void *buffer, size_t size, const fs::ReadOption &option) override final;

            virtual Result GetSizeImpl(s64 *out) override final {
                *out = this->partition_entry->size;
                return ResultSuccess();
            }

            virtual Result FlushImpl() override final {
                /* Nothing to do if writing disallowed. */
                R_SUCCEED_IF(!(this->mode & fs::OpenMode_Write));

                /* Flush base storage. */
                return this->parent->base_storage->Flush();
            }

            virtual Result WriteImpl(s64 offset, const void *buffer, size_t size, const fs::WriteOption &option) override final {
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

            virtual Result SetSizeImpl(s64 size) override final {
                R_TRY(this->DrySetSize(size, this->mode));
                return fs::ResultUnsupportedOperationInPartitionFileA();
            }

            virtual Result OperateRangeImpl(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override final {
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
    Result PartitionFileSystemCore<PartitionFileSystemMeta>::PartitionFile::ReadImpl(size_t *out, s64 offset, void *dst, size_t dst_size, const fs::ReadOption &option) {
        /* Perform a dry read. */
        size_t read_size = 0;
        R_TRY(this->DryRead(std::addressof(read_size), offset, dst_size, option, this->mode));

        /* Read from the base storage. */
        R_TRY(this->parent->base_storage->Read(this->parent->meta_data_size + this->partition_entry->offset + offset, dst, read_size));

        /* Set output size. */
        *out = read_size;
        return ResultSuccess();
    }

    template <typename MetaType>
    class PartitionFileSystemCore<MetaType>::PartitionDirectory : public fs::fsa::IDirectory, public fs::impl::Newable {
        private:
            u32 cur_index;
            PartitionFileSystemCore<MetaType> *parent;
            fs::OpenDirectoryMode mode;
        public:
            PartitionDirectory(PartitionFileSystemCore<MetaType> *parent, fs::OpenDirectoryMode mode) : cur_index(0), parent(parent), mode(mode) { /* ... */ }

            virtual ~PartitionDirectory() { /* ... */ }
        public:
            virtual Result ReadImpl(s64 *out_count, fs::DirectoryEntry *out_entries, s64 max_entries) override final {
                /* There are no subdirectories. */
                if (!(this->mode & fs::OpenDirectoryMode_File)) {
                    *out_count = 0;
                    return ResultSuccess();
                }

                /* Calculate number of entries. */
                const s32 entry_count = std::min(max_entries, static_cast<s64>(this->parent->meta_data->GetEntryCount() - this->cur_index));

                /* Populate output directory entries. */
                for (s32 i = 0; i < entry_count; i++, this->cur_index++) {
                    fs::DirectoryEntry &dir_entry = out_entries[i];
                    
                    /* Setup the output directory entry. */
                    dir_entry.type = fs::DirectoryEntryType_File;
                    dir_entry.file_size = this->parent->meta_data->GetEntry(this->cur_index)->size;
                    std::strncpy(dir_entry.name, this->parent->meta_data->GetEntryName(this->cur_index), fs::EntryNameLengthMax);
                    dir_entry.name[fs::EntryNameLengthMax] = StringTraits::NullTerminator;
                }

                *out_count = entry_count;
                return ResultSuccess();
            }

            virtual Result GetEntryCountImpl(s64 *out) override final {
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
    Result PartitionFileSystemCore<MetaType>::CleanDirectoryRecursivelyImpl(const char *path) {
        return fs::ResultUnsupportedOperationInPartitionFileSystemA();
    }

    template <typename MetaType>
    Result PartitionFileSystemCore<MetaType>::CommitImpl() {
        return ResultSuccess();
    }

    template <typename MetaType>
    Result PartitionFileSystemCore<MetaType>::CommitProvisionallyImpl(s64 counter) {
        return fs::ResultUnsupportedOperationInPartitionFileSystemB();
    }

    template <typename MetaType>
    Result PartitionFileSystemCore<MetaType>::CreateDirectoryImpl(const char *path) {
        return fs::ResultUnsupportedOperationInPartitionFileSystemA();
    }

    template <typename MetaType>
    Result PartitionFileSystemCore<MetaType>::CreateFileImpl(const char *path, s64 size, int option) {
        return fs::ResultUnsupportedOperationInPartitionFileSystemA();
    }

    template <typename MetaType>
    Result PartitionFileSystemCore<MetaType>::DeleteDirectoryImpl(const char *path) {
        return fs::ResultUnsupportedOperationInPartitionFileSystemA();
    }

    template <typename MetaType>
    Result PartitionFileSystemCore<MetaType>::DeleteDirectoryRecursivelyImpl(const char *path) {
        return fs::ResultUnsupportedOperationInPartitionFileSystemA();
    }

    template <typename MetaType>
    Result PartitionFileSystemCore<MetaType>::DeleteFileImpl(const char *path) {
        return fs::ResultUnsupportedOperationInPartitionFileSystemA();
    }

    template <typename MetaType>
    Result PartitionFileSystemCore<MetaType>::GetEntryTypeImpl(fs::DirectoryEntryType *out, const char *path) {
        /* Validate preconditions. */
        R_UNLESS(this->initialized,              fs::ResultPreconditionViolation());
        R_UNLESS(PathTool::IsSeparator(path[0]), fs::ResultInvalidPathFormat());

        /* Check if the path is for a directory. */
        if (std::strncmp(path, PathTool::RootPath, sizeof(PathTool::RootPath)) == 0) {
            *out = fs::DirectoryEntryType_Directory;
            return ResultSuccess();
        }

        /* Ensure that path is for a file. */
        R_UNLESS(this->meta_data->GetEntryIndex(path + 1) >= 0, fs::ResultPathNotFound());
 
        *out = fs::DirectoryEntryType_File;
        return ResultSuccess();
    }

    template <typename MetaType>
    Result PartitionFileSystemCore<MetaType>::OpenDirectoryImpl(std::unique_ptr<fs::fsa::IDirectory> *out_dir, const char *path, fs::OpenDirectoryMode mode) {
        /* Validate preconditions. */
        R_UNLESS(this->initialized,                                                       fs::ResultPreconditionViolation());
        R_UNLESS(std::strncmp(path, PathTool::RootPath, sizeof(PathTool::RootPath)) == 0, fs::ResultPathNotFound());

        /* Create and output the partition directory. */
        std::unique_ptr directory = std::make_unique<PartitionDirectory>(this, mode);
        R_UNLESS(directory != nullptr, fs::ResultAllocationFailureInPartitionFileSystemA());
        *out_dir = std::move(directory);
        return ResultSuccess();
    }

    template <typename MetaType>
    Result PartitionFileSystemCore<MetaType>::OpenFileImpl(std::unique_ptr<fs::fsa::IFile> *out_file, const char *path, fs::OpenMode mode) {
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
    Result PartitionFileSystemCore<MetaType>::RenameDirectoryImpl(const char *old_path, const char *new_path) {
        return fs::ResultUnsupportedOperationInPartitionFileSystemA();
    }

    template <typename MetaType>
    Result PartitionFileSystemCore<MetaType>::RenameFileImpl(const char *old_path, const char *new_path) {
        return fs::ResultUnsupportedOperationInPartitionFileSystemA();
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
    Result PartitionFileSystemCore<MetaType>::Initialize(std::unique_ptr<MetaType> &meta_data, std::shared_ptr<fs::IStorage> base_storage) {
        this->unique_meta_data = std::move(meta_data);
        return this->Initialize(this->unique_meta_data.get(), base_storage);
    }

    template <typename MetaType>
    Result PartitionFileSystemCore<MetaType>::Initialize(MetaType *meta_data, std::shared_ptr<fs::IStorage> base_storage) {
        /* Validate preconditions. */
        R_UNLESS(!this->initialized, fs::ResultPreconditionViolation());

        /* Initialize members. */
        this->shared_storage = base_storage;
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
        this->shared_storage = base_storage;
        return this->Initialize(this->shared_storage.get());
    }

    template <typename MetaType>
    Result PartitionFileSystemCore<MetaType>::Initialize(std::shared_ptr<fs::IStorage> base_storage, MemoryResource *allocator) {
        this->shared_storage = base_storage;
        return this->Initialize(this->shared_storage.get(), allocator);
    }

    template class PartitionFileSystemCore<PartitionFileSystemMeta>;

}
