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
#include <stratosphere/fs/fs_operate_range.hpp>

namespace ams::fs {

    /* ACCURATE_TO_VERSION: 14.3.0.0 */
    class IStorage {
        public:
            virtual ~IStorage() { /* ... */ }

            virtual Result Read(s64 offset, void *buffer, size_t size) = 0;

            virtual Result Write(s64 offset, const void *buffer, size_t size) = 0;

            virtual Result Flush() = 0;

            virtual Result SetSize(s64 size) = 0;

            virtual Result GetSize(s64 *out) = 0;

            virtual Result OperateRange(void *dst, size_t dst_size, OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) = 0;

            virtual Result OperateRange(OperationId op_id, s64 offset, s64 size) {
                R_RETURN(this->OperateRange(nullptr, 0, op_id, offset, size, nullptr, 0));
            }
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
            std::shared_ptr<IStorage> m_shared_storage;
            std::unique_ptr<IStorage> m_unique_storage;
            IStorage *m_storage;
        public:
            explicit ReadOnlyStorageAdapter(IStorage *s) : m_unique_storage(s) {
                m_storage = m_unique_storage.get();
            }
            explicit ReadOnlyStorageAdapter(std::shared_ptr<IStorage> s) : m_shared_storage(s) {
                m_storage = m_shared_storage.get();
            }
            explicit ReadOnlyStorageAdapter(std::unique_ptr<IStorage> s) : m_unique_storage(std::move(s)) {
                m_storage = m_unique_storage.get();
            }

            virtual ~ReadOnlyStorageAdapter() { /* ... */ }
        public:
            virtual Result Read(s64 offset, void *buffer, size_t size) override {
                R_RETURN(m_storage->Read(offset, buffer, size));
            }

            virtual Result Flush() override {
                R_RETURN(m_storage->Flush());
            }

            virtual Result GetSize(s64 *out) override {
                R_RETURN(m_storage->GetSize(out));
            }

            virtual Result OperateRange(void *dst, size_t dst_size, OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override {
                R_RETURN(m_storage->OperateRange(dst, dst_size, op_id, offset, size, src, src_size));
            }

            virtual Result Write(s64 offset, const void *buffer, size_t size) override {
                /* TODO: Better result? Is it possible to get a more specific one? */
                AMS_UNUSED(offset, buffer, size);
                R_THROW(fs::ResultUnsupportedOperation());
            }

            virtual Result SetSize(s64 size) override {
                /* TODO: Better result? Is it possible to get a more specific one? */
                AMS_UNUSED(size);
                R_THROW(fs::ResultUnsupportedOperation());
            }
    };

    template<typename T>
    concept PointerToStorage = ::ams::util::RawOrSmartPointerTo<T, ::ams::fs::IStorage>;

}
