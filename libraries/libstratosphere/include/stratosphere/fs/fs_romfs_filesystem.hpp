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
#include "fs_common.hpp"
#include "impl/fs_newable.hpp"
#include "fsa/fs_ifile.hpp"
#include "fsa/fs_idirectory.hpp"
#include "fsa/fs_ifilesystem.hpp"
#include "fs_dbm_hierarchical_rom_file_table.hpp"

namespace ams::fs {

    class RomFsFileSystem : public fsa::IFileSystem, public impl::Newable {
        NON_COPYABLE(RomFsFileSystem);
        public:
            using RomFileTable = HierarchicalRomFileTable;
        private:
            RomFileTable rom_file_table;
            IStorage *base_storage;
            std::unique_ptr<IStorage> unique_storage;
            std::unique_ptr<IStorage> dir_bucket_storage;
            std::unique_ptr<IStorage> dir_entry_storage;
            std::unique_ptr<IStorage> file_bucket_storage;
            std::unique_ptr<IStorage> file_entry_storage;
            s64 entry_size;
        private:
            Result GetFileInfo(RomFileTable::FileInfo *out, const char *path);
        public:
            static Result GetRequiredWorkingMemorySize(size_t *out, IStorage *storage);
        public:
            RomFsFileSystem();
            virtual ~RomFsFileSystem() override;

            Result Initialize(IStorage *base, void *work, size_t work_size, bool use_cache);
            Result Initialize(std::unique_ptr<IStorage>&& base, void *work, size_t work_size, bool use_cache);

            IStorage *GetBaseStorage();
            RomFileTable *GetRomFileTable();
            Result GetFileBaseOffset(s64 *out, const char *path);
        public:
            virtual Result CreateFileImpl(const char *path, s64 size, int flags) override;
            virtual Result DeleteFileImpl(const char *path) override;
            virtual Result CreateDirectoryImpl(const char *path) override;
            virtual Result DeleteDirectoryImpl(const char *path) override;
            virtual Result DeleteDirectoryRecursivelyImpl(const char *path) override;
            virtual Result RenameFileImpl(const char *old_path, const char *new_path) override;
            virtual Result RenameDirectoryImpl(const char *old_path, const char *new_path) override;
            virtual Result GetEntryTypeImpl(fs::DirectoryEntryType *out, const char *path) override;
            virtual Result OpenFileImpl(std::unique_ptr<fs::fsa::IFile> *out_file, const char *path, fs::OpenMode mode) override;
            virtual Result OpenDirectoryImpl(std::unique_ptr<fs::fsa::IDirectory> *out_dir, const char *path, fs::OpenDirectoryMode mode) override;
            virtual Result CommitImpl() override;
            virtual Result GetFreeSpaceSizeImpl(s64 *out, const char *path) override;
            virtual Result GetTotalSpaceSizeImpl(s64 *out, const char *path) override;
            virtual Result CleanDirectoryRecursivelyImpl(const char *path) override;

            /* These aren't accessible as commands. */
            virtual Result CommitProvisionallyImpl(s64 counter) override;
            virtual Result RollbackImpl() override;
    };

}
