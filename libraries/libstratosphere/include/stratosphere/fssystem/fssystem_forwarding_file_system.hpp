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
#include <stratosphere/fs/fs_query_range.hpp>

namespace ams::fssystem {

    /* ACCURATE_TO_VERSION: 13.4.0.0 */
    class ForwardingFile final : public ::ams::fs::fsa::IFile, public ::ams::fs::impl::Newable {
        NON_COPYABLE(ForwardingFile);
        NON_MOVEABLE(ForwardingFile);
        private:
            std::unique_ptr<fs::fsa::IFile> m_base_file;
        public:
            ForwardingFile(std::unique_ptr<fs::fsa::IFile> f) : m_base_file(std::move(f)) { /* ... */ }

            virtual ~ForwardingFile() { /* ... */ }
        public:
            virtual Result DoRead(size_t *out, s64 offset, void *buffer, size_t size, const fs::ReadOption &option) override final {
                R_RETURN(m_base_file->Read(out, offset, buffer, size, option));
            }

            virtual Result DoGetSize(s64 *out) override final {
                R_RETURN(m_base_file->GetSize(out));
            }

            virtual Result DoFlush() override final {
                R_RETURN(m_base_file->Flush());
            }

            virtual Result DoWrite(s64 offset, const void *buffer, size_t size, const fs::WriteOption &option) override final {
                R_RETURN(m_base_file->Write(offset, buffer, size, option));
            }

            virtual Result DoSetSize(s64 size) override final {
                R_RETURN(m_base_file->SetSize(size));
            }

            virtual Result DoOperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override final {
                R_RETURN(m_base_file->OperateRange(dst, dst_size, op_id, offset, size, src, src_size));
            }
        public:
            virtual sf::cmif::DomainObjectId GetDomainObjectId() const override final {
                return m_base_file->GetDomainObjectId();
            }
    };

    /* ACCURATE_TO_VERSION: 13.4.0.0 */
    class ForwardingDirectory final : public ::ams::fs::fsa::IDirectory, public ::ams::fs::impl::Newable {
        NON_COPYABLE(ForwardingDirectory);
        NON_MOVEABLE(ForwardingDirectory);
        private:
            std::unique_ptr<fs::fsa::IDirectory> m_base_dir;
        public:
            ForwardingDirectory(std::unique_ptr<fs::fsa::IDirectory> d) : m_base_dir(std::move(d)) { /* ... */ }

            virtual ~ForwardingDirectory() { /* ... */ }
        public:
            virtual Result DoRead(s64 *out_count, fs::DirectoryEntry *out_entries, s64 max_entries) override final {
                R_RETURN(m_base_dir->Read(out_count, out_entries, max_entries));
            }

            virtual Result DoGetEntryCount(s64 *out) override final {
                R_RETURN(m_base_dir->GetEntryCount(out));
            }
        public:
            virtual sf::cmif::DomainObjectId GetDomainObjectId() const override final {
                return m_base_dir->GetDomainObjectId();
            }
    };

    /* ACCURATE_TO_VERSION: 13.4.0.0 */
    class ForwardingFileSystem final : public ::ams::fs::fsa::IFileSystem, public ::ams::fs::impl::Newable {
        NON_COPYABLE(ForwardingFileSystem);
        NON_MOVEABLE(ForwardingFileSystem);
        private:
            std::shared_ptr<fs::fsa::IFileSystem> m_base_fs;
        public:
            ForwardingFileSystem(std::shared_ptr<fs::fsa::IFileSystem> fs) : m_base_fs(std::move(fs)) { /* ... */ }

            virtual ~ForwardingFileSystem() { /* ... */ }
        public:
            virtual Result DoCreateFile(const fs::Path &path, s64 size, int flags) override final {
                R_RETURN(m_base_fs->CreateFile(path, size, flags));
            }

            virtual Result DoDeleteFile(const fs::Path &path) override final {
                R_RETURN(m_base_fs->DeleteFile(path));
            }

            virtual Result DoCreateDirectory(const fs::Path &path) override final {
                R_RETURN(m_base_fs->CreateDirectory(path));
            }

            virtual Result DoDeleteDirectory(const fs::Path &path) override final {
                R_RETURN(m_base_fs->DeleteDirectory(path));
            }

            virtual Result DoDeleteDirectoryRecursively(const fs::Path &path) override final {
                R_RETURN(m_base_fs->DeleteDirectoryRecursively(path));
            }

            virtual Result DoRenameFile(const fs::Path &old_path, const fs::Path &new_path) override final {
                R_RETURN(m_base_fs->RenameFile(old_path, new_path));
            }

            virtual Result DoRenameDirectory(const fs::Path &old_path, const fs::Path &new_path) override final {
                R_RETURN(m_base_fs->RenameDirectory(old_path, new_path));
            }

            virtual Result DoGetEntryType(fs::DirectoryEntryType *out, const fs::Path &path) override final {
                R_RETURN(m_base_fs->GetEntryType(out, path));
            }

            virtual Result DoOpenFile(std::unique_ptr<fs::fsa::IFile> *out_file, const fs::Path &path, fs::OpenMode mode) override final {
                R_RETURN(m_base_fs->OpenFile(out_file, path, mode));
            }

            virtual Result DoOpenDirectory(std::unique_ptr<fs::fsa::IDirectory> *out_dir, const fs::Path &path, fs::OpenDirectoryMode mode) override final {
                R_RETURN(m_base_fs->OpenDirectory(out_dir, path, mode));
            }

            virtual Result DoCommit() override final {
                R_RETURN(m_base_fs->Commit());
            }

            virtual Result DoGetFreeSpaceSize(s64 *out, const fs::Path &path) override final {
                R_RETURN(m_base_fs->GetFreeSpaceSize(out, path));
            }

            virtual Result DoGetTotalSpaceSize(s64 *out, const fs::Path &path) override final {
                R_RETURN(m_base_fs->GetTotalSpaceSize(out, path));
            }

            virtual Result DoCleanDirectoryRecursively(const fs::Path &path) override final {
                R_RETURN(m_base_fs->CleanDirectoryRecursively(path));
            }

            virtual Result DoGetFileTimeStampRaw(fs::FileTimeStampRaw *out, const fs::Path &path) override final {
                R_RETURN(m_base_fs->GetFileTimeStampRaw(out, path));
            }

            virtual Result DoQueryEntry(char *dst, size_t dst_size, const char *src, size_t src_size, fs::fsa::QueryId query, const fs::Path &path) override final {
                R_RETURN(m_base_fs->QueryEntry(dst, dst_size, src, src_size, query, path));
            }

            virtual Result DoCommitProvisionally(s64 counter) override final {
                R_RETURN(m_base_fs->CommitProvisionally(counter));
            }

            virtual Result DoRollback() override final {
                R_RETURN(m_base_fs->Rollback());
            }

            virtual Result DoFlush() override final {
                R_RETURN(m_base_fs->Flush());
            }
    };

}
