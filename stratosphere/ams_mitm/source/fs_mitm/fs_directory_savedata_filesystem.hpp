/*
 * Copyright (c) 2018 Atmosphère-NX
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

#include "fs_ifilesystem.hpp"
#include "fs_path_utils.hpp"

class DirectorySaveDataFileSystem : public IFileSystem {
    private:
        static constexpr FsPath CommittedDirectoryPath     = EncodeConstantFsPath("/0/");
        static constexpr FsPath WorkingDirectoryPath       = EncodeConstantFsPath("/1/");
        static constexpr FsPath SynchronizingDirectoryPath = EncodeConstantFsPath("/_/");

        static constexpr size_t CommittedDirectoryPathLen     = GetConstantFsPathLen(CommittedDirectoryPath);
        static constexpr size_t WorkingDirectoryPathLen       = GetConstantFsPathLen(WorkingDirectoryPath);
        static constexpr size_t SynchronizingDirectoryPathLen = GetConstantFsPathLen(SynchronizingDirectoryPath);

        static constexpr size_t IdealWorkBuffersize = 0x100000; /* 1 MB */
    private:
        std::shared_ptr<IFileSystem> shared_fs;
        std::unique_ptr<IFileSystem> unique_fs;
        IFileSystem *fs;
        HosMutex lock;
        size_t open_writable_files = 0;

    public:
        DirectorySaveDataFileSystem(IFileSystem *fs) : unique_fs(fs) {
            this->fs = this->unique_fs.get();
            Result rc = this->Initialize();
            if (R_FAILED(rc)) {
                fatalSimple(rc);
            }
        }

        DirectorySaveDataFileSystem(std::unique_ptr<IFileSystem> fs) : unique_fs(std::move(fs)) {
            this->fs = this->unique_fs.get();
            Result rc = this->Initialize();
            if (R_FAILED(rc)) {
                fatalSimple(rc);
            }
        }

        DirectorySaveDataFileSystem(std::shared_ptr<IFileSystem> fs) : shared_fs(fs) {
            this->fs = this->shared_fs.get();
            Result rc = this->Initialize();
            if (R_FAILED(rc)) {
                fatalSimple(rc);
            }
        }


        virtual ~DirectorySaveDataFileSystem() { }

    private:
        Result Initialize();
    protected:
        Result SynchronizeDirectory(const FsPath &dst_dir, const FsPath &src_dir);
        Result AllocateWorkBuffer(void **out_buf, size_t *out_size, const size_t ideal_size);

        Result GetFullPath(char *out, size_t out_size, const char *relative_path);
        Result GetFullPath(FsPath &full_path, const FsPath &relative_path) {
            return GetFullPath(full_path.str, sizeof(full_path.str), relative_path.str);
        }
    public:
        void OnWritableFileClose();
    public:
        virtual Result CreateFileImpl(const FsPath &path, uint64_t size, int flags) override;
        virtual Result DeleteFileImpl(const FsPath &path) override;
        virtual Result CreateDirectoryImpl(const FsPath &path) override;
        virtual Result DeleteDirectoryImpl(const FsPath &path) override;
        virtual Result DeleteDirectoryRecursivelyImpl(const FsPath &path) override;
        virtual Result RenameFileImpl(const FsPath &old_path, const FsPath &new_path) override;
        virtual Result RenameDirectoryImpl(const FsPath &old_path, const FsPath &new_path) override;
        virtual Result GetEntryTypeImpl(DirectoryEntryType *out, const FsPath &path) override;
        virtual Result OpenFileImpl(std::unique_ptr<IFile> &out_file, const FsPath &path, OpenMode mode) override;
        virtual Result OpenDirectoryImpl(std::unique_ptr<IDirectory> &out_dir, const FsPath &path, DirectoryOpenMode mode) override;
        virtual Result CommitImpl() override;
        virtual Result GetFreeSpaceSizeImpl(uint64_t *out, const FsPath &path) override;
        virtual Result GetTotalSpaceSizeImpl(uint64_t *out, const FsPath &path) override;
        virtual Result CleanDirectoryRecursivelyImpl(const FsPath &path) override;
        virtual Result GetFileTimeStampRawImpl(FsTimeStampRaw *out, const FsPath &path) override;
        virtual Result QueryEntryImpl(char *out, uint64_t out_size, const char *in, uint64_t in_size, int query, const FsPath &path) override;
};