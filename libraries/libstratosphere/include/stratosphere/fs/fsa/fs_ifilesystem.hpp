/*
 * Copyright (c) Atmosphère-NX
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
#include <stratosphere/fs/fs_filesystem.hpp>
#include <stratosphere/fs/fs_filesystem_for_debug.hpp>
#include <stratosphere/fs/fs_path.hpp>

namespace ams::fs::fsa {

    /* ACCURATE_TO_VERSION: Unknown */
    class IFile;
    class IDirectory;

    enum class QueryId {
        SetConcatenationFileAttribute        = 0,
        UpdateMac                            = 1,
        IsSignedSystemPartitionOnSdCardValid = 2,
        QueryUnpreparedFileInformation       = 3,
    };

    class IFileSystem {
        public:
            virtual ~IFileSystem() { /* ... */ }

            Result CreateFile(const fs::Path &path, s64 size, int option) {
                R_UNLESS(size >= 0, fs::ResultOutOfRange());
                R_RETURN(this->DoCreateFile(path, size, option));
            }

            Result CreateFile(const fs::Path &path, s64 size) {
                R_RETURN(this->CreateFile(path, size, fs::CreateOption_None));
            }

            Result DeleteFile(const fs::Path &path) {
                R_RETURN(this->DoDeleteFile(path));
            }

            Result CreateDirectory(const fs::Path &path) {
                R_RETURN(this->DoCreateDirectory(path));
            }

            Result DeleteDirectory(const fs::Path &path) {
                R_RETURN(this->DoDeleteDirectory(path));
            }

            Result DeleteDirectoryRecursively(const fs::Path &path) {
                R_RETURN(this->DoDeleteDirectoryRecursively(path));
            }

            Result RenameFile(const fs::Path &old_path, const fs::Path &new_path) {
                R_RETURN(this->DoRenameFile(old_path, new_path));
            }

            Result RenameDirectory(const fs::Path &old_path, const fs::Path &new_path) {
                R_RETURN(this->DoRenameDirectory(old_path, new_path));
            }

            Result GetEntryType(DirectoryEntryType *out, const fs::Path &path) {
                R_RETURN(this->DoGetEntryType(out, path));
            }

            Result OpenFile(std::unique_ptr<IFile> *out_file, const fs::Path &path, OpenMode mode) {
                R_UNLESS(out_file != nullptr,              fs::ResultNullptrArgument());
                R_UNLESS((mode & OpenMode_ReadWrite) != 0, fs::ResultInvalidOpenMode());
                R_UNLESS((mode & ~OpenMode_All) == 0,      fs::ResultInvalidOpenMode());
                R_RETURN(this->DoOpenFile(out_file, path, mode));
            }

            Result OpenDirectory(std::unique_ptr<IDirectory> *out_dir, const fs::Path &path, OpenDirectoryMode mode) {
                R_UNLESS(out_dir != nullptr,                                                            fs::ResultNullptrArgument());
                R_UNLESS((mode &  OpenDirectoryMode_All) != 0,                                          fs::ResultInvalidOpenMode());
                R_UNLESS((mode & ~(OpenDirectoryMode_All | OpenDirectoryMode_NotRequireFileSize)) == 0, fs::ResultInvalidOpenMode());
                R_RETURN(this->DoOpenDirectory(out_dir, path, mode));
            }

            Result Commit() {
                R_RETURN(this->DoCommit());
            }

            Result GetFreeSpaceSize(s64 *out, const fs::Path &path) {
                R_UNLESS(out != nullptr,  fs::ResultNullptrArgument());
                R_RETURN(this->DoGetFreeSpaceSize(out, path));
            }

            Result GetTotalSpaceSize(s64 *out, const fs::Path &path) {
                R_UNLESS(out != nullptr,  fs::ResultNullptrArgument());
                R_RETURN(this->DoGetTotalSpaceSize(out, path));
            }

            Result CleanDirectoryRecursively(const fs::Path &path) {
                R_RETURN(this->DoCleanDirectoryRecursively(path));
            }

            Result GetFileTimeStampRaw(FileTimeStampRaw *out, const fs::Path &path) {
                R_UNLESS(out != nullptr,  fs::ResultNullptrArgument());
                R_RETURN(this->DoGetFileTimeStampRaw(out, path));
            }

            Result QueryEntry(char *dst, size_t dst_size, const char *src, size_t src_size, QueryId query, const fs::Path &path) {
                R_RETURN(this->DoQueryEntry(dst, dst_size, src, src_size, query, path));
            }

            /* These aren't accessible as commands. */

            Result CommitProvisionally(s64 counter) {
                R_RETURN(this->DoCommitProvisionally(counter));
            }

            Result Rollback() {
                R_RETURN(this->DoRollback());
            }

            Result Flush() {
                R_RETURN(this->DoFlush());
            }

        protected:
            /* ...? */
        private:
            virtual Result DoCreateFile(const fs::Path &path, s64 size, int flags) = 0;
            virtual Result DoDeleteFile(const fs::Path &path) = 0;
            virtual Result DoCreateDirectory(const fs::Path &path) = 0;
            virtual Result DoDeleteDirectory(const fs::Path &path) = 0;
            virtual Result DoDeleteDirectoryRecursively(const fs::Path &path) = 0;
            virtual Result DoRenameFile(const fs::Path &old_path, const fs::Path &new_path) = 0;
            virtual Result DoRenameDirectory(const fs::Path &old_path, const fs::Path &new_path) = 0;
            virtual Result DoGetEntryType(fs::DirectoryEntryType *out, const fs::Path &path) = 0;
            virtual Result DoOpenFile(std::unique_ptr<fs::fsa::IFile> *out_file, const fs::Path &path, fs::OpenMode mode) = 0;
            virtual Result DoOpenDirectory(std::unique_ptr<fs::fsa::IDirectory> *out_dir, const fs::Path &path, fs::OpenDirectoryMode mode) = 0;
            virtual Result DoCommit() = 0;

            virtual Result DoGetFreeSpaceSize(s64 *out, const fs::Path &path) {
                AMS_UNUSED(out, path);
                R_THROW(fs::ResultNotImplemented());
            }

            virtual Result DoGetTotalSpaceSize(s64 *out, const fs::Path &path) {
                AMS_UNUSED(out, path);
                R_THROW(fs::ResultNotImplemented());
            }

            virtual Result DoCleanDirectoryRecursively(const fs::Path &path) = 0;

            virtual Result DoGetFileTimeStampRaw(fs::FileTimeStampRaw *out, const fs::Path &path) {
                AMS_UNUSED(out, path);
                R_THROW(fs::ResultNotImplemented());
            }

            virtual Result DoQueryEntry(char *dst, size_t dst_size, const char *src, size_t src_size, fs::fsa::QueryId query, const fs::Path &path) {
                AMS_UNUSED(dst, dst_size, src, src_size, query, path);
                R_THROW(fs::ResultNotImplemented());
            }

            /* These aren't accessible as commands. */
            virtual Result DoCommitProvisionally(s64 counter) {
                AMS_UNUSED(counter);
                R_THROW(fs::ResultNotImplemented());
            }

            virtual Result DoRollback() {
                R_THROW(fs::ResultNotImplemented());
            }

            virtual Result DoFlush() {
                R_THROW(fs::ResultNotImplemented());
            }
    };

    template<typename T>
    concept PointerToFileSystem = ::ams::util::RawOrSmartPointerTo<T, ::ams::fs::fsa::IFileSystem>;

}
