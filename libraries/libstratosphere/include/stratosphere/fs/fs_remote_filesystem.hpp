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
#include <stratosphere/fs/impl/fs_newable.hpp>
#include <stratosphere/fs/fsa/fs_ifile.hpp>
#include <stratosphere/fs/fsa/fs_idirectory.hpp>
#include <stratosphere/fs/fsa/fs_ifilesystem.hpp>
#include <stratosphere/fs/fs_query_range.hpp>
#include <stratosphere/fs/fs_path_utils.hpp>

namespace ams::fs {

    class RemoteFile : public fsa::IFile, public impl::Newable {
        private:
            ::FsFile base_file;
        public:
            RemoteFile(const ::FsFile &f) : base_file(f) { /* ... */ }

            virtual ~RemoteFile() { fsFileClose(std::addressof(this->base_file)); }
        public:
            virtual Result DoRead(size_t *out, s64 offset, void *buffer, size_t size, const fs::ReadOption &option) override final {
                return fsFileRead(std::addressof(this->base_file), offset, buffer, size, option.value, out);
            }

            virtual Result DoGetSize(s64 *out) override final {
                return fsFileGetSize(std::addressof(this->base_file), out);
            }

            virtual Result DoFlush() override final {
                return fsFileFlush(std::addressof(this->base_file));
            }

            virtual Result DoWrite(s64 offset, const void *buffer, size_t size, const fs::WriteOption &option) override final {
                return fsFileWrite(std::addressof(this->base_file), offset, buffer, size, option.value);
            }

            virtual Result DoSetSize(s64 size) override final {
                return fsFileSetSize(std::addressof(this->base_file), size);
            }

            virtual Result DoOperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override final {
                R_UNLESS(op_id == OperationId::QueryRange,       fs::ResultUnsupportedOperationInFileServiceObjectAdapterA());
                R_UNLESS(dst_size == sizeof(FileQueryRangeInfo), fs::ResultInvalidSize());

                return fsFileOperateRange(std::addressof(this->base_file), static_cast<::FsOperationId>(op_id), offset, size, reinterpret_cast<::FsRangeInfo *>(dst));
            }
        public:
            virtual sf::cmif::DomainObjectId GetDomainObjectId() const override {
                return sf::cmif::DomainObjectId{serviceGetObjectId(const_cast<::Service *>(std::addressof(this->base_file.s)))};
            }
    };

    class RemoteDirectory : public fsa::IDirectory, public impl::Newable {
        private:
            ::FsDir base_dir;
        public:
            RemoteDirectory(const ::FsDir &d) : base_dir(d) { /* ... */ }

            virtual ~RemoteDirectory() { fsDirClose(std::addressof(this->base_dir)); }
        public:
            virtual Result DoRead(s64 *out_count, DirectoryEntry *out_entries, s64 max_entries) override final {
                return fsDirRead(std::addressof(this->base_dir), out_count, max_entries, out_entries);
            }

            virtual Result DoGetEntryCount(s64 *out) override final {
                return fsDirGetEntryCount(std::addressof(this->base_dir), out);
            }
        public:
            virtual sf::cmif::DomainObjectId GetDomainObjectId() const override {
                return sf::cmif::DomainObjectId{serviceGetObjectId(const_cast<::Service *>(std::addressof(this->base_dir.s)))};
            }
    };

    class RemoteFileSystem : public fsa::IFileSystem, public impl::Newable {
        private:
            ::FsFileSystem base_fs;
        public:
            RemoteFileSystem(const ::FsFileSystem &fs) : base_fs(fs) { /* ... */ }

            virtual ~RemoteFileSystem() { fsFsClose(std::addressof(this->base_fs)); }
        private:
            Result GetPathForServiceObject(fssrv::sf::Path *out_path, const char *path) {
                /* Copy and null terminate. */
                std::strncpy(out_path->str, path, sizeof(out_path->str) - 1);
                out_path->str[sizeof(out_path->str) - 1] = '\x00';

                /* Replace directory separators. */
                fs::Replace(out_path->str, sizeof(out_path->str) - 1, StringTraits::AlternateDirectorySeparator, StringTraits::DirectorySeparator);

                /* Get lengths. */
                const auto skip_len = fs::GetWindowsPathSkipLength(path);
                const auto rel_path = out_path->str + skip_len;
                const auto max_len  = fs::EntryNameLengthMax - skip_len;
                return VerifyPath(rel_path, max_len, max_len);
            }
        public:
            virtual Result DoCreateFile(const char *path, s64 size, int flags) override final {
                fssrv::sf::Path sf_path;
                R_TRY(GetPathForServiceObject(std::addressof(sf_path), path));
                return fsFsCreateFile(std::addressof(this->base_fs), sf_path.str, size, flags);
            }

            virtual Result DoDeleteFile(const char *path) override final {
                fssrv::sf::Path sf_path;
                R_TRY(GetPathForServiceObject(std::addressof(sf_path), path));
                return fsFsDeleteFile(std::addressof(this->base_fs), sf_path.str);
            }

            virtual Result DoCreateDirectory(const char *path) override final {
                fssrv::sf::Path sf_path;
                R_TRY(GetPathForServiceObject(std::addressof(sf_path), path));
                return fsFsCreateDirectory(std::addressof(this->base_fs), sf_path.str);
            }

            virtual Result DoDeleteDirectory(const char *path) override final {
                fssrv::sf::Path sf_path;
                R_TRY(GetPathForServiceObject(std::addressof(sf_path), path));
                return fsFsDeleteDirectory(std::addressof(this->base_fs), sf_path.str);
            }

            virtual Result DoDeleteDirectoryRecursively(const char *path) override final {
                fssrv::sf::Path sf_path;
                R_TRY(GetPathForServiceObject(std::addressof(sf_path), path));
                return fsFsDeleteDirectoryRecursively(std::addressof(this->base_fs), sf_path.str);
            }

            virtual Result DoRenameFile(const char *old_path, const char *new_path) override final {
                fssrv::sf::Path old_sf_path;
                fssrv::sf::Path new_sf_path;
                R_TRY(GetPathForServiceObject(std::addressof(old_sf_path), old_path));
                R_TRY(GetPathForServiceObject(std::addressof(new_sf_path), new_path));
                return fsFsRenameFile(std::addressof(this->base_fs), old_sf_path.str, new_sf_path.str);
            }

            virtual Result DoRenameDirectory(const char *old_path, const char *new_path) override final {
                fssrv::sf::Path old_sf_path;
                fssrv::sf::Path new_sf_path;
                R_TRY(GetPathForServiceObject(std::addressof(old_sf_path), old_path));
                R_TRY(GetPathForServiceObject(std::addressof(new_sf_path), new_path));
                return fsFsRenameDirectory(std::addressof(this->base_fs), old_sf_path.str, new_sf_path.str);
            }

            virtual Result DoGetEntryType(DirectoryEntryType *out, const char *path) override final {
                fssrv::sf::Path sf_path;
                R_TRY(GetPathForServiceObject(std::addressof(sf_path), path));

                static_assert(sizeof(::FsDirEntryType) == sizeof(DirectoryEntryType));
                return fsFsGetEntryType(std::addressof(this->base_fs), sf_path.str, reinterpret_cast<::FsDirEntryType *>(out));
            }

            virtual Result DoOpenFile(std::unique_ptr<fsa::IFile> *out_file, const char *path, OpenMode mode) override final {
                fssrv::sf::Path sf_path;
                R_TRY(GetPathForServiceObject(std::addressof(sf_path), path));

                FsFile f;
                R_TRY(fsFsOpenFile(std::addressof(this->base_fs), sf_path.str, mode, &f));

                auto file = std::make_unique<RemoteFile>(f);
                R_UNLESS(file != nullptr, fs::ResultAllocationFailureInNew());

                *out_file = std::move(file);
                return ResultSuccess();
            }

            virtual Result DoOpenDirectory(std::unique_ptr<fsa::IDirectory> *out_dir, const char *path, OpenDirectoryMode mode) override final {
                fssrv::sf::Path sf_path;
                R_TRY(GetPathForServiceObject(std::addressof(sf_path), path));

                FsDir d;
                R_TRY(fsFsOpenDirectory(std::addressof(this->base_fs), sf_path.str, mode, &d));

                auto dir = std::make_unique<RemoteDirectory>(d);
                R_UNLESS(dir != nullptr, fs::ResultAllocationFailureInNew());

                *out_dir = std::move(dir);
                return ResultSuccess();
            }

            virtual Result DoCommit() override final {
                return fsFsCommit(std::addressof(this->base_fs));
            }


            virtual Result DoGetFreeSpaceSize(s64 *out, const char *path) {
                fssrv::sf::Path sf_path;
                R_TRY(GetPathForServiceObject(std::addressof(sf_path), path));
                return fsFsGetFreeSpace(std::addressof(this->base_fs), sf_path.str, out);
            }

            virtual Result DoGetTotalSpaceSize(s64 *out, const char *path) {
                fssrv::sf::Path sf_path;
                R_TRY(GetPathForServiceObject(std::addressof(sf_path), path));
                return fsFsGetTotalSpace(std::addressof(this->base_fs), sf_path.str, out);
            }

            virtual Result DoCleanDirectoryRecursively(const char *path) {
                fssrv::sf::Path sf_path;
                R_TRY(GetPathForServiceObject(std::addressof(sf_path), path));
                return fsFsCleanDirectoryRecursively(std::addressof(this->base_fs), sf_path.str);
            }

            virtual Result DoGetFileTimeStampRaw(FileTimeStampRaw *out, const char *path) {
                fssrv::sf::Path sf_path;
                R_TRY(GetPathForServiceObject(std::addressof(sf_path), path));
                static_assert(sizeof(FileTimeStampRaw) == sizeof(::FsTimeStampRaw));
                return fsFsGetFileTimeStampRaw(std::addressof(this->base_fs), sf_path.str, reinterpret_cast<::FsTimeStampRaw *>(out));
            }

            virtual Result DoQueryEntry(char *dst, size_t dst_size, const char *src, size_t src_size, fsa::QueryId query, const char *path) {
                fssrv::sf::Path sf_path;
                R_TRY(GetPathForServiceObject(std::addressof(sf_path), path));
                return fsFsQueryEntry(std::addressof(this->base_fs), dst, dst_size, src, src_size, sf_path.str, static_cast<FsFileSystemQueryId>(query));
            }
    };

}
