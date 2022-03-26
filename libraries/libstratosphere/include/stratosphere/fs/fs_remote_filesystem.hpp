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

namespace ams::fs {

    #if defined(ATMOSPHERE_OS_HORIZON)
    class RemoteFile : public fsa::IFile, public impl::Newable {
        NON_COPYABLE(RemoteFile);
        NON_MOVEABLE(RemoteFile);
        private:
            ::FsFile m_base_file;
        public:
            RemoteFile(const ::FsFile &f) : m_base_file(f) { /* ... */ }

            virtual ~RemoteFile() { fsFileClose(std::addressof(m_base_file)); }
        public:
            virtual Result DoRead(size_t *out, s64 offset, void *buffer, size_t size, const fs::ReadOption &option) override final {
                R_RETURN(fsFileRead(std::addressof(m_base_file), offset, buffer, size, option._value, out));
            }

            virtual Result DoGetSize(s64 *out) override final {
                R_RETURN(fsFileGetSize(std::addressof(m_base_file), out));
            }

            virtual Result DoFlush() override final {
                R_RETURN(fsFileFlush(std::addressof(m_base_file)));
            }

            virtual Result DoWrite(s64 offset, const void *buffer, size_t size, const fs::WriteOption &option) override final {
                R_RETURN(fsFileWrite(std::addressof(m_base_file), offset, buffer, size, option._value));
            }

            virtual Result DoSetSize(s64 size) override final {
                R_RETURN(fsFileSetSize(std::addressof(m_base_file), size));
            }

            virtual Result DoOperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override final {
                AMS_UNUSED(src, src_size);

                R_UNLESS(op_id == OperationId::QueryRange,       fs::ResultUnsupportedOperateRangeForFileServiceObjectAdapter());
                R_UNLESS(dst_size == sizeof(FileQueryRangeInfo), fs::ResultInvalidSize());

                R_RETURN(fsFileOperateRange(std::addressof(m_base_file), static_cast<::FsOperationId>(op_id), offset, size, reinterpret_cast<::FsRangeInfo *>(dst)));
            }
        public:
            virtual sf::cmif::DomainObjectId GetDomainObjectId() const override final {
                return sf::cmif::DomainObjectId{serviceGetObjectId(const_cast<::Service *>(std::addressof(m_base_file.s)))};
            }
    };

    class RemoteDirectory : public fsa::IDirectory, public impl::Newable {
        NON_COPYABLE(RemoteDirectory);
        NON_MOVEABLE(RemoteDirectory);
        private:
            ::FsDir m_base_dir;
        public:
            RemoteDirectory(const ::FsDir &d) : m_base_dir(d) { /* ... */ }

            virtual ~RemoteDirectory() { fsDirClose(std::addressof(m_base_dir)); }
        public:
            virtual Result DoRead(s64 *out_count, DirectoryEntry *out_entries, s64 max_entries) override final {
                static_assert(sizeof(*out_entries) == sizeof(::FsDirectoryEntry));
                R_RETURN(fsDirRead(std::addressof(m_base_dir), out_count, max_entries, reinterpret_cast<::FsDirectoryEntry *>(out_entries)));
            }

            virtual Result DoGetEntryCount(s64 *out) override final {
                R_RETURN(fsDirGetEntryCount(std::addressof(m_base_dir), out));
            }
        public:
            virtual sf::cmif::DomainObjectId GetDomainObjectId() const override final {
                return sf::cmif::DomainObjectId{serviceGetObjectId(const_cast<::Service *>(std::addressof(m_base_dir.s)))};
            }
    };

    class RemoteFileSystem : public fsa::IFileSystem, public impl::Newable {
        NON_COPYABLE(RemoteFileSystem);
        NON_MOVEABLE(RemoteFileSystem);
        private:
            ::FsFileSystem m_base_fs;
        public:
            RemoteFileSystem(const ::FsFileSystem &fs) : m_base_fs(fs) { /* ... */ }

            virtual ~RemoteFileSystem() { fsFsClose(std::addressof(m_base_fs)); }
        private:
            Result GetPathForServiceObject(fssrv::sf::Path *out_path, const fs::Path &path) {
                /* Copy, ensuring length is in bounds. */
                const size_t len = util::Strlcpy<char>(out_path->str, path.GetString(), sizeof(out_path->str));
                R_UNLESS(len < sizeof(out_path->str), fs::ResultTooLongPath());

                /* Replace directory separators. */
                /* TODO: Is this still necessary? We originally had it to not break things on low firmware. */
                #if defined(ATMOSPHERE_BOARD_NINTENDO_NX)
                fs::Replace(out_path->str, sizeof(out_path->str) - 1, '\\', '/');
                #endif

                R_SUCCEED();
            }
        public:
            virtual Result DoCreateFile(const fs::Path &path, s64 size, int flags) override final {
                fssrv::sf::Path sf_path;
                R_TRY(GetPathForServiceObject(std::addressof(sf_path), path));
                R_RETURN(fsFsCreateFile(std::addressof(m_base_fs), sf_path.str, size, flags));
            }

            virtual Result DoDeleteFile(const fs::Path &path) override final {
                fssrv::sf::Path sf_path;
                R_TRY(GetPathForServiceObject(std::addressof(sf_path), path));
                R_RETURN(fsFsDeleteFile(std::addressof(m_base_fs), sf_path.str));
            }

            virtual Result DoCreateDirectory(const fs::Path &path) override final {
                fssrv::sf::Path sf_path;
                R_TRY(GetPathForServiceObject(std::addressof(sf_path), path));
                R_RETURN(fsFsCreateDirectory(std::addressof(m_base_fs), sf_path.str));
            }

            virtual Result DoDeleteDirectory(const fs::Path &path) override final {
                fssrv::sf::Path sf_path;
                R_TRY(GetPathForServiceObject(std::addressof(sf_path), path));
                R_RETURN(fsFsDeleteDirectory(std::addressof(m_base_fs), sf_path.str));
            }

            virtual Result DoDeleteDirectoryRecursively(const fs::Path &path) override final {
                fssrv::sf::Path sf_path;
                R_TRY(GetPathForServiceObject(std::addressof(sf_path), path));
                R_RETURN(fsFsDeleteDirectoryRecursively(std::addressof(m_base_fs), sf_path.str));
            }

            virtual Result DoRenameFile(const fs::Path &old_path, const fs::Path &new_path) override final {
                fssrv::sf::Path old_sf_path;
                fssrv::sf::Path new_sf_path;
                R_TRY(GetPathForServiceObject(std::addressof(old_sf_path), old_path));
                R_TRY(GetPathForServiceObject(std::addressof(new_sf_path), new_path));
                R_RETURN(fsFsRenameFile(std::addressof(m_base_fs), old_sf_path.str, new_sf_path.str));
            }

            virtual Result DoRenameDirectory(const fs::Path &old_path, const fs::Path &new_path) override final {
                fssrv::sf::Path old_sf_path;
                fssrv::sf::Path new_sf_path;
                R_TRY(GetPathForServiceObject(std::addressof(old_sf_path), old_path));
                R_TRY(GetPathForServiceObject(std::addressof(new_sf_path), new_path));
                R_RETURN(fsFsRenameDirectory(std::addressof(m_base_fs), old_sf_path.str, new_sf_path.str));
            }

            virtual Result DoGetEntryType(DirectoryEntryType *out, const fs::Path &path) override final {
                fssrv::sf::Path sf_path;
                R_TRY(GetPathForServiceObject(std::addressof(sf_path), path));

                static_assert(sizeof(::FsDirEntryType) == sizeof(DirectoryEntryType));
                R_RETURN(fsFsGetEntryType(std::addressof(m_base_fs), sf_path.str, reinterpret_cast<::FsDirEntryType *>(out)));
            }

            virtual Result DoOpenFile(std::unique_ptr<fsa::IFile> *out_file, const fs::Path &path, OpenMode mode) override final {
                fssrv::sf::Path sf_path;
                R_TRY(GetPathForServiceObject(std::addressof(sf_path), path));

                FsFile f;
                R_TRY(fsFsOpenFile(std::addressof(m_base_fs), sf_path.str, mode, std::addressof(f)));

                auto file = std::make_unique<RemoteFile>(f);
                R_UNLESS(file != nullptr, fs::ResultAllocationMemoryFailedNew());

                *out_file = std::move(file);
                R_SUCCEED();
            }

            virtual Result DoOpenDirectory(std::unique_ptr<fsa::IDirectory> *out_dir, const fs::Path &path, OpenDirectoryMode mode) override final {
                fssrv::sf::Path sf_path;
                R_TRY(GetPathForServiceObject(std::addressof(sf_path), path));

                FsDir d;
                R_TRY(fsFsOpenDirectory(std::addressof(m_base_fs), sf_path.str, mode, std::addressof(d)));

                auto dir = std::make_unique<RemoteDirectory>(d);
                R_UNLESS(dir != nullptr, fs::ResultAllocationMemoryFailedNew());

                *out_dir = std::move(dir);
                R_SUCCEED();
            }

            virtual Result DoCommit() override final {
                R_RETURN(fsFsCommit(std::addressof(m_base_fs)));
            }

            virtual Result DoGetFreeSpaceSize(s64 *out, const fs::Path &path) override final {
                fssrv::sf::Path sf_path;
                R_TRY(GetPathForServiceObject(std::addressof(sf_path), path));
                R_RETURN(fsFsGetFreeSpace(std::addressof(m_base_fs), sf_path.str, out));
            }

            virtual Result DoGetTotalSpaceSize(s64 *out, const fs::Path &path) override final {
                fssrv::sf::Path sf_path;
                R_TRY(GetPathForServiceObject(std::addressof(sf_path), path));
                R_RETURN(fsFsGetTotalSpace(std::addressof(m_base_fs), sf_path.str, out));
            }

            virtual Result DoCleanDirectoryRecursively(const fs::Path &path) override final {
                fssrv::sf::Path sf_path;
                R_TRY(GetPathForServiceObject(std::addressof(sf_path), path));
                R_RETURN(fsFsCleanDirectoryRecursively(std::addressof(m_base_fs), sf_path.str));
            }

            virtual Result DoGetFileTimeStampRaw(FileTimeStampRaw *out, const fs::Path &path) override final {
                fssrv::sf::Path sf_path;
                R_TRY(GetPathForServiceObject(std::addressof(sf_path), path));
                static_assert(sizeof(FileTimeStampRaw) == sizeof(::FsTimeStampRaw));
                R_RETURN(fsFsGetFileTimeStampRaw(std::addressof(m_base_fs), sf_path.str, reinterpret_cast<::FsTimeStampRaw *>(out)));
            }

            virtual Result DoQueryEntry(char *dst, size_t dst_size, const char *src, size_t src_size, fsa::QueryId query, const fs::Path &path) override final {
                fssrv::sf::Path sf_path;
                R_TRY(GetPathForServiceObject(std::addressof(sf_path), path));
                R_RETURN(fsFsQueryEntry(std::addressof(m_base_fs), dst, dst_size, src, src_size, sf_path.str, static_cast<FsFileSystemQueryId>(query)));
            }
    };
    #endif

}
