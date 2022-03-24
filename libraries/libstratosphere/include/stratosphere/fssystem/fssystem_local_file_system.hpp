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
#include <vapours.hpp>
#include <stratosphere/fs/fsa/fs_ifilesystem.hpp>
#include <stratosphere/fs/fs_memory_management.hpp>
#include <stratosphere/fs/fs_path.hpp>

namespace ams::fssystem {

    /* TODO: Put this in its own header? */
    enum PathCaseSensitiveMode {
        PathCaseSensitiveMode_CaseInsensitive = 0,
        PathCaseSensitiveMode_CaseSensitive   = 1,
    };

    /* ACCURATE_TO_VERSION: 13.4.0.0 */
    class LocalFileSystem : public fs::fsa::IFileSystem, public fs::impl::Newable {
        NON_COPYABLE(LocalFileSystem);
        NON_MOVEABLE(LocalFileSystem);
        private:
            #if defined(ATMOSPHERE_OS_WINDOWS)
            using NativeCharacterType = wchar_t;
            #else
            using NativeCharacterType = char;
            #endif

            using NativePathBuffer = std::unique_ptr<NativeCharacterType[], ::ams::fs::impl::Deleter>;
        private:
            fs::Path m_root_path;
            fssystem::PathCaseSensitiveMode m_case_sensitive_mode;
            NativePathBuffer m_native_path_buffer;
            int m_native_path_length;
            bool m_use_posix_time;
        public:
            LocalFileSystem(bool posix_time = true) : m_root_path(), m_native_path_buffer(), m_native_path_length(0), m_use_posix_time(posix_time) {
                /* ... */
            }

            Result Initialize(const fs::Path &root_path, fssystem::PathCaseSensitiveMode case_sensitive_mode);

            Result GetCaseSensitivePath(int *out_size, char *dst, size_t dst_size, const char *path, const char *work_path);
        private:
            Result CheckPathCaseSensitively(const NativeCharacterType *path, const NativeCharacterType *root_path, NativeCharacterType *cs_buf, size_t cs_size, bool check_case_sensitivity);
            Result ResolveFullPath(NativePathBuffer *out, const fs::Path &path, int max_len, int min_len, bool check_case_sensitivity);
        public:
            virtual Result DoCreateFile(const fs::Path &path, s64 size, int flags) override;
            virtual Result DoDeleteFile(const fs::Path &path) override;
            virtual Result DoCreateDirectory(const fs::Path &path) override;
            virtual Result DoDeleteDirectory(const fs::Path &path) override;
            virtual Result DoDeleteDirectoryRecursively(const fs::Path &path) override;
            virtual Result DoRenameFile(const fs::Path &old_path, const fs::Path &new_path) override;
            virtual Result DoRenameDirectory(const fs::Path &old_path, const fs::Path &new_path) override;
            virtual Result DoGetEntryType(fs::DirectoryEntryType *out, const fs::Path &path) override;
            virtual Result DoOpenFile(std::unique_ptr<fs::fsa::IFile> *out_file, const fs::Path &path, fs::OpenMode mode) override;
            virtual Result DoOpenDirectory(std::unique_ptr<fs::fsa::IDirectory> *out_dir, const fs::Path &path, fs::OpenDirectoryMode mode) override;
            virtual Result DoCommit() override;
            virtual Result DoGetFreeSpaceSize(s64 *out, const fs::Path &path) override;
            virtual Result DoGetTotalSpaceSize(s64 *out, const fs::Path &path) override;
            virtual Result DoCleanDirectoryRecursively(const fs::Path &path) override;
            virtual Result DoGetFileTimeStampRaw(fs::FileTimeStampRaw *out, const fs::Path &path) override;
            virtual Result DoQueryEntry(char *dst, size_t dst_size, const char *src, size_t src_size, fs::fsa::QueryId query, const fs::Path &path) override;

            /* These aren't accessible as commands. */
            virtual Result DoCommitProvisionally(s64 counter) override;
            virtual Result DoRollback() override;
        public:
            Result DoGetDiskFreeSpace(s64 *out_free, s64 *out_total, s64 *out_total_free, const fs::Path &path);
            Result DeleteDirectoryRecursivelyInternal(const NativeCharacterType *path, bool delete_top);
    };

}
