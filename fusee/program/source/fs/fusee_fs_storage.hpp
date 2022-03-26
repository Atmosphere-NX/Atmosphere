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
            static inline Result CheckAccessRange(s64 offset, s64 size, s64 total_size) {
                R_UNLESS(offset >= 0,                                    fs::ResultInvalidOffset());
                R_UNLESS(size >= 0,                                      fs::ResultInvalidSize());
                R_UNLESS(util::CanAddWithoutOverflow<s64>(offset, size), fs::ResultOutOfRange());
                R_UNLESS(offset + size <= total_size,                    fs::ResultOutOfRange());
                R_SUCCEED();
            }

            static ALWAYS_INLINE Result CheckAccessRange(s64 offset, size_t size, s64 total_size) {
                R_RETURN(CheckAccessRange(offset, static_cast<s64>(size), total_size));
            }

            static inline Result CheckOffsetAndSize(s64 offset, s64 size) {
                R_UNLESS(offset >= 0,                                    fs::ResultInvalidOffset());
                R_UNLESS(size >= 0,                                      fs::ResultInvalidSize());
                R_UNLESS(util::CanAddWithoutOverflow<s64>(offset, size), fs::ResultOutOfRange());
                R_SUCCEED();
            }

            static ALWAYS_INLINE Result CheckOffsetAndSize(s64 offset, size_t size) {
                R_RETURN(CheckOffsetAndSize(offset, static_cast<s64>(size)));
            }

            static inline Result CheckOffsetAndSizeWithResult(s64 offset, s64 size, Result fail_result) {
                R_TRY_CATCH(CheckOffsetAndSize(offset, size)) {
                    R_CONVERT_ALL(fail_result);
                } R_END_TRY_CATCH;
                R_SUCCEED();
            }

            static ALWAYS_INLINE Result CheckOffsetAndSizeWithResult(s64 offset, size_t size, Result fail_result) {
                R_RETURN(CheckOffsetAndSizeWithResult(offset, static_cast<s64>(size), fail_result));
            }
    };

    class ReadOnlyStorageAdapter : public IStorage {
        private:
            IStorage &m_storage;
        public:
            ReadOnlyStorageAdapter(IStorage &s) : m_storage(s) { /* ... */ }

            virtual Result Read(s64 offset, void *buffer, size_t size) override {
                R_RETURN(m_storage.Read(offset, buffer, size));
            }

            virtual Result Flush() override {
                R_RETURN(m_storage.Flush());
            }

            virtual Result GetSize(s64 *out) override {
                R_RETURN(m_storage.GetSize(out));
            }

            virtual Result Write(s64 offset, const void *buffer, size_t size) override {
                R_THROW(fs::ResultUnsupportedOperation());
            }

            virtual Result SetSize(s64 size) override {
                R_THROW(fs::ResultUnsupportedOperation());
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
                R_UNLESS(buffer != nullptr, fs::ResultNullptrArgument());
                R_TRY(IStorage::CheckAccessRange(offset, size, m_size));
                R_RETURN(m_storage.Read(m_offset + offset, buffer, size));
            }

            virtual Result Write(s64 offset, const void *buffer, size_t size) override{
                /* Succeed immediately on zero-sized operation. */
                R_SUCCEED_IF(size == 0);

                /* Validate arguments and write. */
                R_UNLESS(buffer != nullptr, fs::ResultNullptrArgument());
                R_TRY(IStorage::CheckAccessRange(offset, size, m_size));
                R_RETURN(m_storage.Write(m_offset + offset, buffer, size));
            }

            virtual Result Flush() override {
                R_RETURN(m_storage.Flush());
            }

            virtual Result GetSize(s64 *out) override {
                *out = m_size;
                R_SUCCEED();
            }

            virtual Result SetSize(s64 size) override {
                R_THROW(fs::ResultUnsupportedSetSizeForNotResizableSubStorage());
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
