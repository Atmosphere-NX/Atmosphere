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
#include <stratosphere.hpp>

namespace ams::fssystem {

    class MemoryResourceBufferHoldStorage : public ::ams::fs::IStorage, public ::ams::fs::impl::Newable {
        NON_COPYABLE(MemoryResourceBufferHoldStorage);
        NON_MOVEABLE(MemoryResourceBufferHoldStorage);
        private:
            std::shared_ptr<fs::IStorage> m_storage;
            MemoryResource *m_memory_resource;
            void *m_buffer;
            size_t m_buffer_size;
        public:
            MemoryResourceBufferHoldStorage(std::shared_ptr<fs::IStorage> storage, MemoryResource *mr, size_t buffer_size) : m_storage(std::move(storage)), m_memory_resource(mr), m_buffer(m_memory_resource->Allocate(buffer_size)), m_buffer_size(buffer_size) {
                /* ... */
            }

            virtual ~MemoryResourceBufferHoldStorage() {
                /* If we have a buffer, deallocate it. */
                if (m_buffer != nullptr) {
                    m_memory_resource->Deallocate(m_buffer, m_buffer_size);
                }
            }

            ALWAYS_INLINE bool IsValid() const { return m_buffer != nullptr; }
            ALWAYS_INLINE void *GetBuffer() const { return m_buffer; }
        public:
            virtual Result Read(s64 offset, void *buffer, size_t size) override {
                /* Check pre-conditions. */
                AMS_ASSERT(m_storage != nullptr);

                R_RETURN(m_storage->Read(offset, buffer, size));
            }

            virtual Result OperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override {
                /* Check pre-conditions. */
                AMS_ASSERT(m_storage != nullptr);

                R_RETURN(m_storage->OperateRange(dst, dst_size, op_id, offset, size, src, src_size));
            }

            virtual Result GetSize(s64 *out) override {
                /* Check pre-conditions. */
                AMS_ASSERT(m_storage != nullptr);

                R_RETURN(m_storage->GetSize(out));
            }

            virtual Result Flush() override {
                /* Check pre-conditions. */
                AMS_ASSERT(m_storage != nullptr);

                R_RETURN(m_storage->Flush());
            }

            virtual Result Write(s64 offset, const void *buffer, size_t size) override {
                /* Check pre-conditions. */
                AMS_ASSERT(m_storage != nullptr);

                R_RETURN(m_storage->Write(offset, buffer, size));
            }

            virtual Result SetSize(s64 size) override {
                /* Check pre-conditions. */
                AMS_ASSERT(m_storage != nullptr);

                R_RETURN(m_storage->SetSize(size));
            }
    };

}
