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
#include <stratosphere/fs/fs_common.hpp>
#include <stratosphere/fs/impl/fs_newable.hpp>
#include <stratosphere/fs/fsa/fs_ifile.hpp>
#include <stratosphere/fs/fsa/fs_idirectory.hpp>
#include <stratosphere/fs/fsa/fs_ifilesystem.hpp>

namespace ams::fs {

    class SharedFileSystemHolder : public fsa::IFileSystem, public impl::Newable {
        NON_COPYABLE(SharedFileSystemHolder);
        NON_MOVEABLE(SharedFileSystemHolder);
        private:
            std::shared_ptr<fsa::IFileSystem> fs;
        public:
            SharedFileSystemHolder(std::shared_ptr<fsa::IFileSystem> f) : fs(std::move(f)) { /* ... */ }
        public:
            virtual Result CreateFileImpl(const char *path, s64 size, int flags) override { return this->fs->CreateFile(path, size, flags); }
            virtual Result DeleteFileImpl(const char *path) override { return this->fs->DeleteFile(path); }
            virtual Result CreateDirectoryImpl(const char *path) override { return this->fs->CreateDirectory(path); }
            virtual Result DeleteDirectoryImpl(const char *path) override { return this->fs->DeleteDirectory(path); }
            virtual Result DeleteDirectoryRecursivelyImpl(const char *path) override { return this->fs->DeleteDirectoryRecursively(path); }
            virtual Result RenameFileImpl(const char *old_path, const char *new_path) override { return this->fs->RenameFile(old_path, new_path); }
            virtual Result RenameDirectoryImpl(const char *old_path, const char *new_path) override { return this->fs->RenameDirectory(old_path, new_path); }
            virtual Result GetEntryTypeImpl(fs::DirectoryEntryType *out, const char *path) override { return this->fs->GetEntryType(out, path); }
            virtual Result OpenFileImpl(std::unique_ptr<fs::fsa::IFile> *out_file, const char *path, fs::OpenMode mode) override { return this->fs->OpenFile(out_file, path, mode); }
            virtual Result OpenDirectoryImpl(std::unique_ptr<fs::fsa::IDirectory> *out_dir, const char *path, fs::OpenDirectoryMode mode) override { return this->fs->OpenDirectory(out_dir, path, mode); }
            virtual Result CommitImpl() override { return this->fs->Commit(); }
            virtual Result GetFreeSpaceSizeImpl(s64 *out, const char *path) override { return this->fs->GetFreeSpaceSize(out, path); }
            virtual Result GetTotalSpaceSizeImpl(s64 *out, const char *path) override { return this->fs->GetTotalSpaceSize(out, path); }
            virtual Result CleanDirectoryRecursivelyImpl(const char *path) override { return this->fs->CleanDirectoryRecursively(path); }

            /* These aren't accessible as commands. */
            virtual Result CommitProvisionallyImpl(s64 counter) override { return this->fs->CommitProvisionally(counter); }
            virtual Result RollbackImpl() override { return this->fs->Rollback(); }
            virtual Result FlushImpl() override { return this->fs->Flush(); }
    };

}
