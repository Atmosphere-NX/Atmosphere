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
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_k_slab_heap.hpp>
#include <mesosphere/kern_k_page_group.hpp>
#include <mesosphere/kern_k_memory_block.hpp>
#include <mesosphere/kern_k_dynamic_page_manager.hpp>

namespace ams::kern {

    template<typename T, bool ClearNode = false>
    class KDynamicSlabHeap : protected impl::KSlabHeapImpl {
        NON_COPYABLE(KDynamicSlabHeap);
        NON_MOVEABLE(KDynamicSlabHeap);
        private:
            using PageBuffer = KDynamicPageManager::PageBuffer;
        private:
            util::Atomic<size_t> m_used{0};
            util::Atomic<size_t> m_peak{0};
            util::Atomic<size_t> m_count{0};
            KVirtualAddress m_address{Null<KVirtualAddress>};
            size_t m_size{};
        public:
            constexpr KDynamicSlabHeap() = default;

            constexpr ALWAYS_INLINE KVirtualAddress GetAddress() const { return m_address; }
            constexpr ALWAYS_INLINE size_t GetSize() const { return m_size; }
            constexpr ALWAYS_INLINE size_t GetUsed() const { return m_used.Load(); }
            constexpr ALWAYS_INLINE size_t GetPeak() const { return m_peak.Load(); }
            constexpr ALWAYS_INLINE size_t GetCount() const { return m_count.Load(); }

            constexpr ALWAYS_INLINE bool IsInRange(KVirtualAddress addr) const {
                return this->GetAddress() <= addr && addr <= this->GetAddress() + this->GetSize() - 1;
            }

            ALWAYS_INLINE void Initialize(KDynamicPageManager *page_allocator, size_t num_objects) {
                MESOSPHERE_ASSERT(page_allocator != nullptr);

                /* Initialize members. */
                m_address = page_allocator->GetAddress();
                m_size    = page_allocator->GetSize();

                /* Initialize the base allocator. */
                KSlabHeapImpl::Initialize();

                /* Allocate until we have the correct number of objects. */
                while (m_count.Load() < num_objects) {
                    auto *allocated = reinterpret_cast<T *>(page_allocator->Allocate());
                    MESOSPHERE_ABORT_UNLESS(allocated != nullptr);

                    for (size_t i = 0; i < sizeof(PageBuffer) / sizeof(T); i++) {
                        KSlabHeapImpl::Free(allocated + i);
                    }

                    m_count += sizeof(PageBuffer) / sizeof(T);
                }
            }

            ALWAYS_INLINE T *Allocate(KDynamicPageManager *page_allocator) {
                T *allocated = static_cast<T *>(KSlabHeapImpl::Allocate());

                /* If we successfully allocated and we should clear the node, do so. */
                if constexpr (ClearNode) {
                    if (AMS_LIKELY(allocated != nullptr)) {
                        reinterpret_cast<KSlabHeapImpl::Node *>(allocated)->next = nullptr;
                    }
                }

                /* If we fail to allocate, try to get a new page from our next allocator. */
                if (AMS_UNLIKELY(allocated == nullptr) ) {
                    if (page_allocator != nullptr) {
                        allocated = reinterpret_cast<T *>(page_allocator->Allocate());
                        if (allocated != nullptr) {
                            /* If we succeeded in getting a page, free the rest to our slab. */
                            for (size_t i = 1; i < sizeof(PageBuffer) / sizeof(T); i++) {
                                KSlabHeapImpl::Free(allocated + i);
                            }
                            m_count += sizeof(PageBuffer) / sizeof(T);
                        }
                    }
                }

                if (AMS_LIKELY(allocated != nullptr)) {
                    /* Construct the object. */
                    std::construct_at(allocated);

                    /* Update our tracking. */
                    const size_t used = ++m_used;
                    size_t peak = m_peak.Load();
                    while (peak < used) {
                        if (m_peak.CompareExchangeWeak<std::memory_order_relaxed>(peak, used)) {
                            break;
                        }
                    }
                }

                return allocated;
            }

            ALWAYS_INLINE void Free(T *t) {
                KSlabHeapImpl::Free(t);
                --m_used;
            }
    };

}
