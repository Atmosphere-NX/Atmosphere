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
#include <stratosphere/sf/sf_common.hpp>
#include <stratosphere/lmem.hpp>

namespace ams::sf {

    class ExpHeapMemoryResource : public MemoryResource {
        private:
            lmem::HeapHandle m_handle;
        public:
            constexpr ExpHeapMemoryResource() : m_handle() { /* ... */ }
            constexpr explicit ExpHeapMemoryResource(lmem::HeapHandle h) : m_handle(h) { /* ... */ }

            void Attach(lmem::HeapHandle h) {
                AMS_ABORT_UNLESS(m_handle == lmem::HeapHandle());
                m_handle = h;
            }

            lmem::HeapHandle GetHandle() const { return m_handle; }
        private:
            virtual void *AllocateImpl(size_t size, size_t alignment) override {
                return lmem::AllocateFromExpHeap(m_handle, size, static_cast<int>(alignment));
            }

            virtual void DeallocateImpl(void *buffer, size_t size, size_t alignment) override {
                AMS_UNUSED(size, alignment);
                return lmem::FreeToExpHeap(m_handle, buffer);
            }

            virtual bool IsEqualImpl(const MemoryResource &resource) const override {
                return this == std::addressof(resource);
            }
    };

    class UnitHeapMemoryResource : public MemoryResource {
        private:
            lmem::HeapHandle m_handle;
        public:
            constexpr UnitHeapMemoryResource() : m_handle() { /* ... */ }
            constexpr explicit UnitHeapMemoryResource(lmem::HeapHandle h) : m_handle(h) { /* ... */ }

            void Attach(lmem::HeapHandle h) {
                AMS_ABORT_UNLESS(m_handle == lmem::HeapHandle());
                m_handle = h;
            }

            lmem::HeapHandle GetHandle() const { return m_handle; }
        private:
            virtual void *AllocateImpl(size_t size, size_t alignment) override {
                AMS_ASSERT(size <= lmem::GetUnitHeapUnitSize(m_handle));
                AMS_ASSERT(alignment <= static_cast<size_t>(lmem::GetUnitHeapAlignment(m_handle)));
                AMS_UNUSED(size, alignment);

                return lmem::AllocateFromUnitHeap(m_handle);
            }

            virtual void DeallocateImpl(void *buffer, size_t size, size_t alignment) override {
                AMS_UNUSED(size, alignment);

                return lmem::FreeToUnitHeap(m_handle, buffer);
            }

            virtual bool IsEqualImpl(const MemoryResource &resource) const override {
                return this == std::addressof(resource);
            }
    };

}
