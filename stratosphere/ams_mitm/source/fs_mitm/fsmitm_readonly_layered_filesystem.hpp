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
            virtual Result DoCreateFile(const char *path, s64 size, int flags) override final {
                return ams::fs::ResultUnsupportedOperation();
            }

            virtual Result DoDeleteFile(const char *path) override final {
                return ams::fs::ResultUnsupportedOperation();
            }

            virtual Result DoCreateDirectory(const char *path) override final {
                return ams::fs::ResultUnsupportedOperation();
            }

            virtual Result DoDeleteDirectory(const char *path) override final {
                return ams::fs::ResultUnsupportedOperation();
            }

            virtual Result DoDeleteDirectoryRecursively(const char *path) override final {
                return ams::fs::ResultUnsupportedOperation();
            }

            virtual Result DoRenameFile(const char *old_path, const char *new_path) override final {
                return ams::fs::ResultUnsupportedOperation();
            }

            virtual Result DoRenameDirectory(const char *old_path, const char *new_path) override final {
                return ams::fs::ResultUnsupportedOperation();
            }

            virtual Result DoGetEntryType(ams::fs::DirectoryEntryType *out, const char *path) override final {
                R_UNLESS(R_FAILED(this->fs_1.GetEntryType(out, path)), ResultSuccess());
                return this->fs_2.GetEntryType(out, path);
            }

            virtual Result DoOpenFile(std::unique_ptr<ams::fs::fsa::IFile> *out_file, const char *path, ams::fs::OpenMode mode) override final {
                R_UNLESS(R_FAILED(this->fs_1.OpenFile(out_file, path, mode)), ResultSuccess());
                return this->fs_2.OpenFile(out_file, path, mode);
            }

            virtual Result DoOpenDirectory(std::unique_ptr<ams::fs::fsa::IDirectory> *out_dir, const char *path, ams::fs::OpenDirectoryMode mode) override final {
                R_UNLESS(R_FAILED(this->fs_1.OpenDirectory(out_dir, path, mode)), ResultSuccess());
                return this->fs_2.OpenDirectory(out_dir, path, mode);
            }

            virtual Result DoCommit() override final {
                return ResultSuccess();
            }

            virtual Result DoGetFreeSpaceSize(s64 *out, const char *path) {
                return ams::fs::ResultUnsupportedOperation();
            }

            virtual Result DoGetTotalSpaceSize(s64 *out, const char *path) {
                return ams::fs::ResultUnsupportedOperation();
            }

            virtual Result DoCleanDirectoryRecursively(const char *path) {
                return ams::fs::ResultUnsupportedOperation();
            }

            virtual Result DoGetFileTimeStampRaw(ams::fs::FileTimeStampRaw *out, const char *path) {
                R_UNLESS(R_FAILED(this->fs_1.GetFileTimeStampRaw(out, path)), ResultSuccess());
                return this->fs_2.GetFileTimeStampRaw(out, path);
            }
    };

}
