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

    class DirectoryRedirectionFileSystem : public fs::fsa::IFileSystem, public fs::impl::Newable {
        NON_COPYABLE(DirectoryRedirectionFileSystem);
        private:
            std::unique_ptr<fs::fsa::IFileSystem> m_base_fs;
            fs::Path m_before_dir;
            fs::Path m_after_dir;
        public:
            DirectoryRedirectionFileSystem(std::unique_ptr<fs::fsa::IFileSystem> fs) : m_base_fs(std::move(fs)), m_before_dir(), m_after_dir() {
                /* ... */
            }

            Result InitializeWithFixedPath(const char *before, const char *after) {
                R_TRY(fs::SetUpFixedPath(std::addressof(m_before_dir), before));
                R_TRY(fs::SetUpFixedPath(std::addressof(m_after_dir), after));
                R_SUCCEED();
            }
        private:
            Result ResolveFullPath(fs::Path *out, const fs::Path &path) {
                if (path.IsMatchHead(m_before_dir.GetString(), m_before_dir.GetLength())) {
                    R_TRY(out->Initialize(m_after_dir));
                    R_TRY(out->AppendChild(path.GetString() + m_before_dir.GetLength()));
                } else {
                    R_TRY(out->Initialize(path));
                }
                R_SUCCEED();
            }
        public:
            virtual Result DoCreateFile(const fs::Path &path, s64 size, int option) override {
                fs::Path full_path;
                R_TRY(this->ResolveFullPath(std::addressof(full_path), path));

                R_RETURN(m_base_fs->CreateFile(full_path, size, option));
            }

            virtual Result DoDeleteFile(const fs::Path &path) override {
                fs::Path full_path;
                R_TRY(this->ResolveFullPath(std::addressof(full_path), path));

                R_RETURN(m_base_fs->DeleteFile(full_path));
            }

            virtual Result DoCreateDirectory(const fs::Path &path) override {
                fs::Path full_path;
                R_TRY(this->ResolveFullPath(std::addressof(full_path), path));

                R_RETURN(m_base_fs->CreateDirectory(full_path));
            }

            virtual Result DoDeleteDirectory(const fs::Path &path) override {
                fs::Path full_path;
                R_TRY(this->ResolveFullPath(std::addressof(full_path), path));

                R_RETURN(m_base_fs->DeleteDirectory(full_path));
            }

            virtual Result DoDeleteDirectoryRecursively(const fs::Path &path) override {
                fs::Path full_path;
                R_TRY(this->ResolveFullPath(std::addressof(full_path), path));

                R_RETURN(m_base_fs->DeleteDirectoryRecursively(full_path));
            }

            virtual Result DoRenameFile(const fs::Path &old_path, const fs::Path &new_path) override {
                fs::Path old_full_path;
                fs::Path new_full_path;
                R_TRY(this->ResolveFullPath(std::addressof(old_full_path), old_path));
                R_TRY(this->ResolveFullPath(std::addressof(new_full_path), new_path));

                R_RETURN(m_base_fs->RenameFile(old_full_path, new_full_path));
            }

            virtual Result DoRenameDirectory(const fs::Path &old_path, const fs::Path &new_path) override {
                fs::Path old_full_path;
                fs::Path new_full_path;
                R_TRY(this->ResolveFullPath(std::addressof(old_full_path), old_path));
                R_TRY(this->ResolveFullPath(std::addressof(new_full_path), new_path));

                R_RETURN(m_base_fs->RenameDirectory(old_full_path, new_full_path));
            }

            virtual Result DoGetEntryType(fs::DirectoryEntryType *out, const fs::Path &path) override {
                fs::Path full_path;
                R_TRY(this->ResolveFullPath(std::addressof(full_path), path));

                R_RETURN(m_base_fs->GetEntryType(out, full_path));
            }

            virtual Result DoOpenFile(std::unique_ptr<fs::fsa::IFile> *out_file, const fs::Path &path, fs::OpenMode mode) override {
                fs::Path full_path;
                R_TRY(this->ResolveFullPath(std::addressof(full_path), path));

                R_RETURN(m_base_fs->OpenFile(out_file, full_path, mode));
            }

            virtual Result DoOpenDirectory(std::unique_ptr<fs::fsa::IDirectory> *out_dir, const fs::Path &path, fs::OpenDirectoryMode mode) override {
                fs::Path full_path;
                R_TRY(this->ResolveFullPath(std::addressof(full_path), path));

                R_RETURN(m_base_fs->OpenDirectory(out_dir, full_path, mode));
            }

            virtual Result DoCommit() override {
                R_RETURN(m_base_fs->Commit());
            }

            virtual Result DoGetFreeSpaceSize(s64 *out, const fs::Path &path) override {
                fs::Path full_path;
                R_TRY(this->ResolveFullPath(std::addressof(full_path), path));

                R_RETURN(m_base_fs->GetFreeSpaceSize(out, full_path));
            }

            virtual Result DoGetTotalSpaceSize(s64 *out, const fs::Path &path) override {
                fs::Path full_path;
                R_TRY(this->ResolveFullPath(std::addressof(full_path), path));

                R_RETURN(m_base_fs->GetTotalSpaceSize(out, full_path));
            }

            virtual Result DoCleanDirectoryRecursively(const fs::Path &path) override {
                fs::Path full_path;
                R_TRY(this->ResolveFullPath(std::addressof(full_path), path));

                R_RETURN(m_base_fs->CleanDirectoryRecursively(full_path));
            }

            virtual Result DoGetFileTimeStampRaw(fs::FileTimeStampRaw *out, const fs::Path &path) override {
                fs::Path full_path;
                R_TRY(this->ResolveFullPath(std::addressof(full_path), path));

                R_RETURN(m_base_fs->GetFileTimeStampRaw(out, full_path));
            }

            virtual Result DoQueryEntry(char *dst, size_t dst_size, const char *src, size_t src_size, fs::fsa::QueryId query, const fs::Path &path) override {
                fs::Path full_path;
                R_TRY(this->ResolveFullPath(std::addressof(full_path), path));

                R_RETURN(m_base_fs->QueryEntry(dst, dst_size, src, src_size, query, full_path));
            }

            /* These aren't accessible as commands. */
            virtual Result DoCommitProvisionally(s64 counter) override {
                R_RETURN(m_base_fs->CommitProvisionally(counter));
            }

            virtual Result DoRollback() override {
                R_RETURN(m_base_fs->Rollback());
            }

            virtual Result DoFlush() override {
                R_RETURN(m_base_fs->Flush());
            }
    };

}
