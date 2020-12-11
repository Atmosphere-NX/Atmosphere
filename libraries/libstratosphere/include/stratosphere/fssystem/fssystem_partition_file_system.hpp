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
#include "fssystem_partition_file_system_meta.hpp"
#include "../fs/fsa/fs_ifile.hpp"
#include "../fs/fsa/fs_idirectory.hpp"
#include "../fs/fsa/fs_ifilesystem.hpp"

namespace ams::fssystem {

    template<typename MetaType>
    class PartitionFileSystemCore : public fs::impl::Newable, public fs::fsa::IFileSystem {
        NON_COPYABLE(PartitionFileSystemCore);
        NON_MOVEABLE(PartitionFileSystemCore);
        private:
            class PartitionFile;
            class PartitionDirectory;
        private:
            fs::IStorage *base_storage;
            MetaType *meta_data;
            bool initialized;
            size_t meta_data_size;
            std::unique_ptr<MetaType> unique_meta_data;
            std::shared_ptr<fs::IStorage> shared_storage;
        private:
            Result Initialize(fs::IStorage *base_storage, MemoryResource *allocator);
        public:
            PartitionFileSystemCore();
            virtual ~PartitionFileSystemCore() override;

            Result Initialize(std::unique_ptr<MetaType> &&meta_data, std::shared_ptr<fs::IStorage> base_storage);
            Result Initialize(MetaType *meta_data, std::shared_ptr<fs::IStorage> base_storage);
            Result Initialize(fs::IStorage *base_storage);
            Result Initialize(std::shared_ptr<fs::IStorage> base_storage);
            Result Initialize(std::shared_ptr<fs::IStorage> base_storage, MemoryResource *allocator);

            Result GetFileBaseOffset(s64 *out_offset, const char *path);

            virtual Result DoCreateFile(const char *path, s64 size, int option) override;
            virtual Result DoDeleteFile(const char *path) override;
            virtual Result DoCreateDirectory(const char *path) override;
            virtual Result DoDeleteDirectory(const char *path) override;
            virtual Result DoDeleteDirectoryRecursively(const char *path) override;
            virtual Result DoRenameFile(const char *old_path, const char *new_path) override;
            virtual Result DoRenameDirectory(const char *old_path, const char *new_path) override;
            virtual Result DoGetEntryType(fs::DirectoryEntryType *out, const char *path) override;
            virtual Result DoOpenFile(std::unique_ptr<fs::fsa::IFile> *out_file, const char *path, fs::OpenMode mode) override;
            virtual Result DoOpenDirectory(std::unique_ptr<fs::fsa::IDirectory> *out_dir, const char *path, fs::OpenDirectoryMode mode) override;
            virtual Result DoCommit() override;
            virtual Result DoCleanDirectoryRecursively(const char *path) override;

            /* These aren't accessible as commands. */
            virtual Result DoCommitProvisionally(s64 counter) override;
    };

    using PartitionFileSystem       = PartitionFileSystemCore<PartitionFileSystemMeta>;
    using Sha256PartitionFileSystem = PartitionFileSystemCore<Sha256PartitionFileSystemMeta>;

}
