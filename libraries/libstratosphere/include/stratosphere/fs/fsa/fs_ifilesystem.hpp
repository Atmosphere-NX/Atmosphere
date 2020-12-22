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
#include "../fs_filesystem_for_debug.hpp"

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
                return this->DoCreateFile(path, size, option);
            }

            Result CreateFile(const char *path, s64 size) {
                return this->CreateFile(path, size, 0);
            }

            Result DeleteFile(const char *path) {
                R_UNLESS(path != nullptr, fs::ResultInvalidPath());
                return this->DoDeleteFile(path);
            }

            Result CreateDirectory(const char *path) {
                R_UNLESS(path != nullptr, fs::ResultInvalidPath());
                return this->DoCreateDirectory(path);
            }

            Result DeleteDirectory(const char *path) {
                R_UNLESS(path != nullptr, fs::ResultInvalidPath());
                return this->DoDeleteDirectory(path);
            }

            Result DeleteDirectoryRecursively(const char *path) {
                R_UNLESS(path != nullptr, fs::ResultInvalidPath());
                return this->DoDeleteDirectoryRecursively(path);
            }

            Result RenameFile(const char *old_path, const char *new_path) {
                R_UNLESS(old_path != nullptr, fs::ResultInvalidPath());
                R_UNLESS(new_path != nullptr, fs::ResultInvalidPath());
                return this->DoRenameFile(old_path, new_path);
            }

            Result RenameDirectory(const char *old_path, const char *new_path) {
                R_UNLESS(old_path != nullptr, fs::ResultInvalidPath());
                R_UNLESS(new_path != nullptr, fs::ResultInvalidPath());
                return this->DoRenameDirectory(old_path, new_path);
            }

            Result GetEntryType(DirectoryEntryType *out, const char *path) {
                R_UNLESS(path != nullptr, fs::ResultInvalidPath());
                R_UNLESS(out != nullptr,  fs::ResultNullptrArgument());
                return this->DoGetEntryType(out, path);
            }

            Result OpenFile(std::unique_ptr<IFile> *out_file, const char *path, OpenMode mode) {
                R_UNLESS(path != nullptr,                  fs::ResultInvalidPath());
                R_UNLESS(out_file != nullptr,              fs::ResultNullptrArgument());
                R_UNLESS((mode & OpenMode_ReadWrite) != 0, fs::ResultInvalidOpenMode());
                R_UNLESS((mode & ~OpenMode_All) == 0,      fs::ResultInvalidOpenMode());
                return this->DoOpenFile(out_file, path, mode);
            }

            Result OpenDirectory(std::unique_ptr<IDirectory> *out_dir, const char *path, OpenDirectoryMode mode) {
                R_UNLESS(path != nullptr,                                                               fs::ResultInvalidPath());
                R_UNLESS(out_dir != nullptr,                                                            fs::ResultNullptrArgument());
                R_UNLESS((mode &  OpenDirectoryMode_All) != 0,                                          fs::ResultInvalidOpenMode());
                R_UNLESS((mode & ~(OpenDirectoryMode_All | OpenDirectoryMode_NotRequireFileSize)) == 0, fs::ResultInvalidOpenMode());
                return this->DoOpenDirectory(out_dir, path, mode);
            }

            Result Commit() {
                return this->DoCommit();
            }

            Result GetFreeSpaceSize(s64 *out, const char *path) {
                R_UNLESS(path != nullptr, fs::ResultInvalidPath());
                R_UNLESS(out != nullptr,  fs::ResultNullptrArgument());
                return this->DoGetFreeSpaceSize(out, path);
            }

            Result GetTotalSpaceSize(s64 *out, const char *path) {
                R_UNLESS(path != nullptr, fs::ResultInvalidPath());
                R_UNLESS(out != nullptr,  fs::ResultNullptrArgument());
                return this->DoGetTotalSpaceSize(out, path);
            }

            Result CleanDirectoryRecursively(const char *path) {
                R_UNLESS(path != nullptr, fs::ResultInvalidPath());
                return this->DoCleanDirectoryRecursively(path);
            }

            Result GetFileTimeStampRaw(FileTimeStampRaw *out, const char *path) {
                R_UNLESS(path != nullptr, fs::ResultInvalidPath());
                R_UNLESS(out != nullptr,  fs::ResultNullptrArgument());
                return this->DoGetFileTimeStampRaw(out, path);
            }

            Result QueryEntry(char *dst, size_t dst_size, const char *src, size_t src_size, QueryId query, const char *path) {
                R_UNLESS(path != nullptr, fs::ResultInvalidPath());
                return this->DoQueryEntry(dst, dst_size, src, src_size, query, path);
            }

            /* These aren't accessible as commands. */

            Result CommitProvisionally(s64 counter) {
                return this->DoCommitProvisionally(counter);
            }

            Result Rollback() {
                return this->DoRollback();
            }

            Result Flush() {
                return this->DoFlush();
            }

        protected:
            /* ...? */
        private:
            virtual Result DoCreateFile(const char *path, s64 size, int flags) = 0;
            virtual Result DoDeleteFile(const char *path) = 0;
            virtual Result DoCreateDirectory(const char *path) = 0;
            virtual Result DoDeleteDirectory(const char *path) = 0;
            virtual Result DoDeleteDirectoryRecursively(const char *path) = 0;
            virtual Result DoRenameFile(const char *old_path, const char *new_path) = 0;
            virtual Result DoRenameDirectory(const char *old_path, const char *new_path) = 0;
            virtual Result DoGetEntryType(fs::DirectoryEntryType *out, const char *path) = 0;
            virtual Result DoOpenFile(std::unique_ptr<fs::fsa::IFile> *out_file, const char *path, fs::OpenMode mode) = 0;
            virtual Result DoOpenDirectory(std::unique_ptr<fs::fsa::IDirectory> *out_dir, const char *path, fs::OpenDirectoryMode mode) = 0;
            virtual Result DoCommit() = 0;

            virtual Result DoGetFreeSpaceSize(s64 *out, const char *path) {
                return fs::ResultNotImplemented();
            }

            virtual Result DoGetTotalSpaceSize(s64 *out, const char *path) {
                return fs::ResultNotImplemented();
            }

            virtual Result DoCleanDirectoryRecursively(const char *path) = 0;

            virtual Result DoGetFileTimeStampRaw(fs::FileTimeStampRaw *out, const char *path) {
                return fs::ResultNotImplemented();
            }

            virtual Result DoQueryEntry(char *dst, size_t dst_size, const char *src, size_t src_size, fs::fsa::QueryId query, const char *path) {
                return fs::ResultNotImplemented();
            }

            /* These aren't accessible as commands. */
            virtual Result DoCommitProvisionally(s64 counter) {
                return fs::ResultNotImplemented();
            }

            virtual Result DoRollback() {
                return fs::ResultNotImplemented();
            }

            virtual Result DoFlush() {
                return fs::ResultNotImplemented();
            }
    };

}
