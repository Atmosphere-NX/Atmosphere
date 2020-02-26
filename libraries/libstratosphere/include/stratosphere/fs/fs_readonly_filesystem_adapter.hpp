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

namespace ams::fs {

    class ReadOnlyFileAdapter : public fsa::IFile {
        NON_COPYABLE(ReadOnlyFileAdapter);
        private:
            std::unique_ptr<fsa::IFile> base_file;
        public:
            ReadOnlyFileAdapter(fsa::IFile *f) : base_file(f) { /* ... */ }
            ReadOnlyFileAdapter(std::unique_ptr<fsa::IFile> f) : base_file(std::move(f)) { /* ... */ }
            virtual ~ReadOnlyFileAdapter() { /* ... */ }
        public:
            virtual Result ReadImpl(size_t *out, s64 offset, void *buffer, size_t size, const fs::ReadOption &option) override final {
                /* Ensure that we can read these extents. */
                size_t read_size = 0;
                R_TRY(this->DryRead(std::addressof(read_size), offset, size, option, fs::OpenMode_Read));

                /* Validate preconditions. */
                AMS_ASSERT(offset >= 0);
                AMS_ASSERT(buffer != nullptr || size == 0);

                return this->base_file->Read(out, offset, buffer, size, option);
            }

            virtual Result GetSizeImpl(s64 *out) override final {
                return this->base_file->GetSize(out);
            }

            virtual Result FlushImpl() override final {
                return ResultSuccess();
            }

            virtual Result WriteImpl(s64 offset, const void *buffer, size_t size, const fs::WriteOption &option) override final {
                return fs::ResultUnsupportedOperation();
            }

            virtual Result SetSizeImpl(s64 size) override final {
                return fs::ResultUnsupportedOperation();
            }

            virtual Result OperateRangeImpl(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override final {
                /* TODO: How should this be handled? */
                return fs::ResultNotImplemented();
            }
        public:
            virtual sf::cmif::DomainObjectId GetDomainObjectId() const override {
                return this->base_file->GetDomainObjectId();
            }
    };

    class ReadOnlyFileSystemAdapter : public fsa::IFileSystem {
        NON_COPYABLE(ReadOnlyFileSystemAdapter);
        private:
            std::shared_ptr<fsa::IFileSystem> shared_fs;
            std::unique_ptr<fsa::IFileSystem> unique_fs;
        protected:
            fsa::IFileSystem * const base_fs;
        public:
            template<typename T>
            explicit ReadOnlyFileSystemAdapter(std::shared_ptr<T> fs) : shared_fs(std::move(fs)), base_fs(shared_fs.get())  { static_assert(std::is_base_of<fsa::IFileSystem, T>::value); }

            template<typename T>
            explicit ReadOnlyFileSystemAdapter(std::unique_ptr<T> fs) : unique_fs(std::move(fs)), base_fs(unique_fs.get())  { static_assert(std::is_base_of<fsa::IFileSystem, T>::value); }

            virtual ~ReadOnlyFileSystemAdapter() { /* ... */ }
        public:
            virtual Result CreateFileImpl(const char *path, s64 size, int flags) override final {
                return fs::ResultUnsupportedOperation();
            }

            virtual Result DeleteFileImpl(const char *path) override final {
                return fs::ResultUnsupportedOperation();
            }

            virtual Result CreateDirectoryImpl(const char *path) override final {
                return fs::ResultUnsupportedOperation();
            }

            virtual Result DeleteDirectoryImpl(const char *path) override final {
                return fs::ResultUnsupportedOperation();
            }

            virtual Result DeleteDirectoryRecursivelyImpl(const char *path) override final {
                return fs::ResultUnsupportedOperation();
            }

            virtual Result RenameFileImpl(const char *old_path, const char *new_path) override final {
                return fs::ResultUnsupportedOperation();
            }

            virtual Result RenameDirectoryImpl(const char *old_path, const char *new_path) override final {
                return fs::ResultUnsupportedOperation();
            }

            virtual Result GetEntryTypeImpl(DirectoryEntryType *out, const char *path) override final {
                return this->base_fs->GetEntryType(out, path);
            }

            virtual Result OpenFileImpl(std::unique_ptr<fsa::IFile> *out_file, const char *path, OpenMode mode) override final {
                std::unique_ptr<fsa::IFile> f;
                R_TRY(this->base_fs->OpenFile(std::addressof(f), path, mode));

                *out_file = std::make_unique<ReadOnlyFileAdapter>(std::move(f));
                return ResultSuccess();
            }

            virtual Result OpenDirectoryImpl(std::unique_ptr<fsa::IDirectory> *out_dir, const char *path, OpenDirectoryMode mode) override final {
                return this->base_fs->OpenDirectory(out_dir, path, mode);
            }

            virtual Result CommitImpl() override final {
                return ResultSuccess();
            }

            virtual Result GetFreeSpaceSizeImpl(s64 *out, const char *path) {
                return fs::ResultUnsupportedOperation();
            }

            virtual Result GetTotalSpaceSizeImpl(s64 *out, const char *path) {
                return fs::ResultUnsupportedOperation();
            }

            virtual Result CleanDirectoryRecursivelyImpl(const char *path) {
                return fs::ResultUnsupportedOperation();
            }

            virtual Result GetFileTimeStampRawImpl(FileTimeStampRaw *out, const char *path) {
                return this->base_fs->GetFileTimeStampRaw(out, path);
            }
    };

}
