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

namespace ams::mitm::fs {

    class ReadOnlyLayeredFileSystem : public ams::fs::fsa::IFileSystem {
        private:
            ams::fs::ReadOnlyFileSystem fs_1;
            ams::fs::ReadOnlyFileSystem fs_2;
        public:
            explicit ReadOnlyLayeredFileSystem(std::unique_ptr<ams::fs::fsa::IFileSystem> a, std::unique_ptr<ams::fs::fsa::IFileSystem> b) : fs_1(std::move(a)), fs_2(std::move(b)) { /* ... */ }

            virtual ~ReadOnlyLayeredFileSystem() { /* ... */ }
        private:
            virtual Result CreateFileImpl(const char *path, s64 size, int flags) override final {
                return ams::fs::ResultUnsupportedOperation();
            }

            virtual Result DeleteFileImpl(const char *path) override final {
                return ams::fs::ResultUnsupportedOperation();
            }

            virtual Result CreateDirectoryImpl(const char *path) override final {
                return ams::fs::ResultUnsupportedOperation();
            }

            virtual Result DeleteDirectoryImpl(const char *path) override final {
                return ams::fs::ResultUnsupportedOperation();
            }

            virtual Result DeleteDirectoryRecursivelyImpl(const char *path) override final {
                return ams::fs::ResultUnsupportedOperation();
            }

            virtual Result RenameFileImpl(const char *old_path, const char *new_path) override final {
                return ams::fs::ResultUnsupportedOperation();
            }

            virtual Result RenameDirectoryImpl(const char *old_path, const char *new_path) override final {
                return ams::fs::ResultUnsupportedOperation();
            }

            virtual Result GetEntryTypeImpl(ams::fs::DirectoryEntryType *out, const char *path) override final {
                R_UNLESS(R_FAILED(this->fs_1.GetEntryType(out, path)), ResultSuccess());
                return this->fs_2.GetEntryType(out, path);
            }

            virtual Result OpenFileImpl(std::unique_ptr<ams::fs::fsa::IFile> *out_file, const char *path, ams::fs::OpenMode mode) override final {
                R_UNLESS(R_FAILED(this->fs_1.OpenFile(out_file, path, mode)), ResultSuccess());
                return this->fs_2.OpenFile(out_file, path, mode);
            }

            virtual Result OpenDirectoryImpl(std::unique_ptr<ams::fs::fsa::IDirectory> *out_dir, const char *path, ams::fs::OpenDirectoryMode mode) override final {
                R_UNLESS(R_FAILED(this->fs_1.OpenDirectory(out_dir, path, mode)), ResultSuccess());
                return this->fs_2.OpenDirectory(out_dir, path, mode);
            }

            virtual Result CommitImpl() override final {
                return ResultSuccess();
            }

            virtual Result GetFreeSpaceSizeImpl(s64 *out, const char *path) {
                return ams::fs::ResultUnsupportedOperation();
            }

            virtual Result GetTotalSpaceSizeImpl(s64 *out, const char *path) {
                return ams::fs::ResultUnsupportedOperation();
            }

            virtual Result CleanDirectoryRecursivelyImpl(const char *path) {
                return ams::fs::ResultUnsupportedOperation();
            }

            virtual Result GetFileTimeStampRawImpl(ams::fs::FileTimeStampRaw *out, const char *path) {
                R_UNLESS(R_FAILED(this->fs_1.GetFileTimeStampRaw(out, path)), ResultSuccess());
                return this->fs_2.GetFileTimeStampRaw(out, path);
            }
    };

}
