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
#include <stratosphere/os.hpp>
#include <stratosphere/lmem/lmem_exp_heap.hpp>

namespace ams::fssrv {

    class MemoryResourceFromExpHeap : public ams::MemoryResource {
        private:
            lmem::HeapHandle m_heap_handle;
        public:
            constexpr explicit MemoryResourceFromExpHeap(lmem::HeapHandle handle) : m_heap_handle(handle) { /* ... */ }
        protected:
            virtual void *AllocateImpl(size_t size, size_t align) override {
                return lmem::AllocateFromExpHeap(m_heap_handle, size, static_cast<s32>(align));
            }

            virtual void DeallocateImpl(void *p, size_t size, size_t align) override {
                AMS_UNUSED(size, align);
                return lmem::FreeToExpHeap(m_heap_handle, p);
            }

            virtual bool IsEqualImpl(const MemoryResource &rhs) const override {
                AMS_UNUSED(rhs);
                return false;
            }
    };

    class PeakCheckableMemoryResourceFromExpHeap : public ams::MemoryResource {
        private:
            lmem::HeapHandle m_heap_handle;
            os::SdkMutex m_mutex;
            size_t m_peak_free_size;
            size_t m_current_free_size;
        public:
            constexpr explicit PeakCheckableMemoryResourceFromExpHeap(size_t heap_size) : m_heap_handle(nullptr), m_mutex(), m_peak_free_size(heap_size), m_current_free_size(heap_size) { /* ... */ }

            void SetHeapHandle(lmem::HeapHandle handle) {
                m_heap_handle = handle;
            }

            size_t GetPeakFreeSize() const { return m_peak_free_size; }
            size_t GetCurrentFreeSize() const { return m_current_free_size; }

            void ClearPeak() { m_peak_free_size = m_current_free_size; }

            std::scoped_lock<os::SdkMutex> GetScopedLock() {
                return std::scoped_lock(m_mutex);
            }

            void OnAllocate(void *p, size_t size);
            void OnDeallocate(void *p, size_t size);
        protected:
            virtual void *AllocateImpl(size_t size, size_t align) override;
            virtual void DeallocateImpl(void *p, size_t size, size_t align) override;
            virtual bool IsEqualImpl(const MemoryResource &rhs) const override {
                AMS_UNUSED(rhs);
                return false;
            }
    };

}