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
#include <stratosphere/fs/fs_istorage.hpp>
#include <stratosphere/fs/fsa/fs_ifile.hpp>
#include <stratosphere/fs/fsa/fs_ifilesystem.hpp>
#include <stratosphere/fs/impl/fs_newable.hpp>

namespace ams::fs {

    class FileStorage : public IStorage, public impl::Newable {
        NON_COPYABLE(FileStorage);
        NON_MOVEABLE(FileStorage);
        private:
            static constexpr s64 InvalidSize = -1;
        private:
            std::unique_ptr<fsa::IFile> unique_file;
            std::shared_ptr<fsa::IFile> shared_file;
            fsa::IFile *base_file;
            s64 size;
        public:
            FileStorage(fsa::IFile *f) : unique_file(f), size(InvalidSize) {
                this->base_file = this->unique_file.get();
            }

            FileStorage(std::unique_ptr<fsa::IFile> f) : unique_file(std::move(f)), size(InvalidSize) {
                this->base_file = this->unique_file.get();
            }

            FileStorage(std::shared_ptr<fsa::IFile> f) : shared_file(f), size(InvalidSize) {
                this->base_file = this->shared_file.get();
            }

            virtual ~FileStorage() { /* ... */ }
        private:
            Result UpdateSize();
        protected:
            constexpr FileStorage() : unique_file(), shared_file(), base_file(nullptr), size(InvalidSize) { /* ... */ }

            void SetFile(fs::fsa::IFile *file) {
                AMS_ASSERT(file != nullptr);
                AMS_ASSERT(this->base_file == nullptr);
                this->base_file = file;
            }

            void SetFile(std::unique_ptr<fs::fsa::IFile> &&file) {
                AMS_ASSERT(file != nullptr);
                AMS_ASSERT(this->base_file == nullptr);
                AMS_ASSERT(this->unique_file == nullptr);

                this->unique_file = std::move(file);
                this->base_file   = this->unique_file.get();
            }
        public:
            virtual Result Read(s64 offset, void *buffer, size_t size) override;
            virtual Result Write(s64 offset, const void *buffer, size_t size) override;
            virtual Result Flush() override;
            virtual Result GetSize(s64 *out_size) override;
            virtual Result SetSize(s64 size) override;
            virtual Result OperateRange(void *dst, size_t dst_size, OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override;
    };

    class FileStorageBasedFileSystem : public FileStorage {
        NON_COPYABLE(FileStorageBasedFileSystem);
        NON_MOVEABLE(FileStorageBasedFileSystem);
        private:
            std::shared_ptr<fs::fsa::IFileSystem> base_file_system;
        public:
            constexpr FileStorageBasedFileSystem() : FileStorage(), base_file_system(nullptr) { /* ... */ }

            Result Initialize(std::shared_ptr<fs::fsa::IFileSystem> base_file_system, const char *path, fs::OpenMode mode);
    };

    class FileHandleStorage : public IStorage, public impl::Newable {
        private:
            static constexpr s64 InvalidSize = -1;
        private:
            FileHandle handle;
            bool close_file;
            s64 size;
            os::Mutex mutex;
        public:
            constexpr explicit FileHandleStorage(FileHandle handle, bool close_file) : handle(handle), close_file(close_file), size(InvalidSize), mutex(false) { /* ... */ }
            constexpr explicit FileHandleStorage(FileHandle handle) : FileHandleStorage(handle, false) { /* ... */ }

            virtual ~FileHandleStorage() override {
                if (this->close_file) {
                    CloseFile(this->handle);
                }
             }
        protected:
            Result UpdateSize();
        public:
            virtual Result Read(s64 offset, void *buffer, size_t size) override;
            virtual Result Write(s64 offset, const void *buffer, size_t size) override;
            virtual Result Flush() override;
            virtual Result GetSize(s64 *out_size) override;
            virtual Result SetSize(s64 size) override;
            virtual Result OperateRange(void *dst, size_t dst_size, OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override;
    };

}
