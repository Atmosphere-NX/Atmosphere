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

    /* ACCURATE_TO_VERSION: 13.4.0.0 */
    class FileInterfaceAdapter {
        NON_COPYABLE(FileInterfaceAdapter);
        NON_MOVEABLE(FileInterfaceAdapter);
        private:
            ams::sf::SharedPointer<FileSystemInterfaceAdapter> m_parent_filesystem;
            std::unique_ptr<fs::fsa::IFile> m_base_file;
            bool m_allow_all_operations;
        public:
            FileInterfaceAdapter(std::unique_ptr<fs::fsa::IFile> &&file, FileSystemInterfaceAdapter *parent, bool allow_all);
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
            Result OperateRangeWithBuffer(const ams::sf::OutNonSecureBuffer &out_buf, const ams::sf::InNonSecureBuffer &in_buf, s32 op_id, s64 offset, s64 size);
    };
    static_assert(fssrv::sf::IsIFile<FileInterfaceAdapter>);

    /* ACCURATE_TO_VERSION: 13.4.0.0 */
    class DirectoryInterfaceAdapter {
        NON_COPYABLE(DirectoryInterfaceAdapter);
        NON_MOVEABLE(DirectoryInterfaceAdapter);
        private:
            ams::sf::SharedPointer<FileSystemInterfaceAdapter> m_parent_filesystem;
            std::unique_ptr<fs::fsa::IDirectory> m_base_dir;
            bool m_allow_all_operations;
        public:
            DirectoryInterfaceAdapter(std::unique_ptr<fs::fsa::IDirectory> &&dir, FileSystemInterfaceAdapter *parent, bool allow_all);
        public:
            /* Command API */
            Result Read(ams::sf::Out<s64> out, const ams::sf::OutBuffer &out_entries);
            Result GetEntryCount(ams::sf::Out<s64> out);
    };
    static_assert(fssrv::sf::IsIDirectory<DirectoryInterfaceAdapter>);

    /* ACCURATE_TO_VERSION: 13.4.0.0 */
    class FileSystemInterfaceAdapter : public ams::sf::ISharedObject {
        NON_COPYABLE(FileSystemInterfaceAdapter);
        NON_MOVEABLE(FileSystemInterfaceAdapter);
        private:
            std::shared_ptr<fs::fsa::IFileSystem> m_base_fs;
            fs::PathFlags m_path_flags;
            bool m_allow_all_operations;
            bool m_is_mitm_interface;
        public:
            FileSystemInterfaceAdapter(std::shared_ptr<fs::fsa::IFileSystem> &&fs, const fs::PathFlags &flags, bool allow_all, bool is_mitm_interface = false)
                : m_base_fs(std::move(fs)), m_path_flags(flags), m_allow_all_operations(allow_all), m_is_mitm_interface(is_mitm_interface)
            {
                /* ... */
            }

            FileSystemInterfaceAdapter(std::shared_ptr<fs::fsa::IFileSystem> &&fs, bool allow_all, bool is_mitm_interface = false)
                : m_base_fs(std::move(fs)), m_path_flags(), m_allow_all_operations(allow_all), m_is_mitm_interface(is_mitm_interface)
            {
                /* ... */
            }
        private:
            Result SetUpPath(fs::Path *out, const fssrv::sf::Path &sf_path);
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
            Result OpenFile(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFile>> out, const fssrv::sf::Path &path, u32 mode);
            Result OpenDirectory(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IDirectory>> out, const fssrv::sf::Path &path, u32 mode);
            Result Commit();
            Result GetFreeSpaceSize(ams::sf::Out<s64> out, const fssrv::sf::Path &path);
            Result GetTotalSpaceSize(ams::sf::Out<s64> out, const fssrv::sf::Path &path);

            Result CleanDirectoryRecursively(const fssrv::sf::Path &path);
            Result GetFileTimeStampRaw(ams::sf::Out<fs::FileTimeStampRaw> out, const fssrv::sf::Path &path);

            Result QueryEntry(const ams::sf::OutNonSecureBuffer &out_buf, const ams::sf::InNonSecureBuffer &in_buf, s32 query_id, const fssrv::sf::Path &path);
    };

    #if defined(ATMOSPHERE_OS_HORIZON)
    class RemoteFile {
        NON_COPYABLE(RemoteFile);
        NON_MOVEABLE(RemoteFile);
        private:
            ::FsFile m_base_file;
        public:
            RemoteFile(::FsFile &s) : m_base_file(s) { /* ... */}

            virtual ~RemoteFile() { fsFileClose(std::addressof(m_base_file)); }
        public:
            Result Read(ams::sf::Out<s64> out, s64 offset, const ams::sf::OutNonSecureBuffer &buffer, s64 size, fs::ReadOption option) {
                R_RETURN(fsFileRead(std::addressof(m_base_file), offset, buffer.GetPointer(), size, option._value, reinterpret_cast<u64 *>(out.GetPointer())));
            }

            Result Write(s64 offset, const ams::sf::InNonSecureBuffer &buffer, s64 size, fs::WriteOption option) {
                R_RETURN(fsFileWrite(std::addressof(m_base_file), offset, buffer.GetPointer(), size, option._value));
            }

            Result Flush(){
                R_RETURN(fsFileFlush(std::addressof(m_base_file)));
            }

            Result SetSize(s64 size) {
                R_RETURN(fsFileSetSize(std::addressof(m_base_file), size));
            }

            Result GetSize(ams::sf::Out<s64> out) {
                R_RETURN(fsFileGetSize(std::addressof(m_base_file), out.GetPointer()));
            }

            Result OperateRange(ams::sf::Out<fs::FileQueryRangeInfo> out, s32 op_id, s64 offset, s64 size) {
                static_assert(sizeof(::FsRangeInfo) == sizeof(fs::FileQueryRangeInfo));
                R_RETURN(fsFileOperateRange(std::addressof(m_base_file), static_cast<::FsOperationId>(op_id), offset, size, reinterpret_cast<::FsRangeInfo *>(out.GetPointer())));
            }

            Result OperateRangeWithBuffer(const ams::sf::OutNonSecureBuffer &out_buf, const ams::sf::InNonSecureBuffer &in_buf, s32 op_id, s64 offset, s64 size) {
                AMS_UNUSED(out_buf, in_buf, op_id, offset, size);
                AMS_ABORT("TODO");
            }
    };
    static_assert(fssrv::sf::IsIFile<RemoteFile>);

    class RemoteDirectory {
        NON_COPYABLE(RemoteDirectory);
        NON_MOVEABLE(RemoteDirectory);
        private:
            ::FsDir m_base_dir;
        public:
            RemoteDirectory(::FsDir &s) : m_base_dir(s) { /* ... */}

            virtual ~RemoteDirectory() { fsDirClose(std::addressof(m_base_dir)); }
        public:
            Result Read(ams::sf::Out<s64> out, const ams::sf::OutBuffer &out_entries) {
                static_assert(sizeof(::FsDirectoryEntry) == sizeof(fs::DirectoryEntry));
                R_RETURN(fsDirRead(std::addressof(m_base_dir), out.GetPointer(), out_entries.GetSize() / sizeof(fs::DirectoryEntry), reinterpret_cast<::FsDirectoryEntry *>(out_entries.GetPointer())));
            }

            Result GetEntryCount(ams::sf::Out<s64> out) {
                R_RETURN(fsDirGetEntryCount(std::addressof(m_base_dir), out.GetPointer()));
            }
    };
    static_assert(fssrv::sf::IsIDirectory<RemoteDirectory>);

    class RemoteFileSystem {
        NON_COPYABLE(RemoteFileSystem);
        NON_MOVEABLE(RemoteFileSystem);
        private:
            ::FsFileSystem m_base_fs;
        public:
            RemoteFileSystem(::FsFileSystem &s) : m_base_fs(s) { /* ... */}

            virtual ~RemoteFileSystem() { fsFsClose(std::addressof(m_base_fs)); }
        public:
            /* Command API. */
            Result CreateFile(const fssrv::sf::Path &path, s64 size, s32 option) {
                R_RETURN(fsFsCreateFile(std::addressof(m_base_fs), path.str, size, option));
            }

            Result DeleteFile(const fssrv::sf::Path &path) {
                R_RETURN(fsFsDeleteFile(std::addressof(m_base_fs), path.str));
            }

            Result CreateDirectory(const fssrv::sf::Path &path) {
                R_RETURN(fsFsCreateDirectory(std::addressof(m_base_fs), path.str));
            }

            Result DeleteDirectory(const fssrv::sf::Path &path) {
                R_RETURN(fsFsDeleteDirectory(std::addressof(m_base_fs), path.str));
            }

            Result DeleteDirectoryRecursively(const fssrv::sf::Path &path) {
                R_RETURN(fsFsDeleteDirectoryRecursively(std::addressof(m_base_fs), path.str));
            }

            Result RenameFile(const fssrv::sf::Path &old_path, const fssrv::sf::Path &new_path) {
                R_RETURN(fsFsRenameFile(std::addressof(m_base_fs), old_path.str, new_path.str));
            }

            Result RenameDirectory(const fssrv::sf::Path &old_path, const fssrv::sf::Path &new_path) {
                R_RETURN(fsFsRenameDirectory(std::addressof(m_base_fs), old_path.str, new_path.str));
            }

            Result GetEntryType(ams::sf::Out<u32> out, const fssrv::sf::Path &path) {
                static_assert(sizeof(::FsDirEntryType) == sizeof(u32));
                R_RETURN(fsFsGetEntryType(std::addressof(m_base_fs), path.str, reinterpret_cast<::FsDirEntryType *>(out.GetPointer())));
            }

            Result Commit() {
                R_RETURN(fsFsCommit(std::addressof(m_base_fs)));
            }

            Result GetFreeSpaceSize(ams::sf::Out<s64> out, const fssrv::sf::Path &path) {
                R_RETURN(fsFsGetFreeSpace(std::addressof(m_base_fs), path.str, out.GetPointer()));
            }

            Result GetTotalSpaceSize(ams::sf::Out<s64> out, const fssrv::sf::Path &path) {
                R_RETURN(fsFsGetTotalSpace(std::addressof(m_base_fs), path.str, out.GetPointer()));
            }

            Result CleanDirectoryRecursively(const fssrv::sf::Path &path) {
                R_RETURN(fsFsCleanDirectoryRecursively(std::addressof(m_base_fs), path.str));
            }

            Result GetFileTimeStampRaw(ams::sf::Out<fs::FileTimeStampRaw> out, const fssrv::sf::Path &path) {
                static_assert(sizeof(fs::FileTimeStampRaw) == sizeof(::FsTimeStampRaw));
                R_RETURN(fsFsGetFileTimeStampRaw(std::addressof(m_base_fs), path.str, reinterpret_cast<::FsTimeStampRaw *>(out.GetPointer())));
            }

            Result QueryEntry(const ams::sf::OutNonSecureBuffer &out_buf, const ams::sf::InNonSecureBuffer &in_buf, s32 query_id, const fssrv::sf::Path &path) {
                R_RETURN(fsFsQueryEntry(std::addressof(m_base_fs), out_buf.GetPointer(), out_buf.GetSize(), in_buf.GetPointer(), in_buf.GetSize(), path.str, static_cast<FsFileSystemQueryId>(query_id)));
            }

            Result OpenFile(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFile>> out, const fssrv::sf::Path &path, u32 mode);
            Result OpenDirectory(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IDirectory>> out, const fssrv::sf::Path &path, u32 mode);
    };
    static_assert(fssrv::sf::IsIFileSystem<RemoteFileSystem>);
    #endif

}
