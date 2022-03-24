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
#include <stratosphere/fs/fsa/fs_ifile.hpp>
#include <stratosphere/fs/fsa/fs_idirectory.hpp>
#include <stratosphere/fs/fsa/fs_ifilesystem.hpp>
#include <stratosphere/fs/impl/fs_newable.hpp>

namespace ams::fssystem {

    /* ACCURATE_TO_VERSION: 13.4.0.0 */
    class DirectorySaveDataFileSystem : public fs::fsa::IFileSystem, public fs::impl::Newable {
        NON_COPYABLE(DirectorySaveDataFileSystem);
        private:
            std::unique_ptr<fs::fsa::IFileSystem> m_unique_fs;
            fs::fsa::IFileSystem * const m_base_fs;
            os::SdkMutex m_accessor_mutex    = {};
            s32 m_open_writable_files        = 0;
            bool m_is_journaling_supported   = false;
            bool m_is_multi_commit_supported = false;
            bool m_is_journaling_enabled     = false;

            /* Extension member to ensure proper savedata locking. */
            std::unique_ptr<fs::fsa::IFile> m_lock_file = nullptr;
        public:
            DirectorySaveDataFileSystem(std::unique_ptr<fs::fsa::IFileSystem> fs) : m_unique_fs(std::move(fs)), m_base_fs(m_unique_fs.get()) { /* ... */ }
            DirectorySaveDataFileSystem(fs::fsa::IFileSystem *fs) : m_unique_fs(), m_base_fs(fs) { /* ... */ }
            Result Initialize(bool journaling_supported, bool multi_commit_enabled, bool journaling_enabled);
        private:
            Result SynchronizeDirectory(const fs::Path &dst, const fs::Path &src);
            Result ResolvePath(fs::Path *out, const fs::Path &path);
            Result AcquireLockFile();
        public:
            void DecrementWriteOpenFileCount();
        public:
            virtual Result DoCreateFile(const fs::Path &path, s64 size, int option) override;
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

            /* These aren't accessible as commands. */
            virtual Result DoCommitProvisionally(s64 counter) override;
            virtual Result DoRollback() override;
    };

}
