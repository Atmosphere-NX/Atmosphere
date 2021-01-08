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
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_k_slab_heap.hpp>
#include <mesosphere/kern_k_page_group.hpp>
#include <mesosphere/kern_k_memory_block.hpp>
#include <mesosphere/kern_k_dynamic_page_manager.hpp>

namespace ams::kern {

    template<typename T>
    class KDynamicSlabHeap {
        NON_COPYABLE(KDynamicSlabHeap);
        NON_MOVEABLE(KDynamicSlabHeap);
        private:
            using Impl       = impl::KSlabHeapImpl;
            using PageBuffer = KDynamicPageManager::PageBuffer;
        private:
            Impl m_impl;
            KDynamicPageManager *m_page_allocator;
            std::atomic<size_t> m_used;
            std::atomic<size_t> m_peak;
            std::atomic<size_t> m_count;
            KVirtualAddress m_address;
            size_t m_size;
        private:
            ALWAYS_INLINE Impl *GetImpl() {
                return std::addressof(m_impl);
            }
            ALWAYS_INLINE const Impl *GetImpl() const {
                return std::addressof(m_impl);
            }
        public:
            constexpr KDynamicSlabHeap() : m_impl(), m_page_allocator(), m_used(), m_peak(), m_count(), m_address(), m_size() { /* ... */ }

            constexpr KVirtualAddress GetAddress() const { return m_address; }
            constexpr size_t GetSize() const { return m_size; }
            constexpr size_t GetUsed() const { return m_used.load(); }
            constexpr size_t GetPeak() const { return m_peak.load(); }
            constexpr size_t GetCount() const { return m_count.load(); }

            constexpr bool IsInRange(KVirtualAddress addr) const {
                return this->GetAddress() <= addr && addr <= this->GetAddress() + this->GetSize() - 1;
            }

            void Initialize(KVirtualAddress memory, size_t sz) {
                /* Set tracking fields. */
                m_address = memory;
                m_count   = sz / sizeof(T);
                m_size    = m_count * sizeof(T);

                /* Free blocks to memory. */
                u8 *cur = GetPointer<u8>(m_address + m_size);
                for (size_t i = 0; i < sz / sizeof(T); i++) {
                    cur -= sizeof(T);
                    this->GetImpl()->Free(cur);
                }
            }

            void Initialize(KDynamicPageManager *page_allocator) {
                m_page_allocator = page_allocator;
                m_address        = m_page_allocator->GetAddress();
                m_size           = m_page_allocator->GetSize();
            }

            void Initialize(KDynamicPageManager *page_allocator, size_t num_objects) {
                MESOSPHERE_ASSERT(page_allocator != nullptr);

                /* Initialize members. */
                this->Initialize(page_allocator);

                /* Allocate until we have the correct number of objects. */
                while (m_count.load() < num_objects) {
                    auto *allocated = reinterpret_cast<T *>(m_page_allocator->Allocate());
                    MESOSPHERE_ABORT_UNLESS(allocated != nullptr);
                    for (size_t i = 0; i < sizeof(PageBuffer) / sizeof(T); i++) {
                        this->GetImpl()->Free(allocated + i);
                    }
                    m_count.fetch_add(sizeof(PageBuffer) / sizeof(T));
                }
            }

            T *Allocate() {
                T *allocated = reinterpret_cast<T *>(this->GetImpl()->Allocate());

                /* If we fail to allocate, try to get a new page from our next allocator. */
                if (AMS_UNLIKELY(allocated == nullptr)) {
                    if (m_page_allocator != nullptr) {
                        allocated = reinterpret_cast<T *>(m_page_allocator->Allocate());
                        if (allocated != nullptr) {
                            /* If we succeeded in getting a page, free the rest to our slab. */
                            for (size_t i = 1; i < sizeof(PageBuffer) / sizeof(T); i++) {
                                this->GetImpl()->Free(allocated + i);
                            }
                            m_count.fetch_add(sizeof(PageBuffer) / sizeof(T));
                        }
                    }
                }

                if (AMS_LIKELY(allocated != nullptr)) {
                    /* Construct the object. */
                    new (allocated) T();

                    /* Update our tracking. */
                    size_t used = m_used.fetch_add(1) + 1;
                    size_t peak = m_peak.load();
                    while (peak < used) {
                        if (m_peak.compare_exchange_weak(peak, used, std::memory_order_relaxed)) {
                            break;
                        }
                    }
                }

                return allocated;
            }

            void Free(T *t) {
                this->GetImpl()->Free(t);
                m_used.fetch_sub(1);
            }
    };

    class KBlockInfoManager       : public KDynamicSlabHeap<KBlockInfo>{};
    class KMemoryBlockSlabManager : public KDynamicSlabHeap<KMemoryBlock>{};

}
