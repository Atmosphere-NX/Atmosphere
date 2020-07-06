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
#include <stratosphere/fs/fs_file.hpp>
#include <stratosphere/fs/fs_directory.hpp>
#include <stratosphere/fs/fs_filesystem.hpp>
#include <stratosphere/fs/fs_query_range.hpp>
#include <stratosphere/fssystem/fssystem_utility.hpp>
#include <stratosphere/fssrv/sf/fssrv_sf_path.hpp>
#include <stratosphere/fssrv/sf/fssrv_sf_ifile.hpp>
#include <stratosphere/fssrv/sf/fssrv_sf_idirectory.hpp>
#include <stratosphere/fssrv/sf/fssrv_sf_ifilesystem.hpp>

namespace ams::fs::fsa {

    class IFile;
    class IDirectory;
    class IFileSystem;

}

namespace ams::fssrv::impl {

    class FileSystemInterfaceAdapter;

    class FileInterfaceAdapter final {
        NON_COPYABLE(FileInterfaceAdapter);
        private:
            std::shared_ptr<FileSystemInterfaceAdapter> parent_filesystem;
            std::unique_ptr<fs::fsa::IFile> base_file;
            std::unique_lock<fssystem::SemaphoreAdapter> open_count_semaphore;
        public:
            FileInterfaceAdapter(std::unique_ptr<fs::fsa::IFile> &&file, std::shared_ptr<FileSystemInterfaceAdapter> &&parent, std::unique_lock<fssystem::SemaphoreAdapter> &&sema);
            ~FileInterfaceAdapter();
        private:
            void InvalidateCache();
        public:
            /* Command API. */
            Result Read(ams::sf::Out<s64> out, s64 offset, const ams::sf::OutNonSecureBuffer &buffer, s64 size, fs::ReadOption option);
            Result Write(s64 offset, const ams::sf::InNonSecureBuffer &buffer, s64 size, fs::WriteOption option);
            Result Flush();
            Result SetSize(s64 size);
            Result GetSize(ams::sf::Out<s64> out);
            Result OperateRange(ams::sf::Out<fs::FileQueryRangeInfo> out, s32 op_id, s64 offset, s64 size);
    };
    static_assert(fssrv::sf::IsIFile<FileInterfaceAdapter>);

    class DirectoryInterfaceAdapter final {
        NON_COPYABLE(DirectoryInterfaceAdapter);
        private:
            std::shared_ptr<FileSystemInterfaceAdapter> parent_filesystem;
            std::unique_ptr<fs::fsa::IDirectory> base_dir;
            std::unique_lock<fssystem::SemaphoreAdapter> open_count_semaphore;
        public:
            DirectoryInterfaceAdapter(std::unique_ptr<fs::fsa::IDirectory> &&dir, std::shared_ptr<FileSystemInterfaceAdapter> &&parent, std::unique_lock<fssystem::SemaphoreAdapter> &&sema);
            ~DirectoryInterfaceAdapter();
        public:
            /* Command API */
            Result Read(ams::sf::Out<s64> out, const ams::sf::OutBuffer &out_entries);
            Result GetEntryCount(ams::sf::Out<s64> out);
    };
    static_assert(fssrv::sf::IsIDirectory<DirectoryInterfaceAdapter>);

    class FileSystemInterfaceAdapter final : public std::enable_shared_from_this<FileSystemInterfaceAdapter> {
        NON_COPYABLE(FileSystemInterfaceAdapter);
        private:
            std::shared_ptr<fs::fsa::IFileSystem> base_fs;
            std::unique_lock<fssystem::SemaphoreAdapter> mount_count_semaphore;
            os::ReadWriteLock invalidation_lock;
            bool open_count_limited;
            bool deep_retry_enabled = false;
        public:
            FileSystemInterfaceAdapter(std::shared_ptr<fs::fsa::IFileSystem> &&fs, bool open_limited);
            /* TODO: Other constructors. */

            ~FileSystemInterfaceAdapter();
        public:
            bool IsDeepRetryEnabled() const;
            bool IsAccessFailureDetectionObserved() const;
            std::optional<std::shared_lock<os::ReadWriteLock>> AcquireCacheInvalidationReadLock();
            os::ReadWriteLock &GetReadWriteLockForCacheInvalidation();
        public:
            /* Command API. */
            Result CreateFile(const fssrv::sf::Path &path, s64 size, s32 option);
            Result DeleteFile(const fssrv::sf::Path &path);
            Result CreateDirectory(const fssrv::sf::Path &path);
            Result DeleteDirectory(const fssrv::sf::Path &path);
            Result DeleteDirectoryRecursively(const fssrv::sf::Path &path);
            Result RenameFile(const fssrv::sf::Path &old_path, const fssrv::sf::Path &new_path);
            Result RenameDirectory(const fssrv::sf::Path &old_path, const fssrv::sf::Path &new_path);
            Result GetEntryType(ams::sf::Out<u32> out, const fssrv::sf::Path &path);
            Result OpenFile(ams::sf::Out<std::shared_ptr<fssrv::sf::IFile>> out, const fssrv::sf::Path &path, u32 mode);
            Result OpenDirectory(ams::sf::Out<std::shared_ptr<fssrv::sf::IDirectory>> out, const fssrv::sf::Path &path, u32 mode);
            Result Commit();
            Result GetFreeSpaceSize(ams::sf::Out<s64> out, const fssrv::sf::Path &path);
            Result GetTotalSpaceSize(ams::sf::Out<s64> out, const fssrv::sf::Path &path);

            Result CleanDirectoryRecursively(const fssrv::sf::Path &path);
            Result GetFileTimeStampRaw(ams::sf::Out<fs::FileTimeStampRaw> out, const fssrv::sf::Path &path);

            Result QueryEntry(const ams::sf::OutBuffer &out_buf, const ams::sf::InBuffer &in_buf, s32 query_id, const fssrv::sf::Path &path);
    };

}
