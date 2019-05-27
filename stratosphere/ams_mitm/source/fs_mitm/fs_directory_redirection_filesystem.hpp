/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

class DirectoryRedirectionFileSystem : public IFileSystem {
    private:
        std::shared_ptr<IFileSystem> base_fs;
        char *before_dir = nullptr;
        size_t before_dir_len = 0;
        char *after_dir = nullptr;
        size_t after_dir_len = 0;

    public:
        DirectoryRedirectionFileSystem(IFileSystem *fs, const char *before, const char *after) : base_fs(fs) {
            Result rc = this->Initialize(before, after);
            if (R_FAILED(rc)) {
                fatalSimple(rc);
            }
        }

        DirectoryRedirectionFileSystem(std::shared_ptr<IFileSystem> fs, const char *before, const char *after) : base_fs(fs) {
            Result rc = this->Initialize(before, after);
            if (R_FAILED(rc)) {
                fatalSimple(rc);
            }
        }


        virtual ~DirectoryRedirectionFileSystem() {
            if (this->before_dir != nullptr) {
                free(this->before_dir);
            }
            if (this->after_dir != nullptr) {
                free(this->after_dir);
            }
        }

    private:
        Result Initialize(const char *before, const char *after);
    protected:
        Result GetFullPath(char *out, size_t out_size, const char *relative_path);
        Result GetFullPath(FsPath &full_path, const FsPath &relative_path) {
            return GetFullPath(full_path.str, sizeof(full_path.str), relative_path.str);
        }

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