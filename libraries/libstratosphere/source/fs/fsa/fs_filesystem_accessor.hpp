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
#include <stratosphere.hpp>
#include <stratosphere/fssrv/fssrv_interface_adapters.hpp>
#include "fs_mount_name.hpp"

namespace ams::fs::impl {

    class FileAccessor;
    class DirectoryAccessor;

    class FileSystemAccessor : public util::IntrusiveListBaseNode<FileSystemAccessor>,  public Newable {
        NON_COPYABLE(FileSystemAccessor);
        friend class FileAccessor;
        friend class DirectoryAccessor;
        private:
            using FileList = util::IntrusiveListBaseTraits<FileAccessor>::ListType;
            using DirList  = util::IntrusiveListBaseTraits<DirectoryAccessor>::ListType;
        private:
            MountName name;
            std::unique_ptr<fsa::IFileSystem> impl;
            FileList open_file_list;
            DirList open_dir_list;
            os::Mutex open_list_lock;
            std::unique_ptr<fsa::ICommonMountNameGenerator> mount_name_generator;
            bool access_log_enabled;
            bool data_cache_attachable;
            bool path_cache_attachable;
            bool path_cache_attached;
            bool multi_commit_supported;
        public:
            FileSystemAccessor(const char *name, std::unique_ptr<fsa::IFileSystem> &&fs, std::unique_ptr<fsa::ICommonMountNameGenerator> &&generator = nullptr);
            virtual ~FileSystemAccessor();

            Result CreateFile(const char *path, s64 size, int option);
            Result DeleteFile(const char *path);
            Result CreateDirectory(const char *path);
            Result DeleteDirectory(const char *path);
            Result DeleteDirectoryRecursively(const char *path);
            Result RenameFile(const char *old_path, const char *new_path);
            Result RenameDirectory(const char *old_path, const char *new_path);
            Result GetEntryType(DirectoryEntryType *out, const char *path);
            Result OpenFile(std::unique_ptr<FileAccessor> *out_file, const char *path, OpenMode mode);
            Result OpenDirectory(std::unique_ptr<DirectoryAccessor> *out_dir, const char *path, OpenDirectoryMode mode);
            Result Commit();
            Result GetFreeSpaceSize(s64 *out, const char *path);
            Result GetTotalSpaceSize(s64 *out, const char *path);
            Result CleanDirectoryRecursively(const char *path);
            Result GetFileTimeStampRaw(FileTimeStampRaw *out, const char *path);
            Result QueryEntry(char *dst, size_t dst_size, const char *src, size_t src_size, fsa::QueryId query, const char *path);

            const char *GetName() const { return this->name.str; }
            Result GetCommonMountName(char *dst, size_t dst_size) const;

            void SetAccessLogEnabled(bool en) { this->access_log_enabled = en; }
            void SetFileDataCacheAttachable(bool en) { this->data_cache_attachable = en; }
            void SetPathBasedFileDataCacheAttachable(bool en) { this->path_cache_attachable = en; }
            void SetMultiCommitSupported(bool en) { this->multi_commit_supported = en; }

            bool IsEnabledAccessLog() const { return this->access_log_enabled; }
            bool IsFileDataCacheAttachable() const { return this->data_cache_attachable; }
            bool IsPathBasedFileDataCacheAttachable() const { return this->path_cache_attachable; }

            void AttachPathBasedFileDataCache() {
                if (this->IsPathBasedFileDataCacheAttachable()) {
                    this->path_cache_attached = true;
                }
            }

            void DetachPathBasedFileDataCache() {
                this->path_cache_attached = false;
            }

            std::shared_ptr<fssrv::impl::FileSystemInterfaceAdapter> GetMultiCommitTarget();

            fsa::IFileSystem *GetRawFileSystemUnsafe() {
                return this->impl.get();
            }
        private:
            void NotifyCloseFile(FileAccessor *f);
            void NotifyCloseDirectory(DirectoryAccessor *d);
    };

}
