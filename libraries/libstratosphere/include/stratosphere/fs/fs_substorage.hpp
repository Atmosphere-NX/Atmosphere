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
#include <stratosphere/fs/impl/fs_newable.hpp>
#include <stratosphere/fs/fs_istorage.hpp>

namespace ams::fs {

    class SubStorage : public ::ams::fs::IStorage, public ::ams::fs::impl::Newable {
        private:
            std::shared_ptr<IStorage> m_shared_base_storage;
            fs::IStorage *m_base_storage;
            s64 m_offset;
            s64 m_size;
            bool m_resizable;
        private:
            constexpr bool IsValid() const {
                return m_base_storage != nullptr;
            }
        public:
            SubStorage() : m_shared_base_storage(), m_base_storage(nullptr), m_offset(0), m_size(0), m_resizable(false) { /* ... */ }

            SubStorage(const SubStorage &rhs) : m_shared_base_storage(), m_base_storage(rhs.m_base_storage), m_offset(rhs.m_offset), m_size(rhs.m_size), m_resizable(rhs.m_resizable) { /* ... */}
            SubStorage &operator=(const SubStorage &rhs) {
                if (this != std::addressof(rhs)) {
                    m_base_storage = rhs.m_base_storage;
                    m_offset       = rhs.m_offset;
                    m_size         = rhs.m_size;
                    m_resizable    = rhs.m_resizable;
                }
                return *this;
            }

            SubStorage(IStorage *storage, s64 o, s64 sz) : m_shared_base_storage(), m_base_storage(storage), m_offset(o), m_size(sz), m_resizable(false) {
                AMS_ABORT_UNLESS(this->IsValid());
                AMS_ABORT_UNLESS(m_offset  >= 0);
                AMS_ABORT_UNLESS(m_size    >= 0);
            }

            SubStorage(std::shared_ptr<IStorage> storage, s64 o, s64 sz) : m_shared_base_storage(storage), m_base_storage(storage.get()), m_offset(o), m_size(sz), m_resizable(false) {
                AMS_ABORT_UNLESS(this->IsValid());
                AMS_ABORT_UNLESS(m_offset  >= 0);
                AMS_ABORT_UNLESS(m_size    >= 0);
            }

            SubStorage(SubStorage *sub, s64 o, s64 sz) : m_shared_base_storage(), m_base_storage(sub->m_base_storage), m_offset(o + sub->m_offset), m_size(sz), m_resizable(false) {
                AMS_ABORT_UNLESS(this->IsValid());
                AMS_ABORT_UNLESS(m_offset    >= 0);
                AMS_ABORT_UNLESS(m_size      >= 0);
                AMS_ABORT_UNLESS(sub->m_size >= o + sz);
            }

        public:
            void SetResizable(bool rsz) {
                m_resizable = rsz;
            }
        public:
            virtual Result Read(s64 offset, void *buffer, size_t size) override {
                /* Ensure we're initialized. */
                R_UNLESS(this->IsValid(),                                  fs::ResultNotInitialized());

                /* Succeed immediately on zero-sized operation. */
                R_SUCCEED_IF(size == 0);


                /* Validate arguments and read. */
                R_UNLESS(buffer != nullptr,                                fs::ResultNullptrArgument());
                R_UNLESS(IStorage::CheckAccessRange(offset, size, m_size), fs::ResultOutOfRange());
                return m_base_storage->Read(m_offset + offset, buffer, size);
            }

            virtual Result Write(s64 offset, const void *buffer, size_t size) override{
                /* Ensure we're initialized. */
                R_UNLESS(this->IsValid(),                                  fs::ResultNotInitialized());

                /* Succeed immediately on zero-sized operation. */
                R_SUCCEED_IF(size == 0);

                /* Validate arguments and write. */
                R_UNLESS(buffer != nullptr,                                fs::ResultNullptrArgument());
                R_UNLESS(IStorage::CheckAccessRange(offset, size, m_size), fs::ResultOutOfRange());
                return m_base_storage->Write(m_offset + offset, buffer, size);
            }

            virtual Result Flush() override {
                R_UNLESS(this->IsValid(), fs::ResultNotInitialized());
                return m_base_storage->Flush();
            }

            virtual Result SetSize(s64 size) override {
                /* Ensure we're initialized and validate arguments. */
                R_UNLESS(this->IsValid(),                              fs::ResultNotInitialized());
                R_UNLESS(m_resizable,                                  fs::ResultUnsupportedOperationInSubStorageA());
                R_UNLESS(IStorage::CheckOffsetAndSize(m_offset, size), fs::ResultInvalidSize());

                /* Ensure that we're allowed to set size. */
                s64 cur_size;
                R_TRY(m_base_storage->GetSize(std::addressof(cur_size)));
                R_UNLESS(cur_size == m_offset + m_size, fs::ResultUnsupportedOperationInSubStorageB());

                /* Set the size. */
                R_TRY(m_base_storage->SetSize(m_offset + size));

                m_size = size;
                return ResultSuccess();
            }

            virtual Result GetSize(s64 *out) override {
                /* Ensure we're initialized. */
                R_UNLESS(this->IsValid(), fs::ResultNotInitialized());

                *out = m_size;
                return ResultSuccess();
            }

            virtual Result OperateRange(void *dst, size_t dst_size, OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override {
                /* Ensure we're initialized. */
                R_UNLESS(this->IsValid(),                              fs::ResultNotInitialized());

                /* Succeed immediately on zero-sized operation. */
                R_SUCCEED_IF(size == 0);

                /* Validate arguments and operate. */
                R_UNLESS(IStorage::CheckOffsetAndSize(offset, size), fs::ResultOutOfRange());
                return m_base_storage->OperateRange(dst, dst_size, op_id, m_offset + offset, size, src, src_size);
            }

            using IStorage::OperateRange;
    };

}
