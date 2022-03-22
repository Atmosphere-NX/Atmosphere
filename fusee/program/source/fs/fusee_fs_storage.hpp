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
#include <exosphere.hpp>
#include "fusee_fs_api.hpp"

namespace ams::fs {

    class IStorage {
        public:
            virtual Result Read(s64 offset, void *buffer, size_t size) = 0;

            virtual Result Write(s64 offset, const void *buffer, size_t size) = 0;

            virtual Result Flush() = 0;

            virtual Result SetSize(s64 size) = 0;

            virtual Result GetSize(s64 *out) = 0;
        public:
            static inline bool CheckAccessRange(s64 offset, s64 size, s64 total_size) {
                return offset >= 0 &&
                       size >= 0 &&
                       size <= total_size &&
                       offset <= (total_size - size);
            }

            static inline bool CheckAccessRange(s64 offset, size_t size, s64 total_size) {
                return CheckAccessRange(offset, static_cast<s64>(size), total_size);
            }

            static inline bool CheckOffsetAndSize(s64 offset, s64 size) {
                return offset >= 0 &&
                       size >= 0 &&
                       offset <= (offset + size);
            }

            static inline bool CheckOffsetAndSize(s64 offset, size_t size) {
                return CheckOffsetAndSize(offset, static_cast<s64>(size));
            }
    };

    class ReadOnlyStorageAdapter : public IStorage {
        private:
            IStorage &m_storage;
        public:
            ReadOnlyStorageAdapter(IStorage &s) : m_storage(s) { /* ... */ }

            virtual Result Read(s64 offset, void *buffer, size_t size) override {
                return m_storage.Read(offset, buffer, size);
            }

            virtual Result Flush() override {
                return m_storage.Flush();
            }

            virtual Result GetSize(s64 *out) override {
                return m_storage.GetSize(out);
            }

            virtual Result Write(s64 offset, const void *buffer, size_t size) override {
                return fs::ResultUnsupportedOperation();
            }

            virtual Result SetSize(s64 size) override {
                return fs::ResultUnsupportedOperation();
            }
    };

    class SubStorage : public IStorage {
        private:
            IStorage &m_storage;
            s64 m_offset;
            s64 m_size;
        public:
            SubStorage(IStorage &s, s64 o, s64 sz) : m_storage(s), m_offset(o), m_size(sz) { /* ... */ }

            virtual Result Read(s64 offset, void *buffer, size_t size) override {
                /* Succeed immediately on zero-sized operation. */
                R_SUCCEED_IF(size == 0);

                /* Validate arguments and read. */
                R_UNLESS(buffer != nullptr,                                fs::ResultNullptrArgument());
                R_UNLESS(IStorage::CheckAccessRange(offset, size, m_size), fs::ResultOutOfRange());
                return m_storage.Read(m_offset + offset, buffer, size);
            }

            virtual Result Write(s64 offset, const void *buffer, size_t size) override{
                /* Succeed immediately on zero-sized operation. */
                R_SUCCEED_IF(size == 0);

                /* Validate arguments and write. */
                R_UNLESS(buffer != nullptr,                                fs::ResultNullptrArgument());
                R_UNLESS(IStorage::CheckAccessRange(offset, size, m_size), fs::ResultOutOfRange());
                return m_storage.Write(m_offset + offset, buffer, size);
            }

            virtual Result Flush() override {
                return m_storage.Flush();
            }

            virtual Result GetSize(s64 *out) override {
                *out = m_size;
                return ResultSuccess();
            }

            virtual Result SetSize(s64 size) override {
                return fs::ResultUnsupportedOperationInSubStorageA();
            }
    };

    class FileHandleStorage : public IStorage {
        private:
            static constexpr s64 InvalidSize = -1;
        private:
            FileHandle m_handle;
            s64 m_size;
        public:
            constexpr explicit FileHandleStorage(FileHandle handle) : m_handle(handle), m_size(InvalidSize) { /* ... */ }

            ~FileHandleStorage() { fs::CloseFile(m_handle); }
        protected:
            Result UpdateSize();
        public:
            virtual Result Read(s64 offset, void *buffer, size_t size) override;
            virtual Result Write(s64 offset, const void *buffer, size_t size) override;
            virtual Result Flush() override;
            virtual Result GetSize(s64 *out_size) override;
            virtual Result SetSize(s64 size) override;
    };

}
