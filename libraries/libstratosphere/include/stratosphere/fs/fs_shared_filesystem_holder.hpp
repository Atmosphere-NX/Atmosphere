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
#include <stratosphere/fs/impl/fs_newable.hpp>
#include <stratosphere/fs/fsa/fs_ifile.hpp>
#include <stratosphere/fs/fsa/fs_idirectory.hpp>
#include <stratosphere/fs/fsa/fs_ifilesystem.hpp>

namespace ams::fs {

    class SharedFileSystemHolder : public fsa::IFileSystem, public impl::Newable {
        NON_COPYABLE(SharedFileSystemHolder);
        NON_MOVEABLE(SharedFileSystemHolder);
        private:
            std::shared_ptr<fsa::IFileSystem> m_fs;
        public:
            SharedFileSystemHolder(std::shared_ptr<fsa::IFileSystem> f) : m_fs(std::move(f)) { /* ... */ }
        public:
            virtual Result DoCreateFile(const char *path, s64 size, int flags) override { return m_fs->CreateFile(path, size, flags); }
            virtual Result DoDeleteFile(const char *path) override { return m_fs->DeleteFile(path); }
            virtual Result DoCreateDirectory(const char *path) override { return m_fs->CreateDirectory(path); }
            virtual Result DoDeleteDirectory(const char *path) override { return m_fs->DeleteDirectory(path); }
            virtual Result DoDeleteDirectoryRecursively(const char *path) override { return m_fs->DeleteDirectoryRecursively(path); }
            virtual Result DoRenameFile(const char *old_path, const char *new_path) override { return m_fs->RenameFile(old_path, new_path); }
            virtual Result DoRenameDirectory(const char *old_path, const char *new_path) override { return m_fs->RenameDirectory(old_path, new_path); }
            virtual Result DoGetEntryType(fs::DirectoryEntryType *out, const char *path) override { return m_fs->GetEntryType(out, path); }
            virtual Result DoOpenFile(std::unique_ptr<fs::fsa::IFile> *out_file, const char *path, fs::OpenMode mode) override { return m_fs->OpenFile(out_file, path, mode); }
            virtual Result DoOpenDirectory(std::unique_ptr<fs::fsa::IDirectory> *out_dir, const char *path, fs::OpenDirectoryMode mode) override { return m_fs->OpenDirectory(out_dir, path, mode); }
            virtual Result DoCommit() override { return m_fs->Commit(); }
            virtual Result DoGetFreeSpaceSize(s64 *out, const char *path) override { return m_fs->GetFreeSpaceSize(out, path); }
            virtual Result DoGetTotalSpaceSize(s64 *out, const char *path) override { return m_fs->GetTotalSpaceSize(out, path); }
            virtual Result DoCleanDirectoryRecursively(const char *path) override { return m_fs->CleanDirectoryRecursively(path); }

            /* These aren't accessible as commands. */
            virtual Result DoCommitProvisionally(s64 counter) override { return m_fs->CommitProvisionally(counter); }
            virtual Result DoRollback() override { return m_fs->Rollback(); }
            virtual Result DoFlush() override { return m_fs->Flush(); }
    };

}
