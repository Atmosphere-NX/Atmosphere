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
#include <stratosphere/fs/fs_common.hpp>
#include <stratosphere/fs/fsa/fs_ifile.hpp>
#include <stratosphere/fs/fsa/fs_idirectory.hpp>
#include <stratosphere/fs/fsa/fs_ifilesystem.hpp>
#include <stratosphere/fs/impl/fs_newable.hpp>

namespace ams::fssystem::impl {

    template<typename Impl>
    class IPathResolutionFileSystem : public fs::fsa::IFileSystem, public fs::impl::Newable {
        NON_COPYABLE(IPathResolutionFileSystem);
        private:
            std::shared_ptr<fs::fsa::IFileSystem> shared_fs;
            std::unique_ptr<fs::fsa::IFileSystem> unique_fs;
            bool unc_preserved;
        protected:
            fs::fsa::IFileSystem * const base_fs;
        public:
            IPathResolutionFileSystem(std::shared_ptr<fs::fsa::IFileSystem> fs, bool unc = false) : shared_fs(std::move(fs)), unc_preserved(unc), base_fs(shared_fs.get())  {
                /* ... */
            }

            IPathResolutionFileSystem(std::unique_ptr<fs::fsa::IFileSystem> fs, bool unc = false) : unique_fs(std::move(fs)), unc_preserved(unc), base_fs(unique_fs.get())  {
                /* ... */
            }

            virtual ~IPathResolutionFileSystem() { /* ... */ }
        protected:
            constexpr inline bool IsUncPreserved() const {
                return this->unc_preserved;
            }
        public:
            virtual Result DoCreateFile(const char *path, s64 size, int option) override {
                char full_path[fs::EntryNameLengthMax + 1];
                R_TRY(static_cast<Impl*>(this)->ResolveFullPath(full_path, sizeof(full_path), path));

                std::optional optional_lock = static_cast<Impl*>(this)->GetAccessorLock();
                return this->base_fs->CreateFile(full_path, size, option);
            }

            virtual Result DoDeleteFile(const char *path) override {
                char full_path[fs::EntryNameLengthMax + 1];
                R_TRY(static_cast<Impl*>(this)->ResolveFullPath(full_path, sizeof(full_path), path));

                std::optional optional_lock = static_cast<Impl*>(this)->GetAccessorLock();
                return this->base_fs->DeleteFile(full_path);
            }

            virtual Result DoCreateDirectory(const char *path) override {
                char full_path[fs::EntryNameLengthMax + 1];
                R_TRY(static_cast<Impl*>(this)->ResolveFullPath(full_path, sizeof(full_path), path));

                std::optional optional_lock = static_cast<Impl*>(this)->GetAccessorLock();
                return this->base_fs->CreateDirectory(full_path);
            }

            virtual Result DoDeleteDirectory(const char *path) override {
                char full_path[fs::EntryNameLengthMax + 1];
                R_TRY(static_cast<Impl*>(this)->ResolveFullPath(full_path, sizeof(full_path), path));

                std::optional optional_lock = static_cast<Impl*>(this)->GetAccessorLock();
                return this->base_fs->DeleteDirectory(full_path);
            }

            virtual Result DoDeleteDirectoryRecursively(const char *path) override {
                char full_path[fs::EntryNameLengthMax + 1];
                R_TRY(static_cast<Impl*>(this)->ResolveFullPath(full_path, sizeof(full_path), path));

                std::optional optional_lock = static_cast<Impl*>(this)->GetAccessorLock();
                return this->base_fs->DeleteDirectoryRecursively(full_path);
            }

            virtual Result DoRenameFile(const char *old_path, const char *new_path) override {
                char old_full_path[fs::EntryNameLengthMax + 1];
                char new_full_path[fs::EntryNameLengthMax + 1];
                R_TRY(static_cast<Impl*>(this)->ResolveFullPath(old_full_path, sizeof(old_full_path), old_path));
                R_TRY(static_cast<Impl*>(this)->ResolveFullPath(new_full_path, sizeof(new_full_path), new_path));

                std::optional optional_lock = static_cast<Impl*>(this)->GetAccessorLock();
                return this->base_fs->RenameFile(old_full_path, new_full_path);
            }

            virtual Result DoRenameDirectory(const char *old_path, const char *new_path) override {
                char old_full_path[fs::EntryNameLengthMax + 1];
                char new_full_path[fs::EntryNameLengthMax + 1];
                R_TRY(static_cast<Impl*>(this)->ResolveFullPath(old_full_path, sizeof(old_full_path), old_path));
                R_TRY(static_cast<Impl*>(this)->ResolveFullPath(new_full_path, sizeof(new_full_path), new_path));

                std::optional optional_lock = static_cast<Impl*>(this)->GetAccessorLock();
                return this->base_fs->RenameDirectory(old_full_path, new_full_path);
            }

            virtual Result DoGetEntryType(fs::DirectoryEntryType *out, const char *path) override {
                char full_path[fs::EntryNameLengthMax + 1];
                R_TRY(static_cast<Impl*>(this)->ResolveFullPath(full_path, sizeof(full_path), path));

                std::optional optional_lock = static_cast<Impl*>(this)->GetAccessorLock();
                return this->base_fs->GetEntryType(out, full_path);
            }

            virtual Result DoOpenFile(std::unique_ptr<fs::fsa::IFile> *out_file, const char *path, fs::OpenMode mode) override {
                char full_path[fs::EntryNameLengthMax + 1];
                R_TRY(static_cast<Impl*>(this)->ResolveFullPath(full_path, sizeof(full_path), path));

                std::optional optional_lock = static_cast<Impl*>(this)->GetAccessorLock();
                return this->base_fs->OpenFile(out_file, full_path, mode);
            }

            virtual Result DoOpenDirectory(std::unique_ptr<fs::fsa::IDirectory> *out_dir, const char *path, fs::OpenDirectoryMode mode) override {
                char full_path[fs::EntryNameLengthMax + 1];
                R_TRY(static_cast<Impl*>(this)->ResolveFullPath(full_path, sizeof(full_path), path));

                std::optional optional_lock = static_cast<Impl*>(this)->GetAccessorLock();
                return this->base_fs->OpenDirectory(out_dir, full_path, mode);
            }

            virtual Result DoCommit() override {
                std::optional optional_lock = static_cast<Impl*>(this)->GetAccessorLock();
                return this->base_fs->Commit();
            }

            virtual Result DoGetFreeSpaceSize(s64 *out, const char *path) override {
                char full_path[fs::EntryNameLengthMax + 1];
                R_TRY(static_cast<Impl*>(this)->ResolveFullPath(full_path, sizeof(full_path), path));

                std::optional optional_lock = static_cast<Impl*>(this)->GetAccessorLock();
                return this->base_fs->GetFreeSpaceSize(out, full_path);
            }

            virtual Result DoGetTotalSpaceSize(s64 *out, const char *path) override {
                char full_path[fs::EntryNameLengthMax + 1];
                R_TRY(static_cast<Impl*>(this)->ResolveFullPath(full_path, sizeof(full_path), path));

                std::optional optional_lock = static_cast<Impl*>(this)->GetAccessorLock();
                return this->base_fs->GetTotalSpaceSize(out, full_path);
            }

            virtual Result DoCleanDirectoryRecursively(const char *path) override {
                char full_path[fs::EntryNameLengthMax + 1];
                R_TRY(static_cast<Impl*>(this)->ResolveFullPath(full_path, sizeof(full_path), path));

                std::optional optional_lock = static_cast<Impl*>(this)->GetAccessorLock();
                return this->base_fs->CleanDirectoryRecursively(full_path);
            }

            virtual Result DoGetFileTimeStampRaw(fs::FileTimeStampRaw *out, const char *path) override {
                char full_path[fs::EntryNameLengthMax + 1];
                R_TRY(static_cast<Impl*>(this)->ResolveFullPath(full_path, sizeof(full_path), path));

                std::optional optional_lock = static_cast<Impl*>(this)->GetAccessorLock();
                return this->base_fs->GetFileTimeStampRaw(out, full_path);
            }

            virtual Result DoQueryEntry(char *dst, size_t dst_size, const char *src, size_t src_size, fs::fsa::QueryId query, const char *path) override {
                char full_path[fs::EntryNameLengthMax + 1];
                R_TRY(static_cast<Impl*>(this)->ResolveFullPath(full_path, sizeof(full_path), path));

                std::optional optional_lock = static_cast<Impl*>(this)->GetAccessorLock();
                return this->base_fs->QueryEntry(dst, dst_size, src, src_size, query, full_path);
            }

            /* These aren't accessible as commands. */
            virtual Result DoCommitProvisionally(s64 counter) override {
                std::optional optional_lock = static_cast<Impl*>(this)->GetAccessorLock();
                return this->base_fs->CommitProvisionally(counter);
            }

            virtual Result DoRollback() override {
                std::optional optional_lock = static_cast<Impl*>(this)->GetAccessorLock();
                return this->base_fs->Rollback();
            }

            virtual Result DoFlush() override {
                std::optional optional_lock = static_cast<Impl*>(this)->GetAccessorLock();
                return this->base_fs->Flush();
            }
    };

}
