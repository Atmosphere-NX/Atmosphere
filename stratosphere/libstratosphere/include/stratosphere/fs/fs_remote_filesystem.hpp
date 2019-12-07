/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include "fs_common.hpp"
#include "fsa/fs_ifile.hpp"
#include "fsa/fs_idirectory.hpp"
#include "fsa/fs_ifilesystem.hpp"

namespace ams::fs {

    class RemoteFile : public fsa::IFile {
        private:
            std::unique_ptr<::FsFile> base_file;
        public:
            RemoteFile(::FsFile *f) : base_file(f) { /* ... */ }
            RemoteFile(std::unique_ptr<::FsFile> f) : base_file(std::move(f)) { /* ... */ }
            RemoteFile(::FsFile f) {
                this->base_file = std::make_unique<::FsFile>(f);
            }

            virtual ~RemoteFile() { fsFileClose(this->base_file.get()); }
        public:
            virtual Result ReadImpl(size_t *out, s64 offset, void *buffer, size_t size, const fs::ReadOption &option) override final {
                return fsFileRead(this->base_file.get(), offset, buffer, size, option.value, out);
            }

            virtual Result GetSizeImpl(s64 *out) override final {
                return fsFileGetSize(this->base_file.get(), out);
            }

            virtual Result FlushImpl() override final {
                return fsFileFlush(this->base_file.get());
            }

            virtual Result WriteImpl(s64 offset, const void *buffer, size_t size, const fs::WriteOption &option) override final {
                return fsFileWrite(this->base_file.get(), offset, buffer, size, option.value);
            }

            virtual Result SetSizeImpl(s64 size) override final {
                return fsFileSetSize(this->base_file.get(), size);
            }

            virtual Result OperateRangeImpl(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override final {
                /* TODO: How should this be handled? */
                return fs::ResultNotImplemented();
            }
        public:
            virtual sf::cmif::DomainObjectId GetDomainObjectId() const override {
                return sf::cmif::DomainObjectId{serviceGetObjectId(&this->base_file->s)};
            }
    };

    class RemoteDirectory : public fsa::IDirectory {
        private:
            std::unique_ptr<::FsDir> base_dir;
        public:
            RemoteDirectory(::FsDir *d) : base_dir(d) { /* ... */ }
            RemoteDirectory(std::unique_ptr<::FsDir> d) : base_dir(std::move(d)) { /* ... */ }
            RemoteDirectory(::FsDir d) {
                this->base_dir = std::make_unique<::FsDir>(d);
            }

            virtual ~RemoteDirectory() { fsDirClose(this->base_dir.get()); }
        public:
            virtual Result ReadImpl(s64 *out_count, DirectoryEntry *out_entries, s64 max_entries) override final {
                return fsDirRead(this->base_dir.get(), out_count, max_entries, out_entries);
            }

            virtual Result GetEntryCountImpl(s64 *out) override final {
                return fsDirGetEntryCount(this->base_dir.get(), out);
            }
        public:
            virtual sf::cmif::DomainObjectId GetDomainObjectId() const override {
                return sf::cmif::DomainObjectId{serviceGetObjectId(&this->base_dir->s)};
            }
    };

    class RemoteFileSystem : public fsa::IFileSystem {
        private:
            std::unique_ptr<::FsFileSystem> base_fs;
        public:
            RemoteFileSystem(::FsFileSystem *fs) : base_fs(fs) { /* ... */ }
            RemoteFileSystem(std::unique_ptr<::FsFileSystem> fs) : base_fs(std::move(fs)) { /* ... */ }
            RemoteFileSystem(::FsFileSystem fs) {
                this->base_fs = std::make_unique<::FsFileSystem>(fs);
            }

            virtual ~RemoteFileSystem() { fsFsClose(this->base_fs.get()); }
        public:
            virtual Result CreateFileImpl(const char *path, s64 size, int flags) override final {
                return fsFsCreateFile(this->base_fs.get(), path, size, flags);
            }

            virtual Result DeleteFileImpl(const char *path) override final {
                return fsFsDeleteFile(this->base_fs.get(), path);
            }

            virtual Result CreateDirectoryImpl(const char *path) override final {
                return fsFsCreateDirectory(this->base_fs.get(), path);
            }

            virtual Result DeleteDirectoryImpl(const char *path) override final {
                return fsFsDeleteDirectory(this->base_fs.get(), path);
            }

            virtual Result DeleteDirectoryRecursivelyImpl(const char *path) override final {
                return fsFsDeleteDirectoryRecursively(this->base_fs.get(), path);
            }

            virtual Result RenameFileImpl(const char *old_path, const char *new_path) override final {
                return fsFsRenameFile(this->base_fs.get(), old_path, new_path);
            }

            virtual Result RenameDirectoryImpl(const char *old_path, const char *new_path) override final {
                return fsFsRenameDirectory(this->base_fs.get(), old_path, new_path);
            }

            virtual Result GetEntryTypeImpl(DirectoryEntryType *out, const char *path) override final {
                static_assert(sizeof(::FsDirEntryType) == sizeof(DirectoryEntryType));
                return fsFsGetEntryType(this->base_fs.get(), path, reinterpret_cast<::FsDirEntryType *>(out));
            }

            virtual Result OpenFileImpl(std::unique_ptr<fsa::IFile> *out_file, const char *path, OpenMode mode) override final {
                FsFile f;
                R_TRY(fsFsOpenFile(this->base_fs.get(), path, mode, &f));

                *out_file = std::make_unique<RemoteFile>(f);
                return ResultSuccess();
            }

            virtual Result OpenDirectoryImpl(std::unique_ptr<fsa::IDirectory> *out_dir, const char *path, OpenDirectoryMode mode) override final {
                FsDir d;
                R_TRY(fsFsOpenDirectory(this->base_fs.get(), path, mode, &d));

                *out_dir = std::make_unique<RemoteDirectory>(d);
                return ResultSuccess();
            }

            virtual Result CommitImpl() override final {
                return fsFsCommit(this->base_fs.get());
            }


            virtual Result GetFreeSpaceSizeImpl(s64 *out, const char *path) {
                return fsFsGetFreeSpace(this->base_fs.get(), path, out);
            }

            virtual Result GetTotalSpaceSizeImpl(s64 *out, const char *path) {
                return fsFsGetTotalSpace(this->base_fs.get(), path, out);
            }

            virtual Result CleanDirectoryRecursivelyImpl(const char *path) {
                return fsFsCleanDirectoryRecursively(this->base_fs.get(), path);
            }

            virtual Result GetFileTimeStampRawImpl(FileTimeStampRaw *out, const char *path) {
                static_assert(sizeof(FileTimeStampRaw) == sizeof(::FsTimeStampRaw));
                return fsFsGetFileTimeStampRaw(this->base_fs.get(), path, reinterpret_cast<::FsTimeStampRaw *>(out));
            }

            virtual Result QueryEntryImpl(char *dst, size_t dst_size, const char *src, size_t src_size, fsa::QueryId query, const char *path) {
                return fsFsQueryEntry(this->base_fs.get(), dst, dst_size, src, src_size, path, static_cast<FsFileSystemQueryId>(query));
            }
    };

}
