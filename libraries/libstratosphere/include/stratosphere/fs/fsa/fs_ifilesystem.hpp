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
#include "../fs_common.hpp"
#include "../fs_filesystem.hpp"

namespace ams::fs::fsa {

    class IFile;
    class IDirectory;

    enum class QueryId {
        SetConcatenationFileAttribute        = 0,
        IsSignedSystemPartitionOnSdCardValid = 2,
    };

    class IFileSystem {
        public:
            virtual ~IFileSystem() { /* ... */ }

            Result CreateFile(const char *path, s64 size, int option) {
                R_UNLESS(path != nullptr, fs::ResultInvalidPath());
                R_UNLESS(size >= 0,       fs::ResultOutOfRange());
                return this->CreateFileImpl(path, size, option);
            }

            Result CreateFile(const char *path, s64 size) {
                return this->CreateFile(path, size, 0);
            }

            Result DeleteFile(const char *path) {
                R_UNLESS(path != nullptr, fs::ResultInvalidPath());
                return this->DeleteFileImpl(path);
            }

            Result CreateDirectory(const char *path) {
                R_UNLESS(path != nullptr, fs::ResultInvalidPath());
                return this->CreateDirectoryImpl(path);
            }

            Result DeleteDirectory(const char *path) {
                R_UNLESS(path != nullptr, fs::ResultInvalidPath());
                return this->DeleteDirectoryImpl(path);
            }

            Result DeleteDirectoryRecursively(const char *path) {
                R_UNLESS(path != nullptr, fs::ResultInvalidPath());
                return this->DeleteDirectoryRecursivelyImpl(path);
            }

            Result RenameFile(const char *old_path, const char *new_path) {
                R_UNLESS(old_path != nullptr, fs::ResultInvalidPath());
                R_UNLESS(new_path != nullptr, fs::ResultInvalidPath());
                return this->RenameFileImpl(old_path, new_path);
            }

            Result RenameDirectory(const char *old_path, const char *new_path) {
                R_UNLESS(old_path != nullptr, fs::ResultInvalidPath());
                R_UNLESS(new_path != nullptr, fs::ResultInvalidPath());
                return this->RenameDirectoryImpl(old_path, new_path);
            }

            Result GetEntryType(DirectoryEntryType *out, const char *path) {
                R_UNLESS(path != nullptr, fs::ResultInvalidPath());
                R_UNLESS(out != nullptr,  fs::ResultNullptrArgument());
                return this->GetEntryTypeImpl(out, path);
            }

            Result OpenFile(std::unique_ptr<IFile> *out_file, const char *path, OpenMode mode) {
                R_UNLESS(path != nullptr,                  fs::ResultInvalidPath());
                R_UNLESS(out_file != nullptr,              fs::ResultNullptrArgument());
                R_UNLESS((mode & OpenMode_ReadWrite) != 0, fs::ResultInvalidOpenMode());
                R_UNLESS((mode & ~OpenMode_All) == 0,      fs::ResultInvalidOpenMode());
                return this->OpenFileImpl(out_file, path, mode);
            }

            Result OpenDirectory(std::unique_ptr<IDirectory> *out_dir, const char *path, OpenDirectoryMode mode) {
                R_UNLESS(path != nullptr,                                                               fs::ResultInvalidPath());
                R_UNLESS(out_dir != nullptr,                                                            fs::ResultNullptrArgument());
                R_UNLESS((mode &  OpenDirectoryMode_All) != 0,                                          fs::ResultInvalidOpenMode());
                R_UNLESS((mode & ~(OpenDirectoryMode_All | OpenDirectoryMode_NotRequireFileSize)) == 0, fs::ResultInvalidOpenMode());
                return this->OpenDirectoryImpl(out_dir, path, mode);
            }

            Result Commit() {
                return this->CommitImpl();
            }

            Result GetFreeSpaceSize(s64 *out, const char *path) {
                R_UNLESS(path != nullptr, fs::ResultInvalidPath());
                R_UNLESS(out != nullptr,  fs::ResultNullptrArgument());
                return this->GetFreeSpaceSizeImpl(out, path);
            }

            Result GetTotalSpaceSize(s64 *out, const char *path) {
                R_UNLESS(path != nullptr, fs::ResultInvalidPath());
                R_UNLESS(out != nullptr,  fs::ResultNullptrArgument());
                return this->GetTotalSpaceSizeImpl(out, path);
            }

            Result CleanDirectoryRecursively(const char *path) {
                R_UNLESS(path != nullptr, fs::ResultInvalidPath());
                return this->CleanDirectoryRecursivelyImpl(path);
            }

            Result GetFileTimeStampRaw(FileTimeStampRaw *out, const char *path) {
                R_UNLESS(path != nullptr, fs::ResultInvalidPath());
                R_UNLESS(out != nullptr,  fs::ResultNullptrArgument());
                return this->GetFileTimeStampRawImpl(out, path);
            }

            Result QueryEntry(char *dst, size_t dst_size, const char *src, size_t src_size, QueryId query, const char *path) {
                R_UNLESS(path != nullptr, fs::ResultInvalidPath());
                return this->QueryEntryImpl(dst, dst_size, src, src_size, query, path);
            }

            /* These aren't accessible as commands. */

            Result CommitProvisionally(s64 counter) {
                return this->CommitProvisionallyImpl(counter);
            }

            Result Rollback() {
                return this->RollbackImpl();
            }

            Result Flush() {
                return this->FlushImpl();
            }

        protected:
            /* ...? */
        private:
            virtual Result CreateFileImpl(const char *path, s64 size, int flags) = 0;
            virtual Result DeleteFileImpl(const char *path) = 0;
            virtual Result CreateDirectoryImpl(const char *path) = 0;
            virtual Result DeleteDirectoryImpl(const char *path) = 0;
            virtual Result DeleteDirectoryRecursivelyImpl(const char *path) = 0;
            virtual Result RenameFileImpl(const char *old_path, const char *new_path) = 0;
            virtual Result RenameDirectoryImpl(const char *old_path, const char *new_path) = 0;
            virtual Result GetEntryTypeImpl(fs::DirectoryEntryType *out, const char *path) = 0;
            virtual Result OpenFileImpl(std::unique_ptr<fs::fsa::IFile> *out_file, const char *path, fs::OpenMode mode) = 0;
            virtual Result OpenDirectoryImpl(std::unique_ptr<fs::fsa::IDirectory> *out_dir, const char *path, fs::OpenDirectoryMode mode) = 0;
            virtual Result CommitImpl() = 0;

            virtual Result GetFreeSpaceSizeImpl(s64 *out, const char *path) {
                return fs::ResultNotImplemented();
            }

            virtual Result GetTotalSpaceSizeImpl(s64 *out, const char *path) {
                return fs::ResultNotImplemented();
            }

            virtual Result CleanDirectoryRecursivelyImpl(const char *path) = 0;

            virtual Result GetFileTimeStampRawImpl(fs::FileTimeStampRaw *out, const char *path) {
                return fs::ResultNotImplemented();
            }

            virtual Result QueryEntryImpl(char *dst, size_t dst_size, const char *src, size_t src_size, fs::fsa::QueryId query, const char *path) {
                return fs::ResultNotImplemented();
            }

            /* These aren't accessible as commands. */
            virtual Result CommitProvisionallyImpl(s64 counter) {
                return fs::ResultNotImplemented();
            }

            virtual Result RollbackImpl() {
                return fs::ResultNotImplemented();
            }

            virtual Result FlushImpl() {
                return fs::ResultNotImplemented();
            }
    };

}
