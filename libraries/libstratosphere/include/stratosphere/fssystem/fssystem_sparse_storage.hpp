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
#include <vapours.hpp>
#include <stratosphere/fssystem/fssystem_indirect_storage.hpp>

namespace ams::fssystem {

    /* ACCURATE_TO_VERSION: Unknown */

    class SparseStorage : public IndirectStorage {
        NON_COPYABLE(SparseStorage);
        NON_MOVEABLE(SparseStorage);
        private:
            class ZeroStorage : public ::ams::fs::IStorage, public ::ams::fs::impl::Newable {
                public:
                    ZeroStorage() { /* ... */ }
                    virtual ~ZeroStorage() { /* ... */ }
                public:
                    virtual Result Read(s64 offset, void *buffer, size_t size) override {
                        AMS_ASSERT(offset >= 0);
                        AMS_ASSERT(buffer != nullptr || size == 0);
                        AMS_UNUSED(offset);

                        if (size > 0) {
                            std::memset(buffer, 0, size);
                        }
                        R_SUCCEED();
                    }

                    virtual Result OperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override {
                        AMS_UNUSED(dst, dst_size, op_id, offset, size, src, src_size);
                        R_SUCCEED();
                    }

                    virtual Result GetSize(s64 *out) override {
                        AMS_ASSERT(out != nullptr);
                        *out = std::numeric_limits<s64>::max();
                        R_SUCCEED();
                    }

                    virtual Result Flush() override {
                        R_SUCCEED();
                    }

                    virtual Result Write(s64 offset, const void *buffer, size_t size) override {
                        AMS_UNUSED(offset, buffer, size);
                        R_THROW(fs::ResultUnsupportedWriteForZeroStorage());
                    }

                    virtual Result SetSize(s64 size) override {
                        AMS_UNUSED(size);
                        R_THROW(fs::ResultUnsupportedSetSizeForZeroStorage());
                    }
            };
        private:
            ZeroStorage m_zero_storage;
        public:
            SparseStorage() : IndirectStorage(), m_zero_storage() { /* ... */ }
            virtual ~SparseStorage() { /* ... */ }

            using IndirectStorage::Initialize;

            void Initialize(s64 end_offset) {
                this->GetEntryTable().Initialize(NodeSize, end_offset);
                this->SetZeroStorage();
            }

            void SetDataStorage(fs::SubStorage storage) {
                AMS_ASSERT(this->IsInitialized());

                this->SetStorage(0, storage);
                this->SetZeroStorage();
            }

            template<typename T>
            void SetDataStorage(T storage, s64 offset, s64 size) {
                AMS_ASSERT(this->IsInitialized());

                this->SetStorage(0, storage, offset, size);
                this->SetZeroStorage();
            }

            virtual Result Read(s64 offset, void *buffer, size_t size) override;
        private:
            void SetZeroStorage() {
                return this->SetStorage(1, std::addressof(m_zero_storage), 0, std::numeric_limits<s64>::max());
            }
    };

}
