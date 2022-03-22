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
#include <mesosphere/kern_k_dynamic_slab_heap.hpp>

namespace ams::kern {

    template<typename T, bool ClearNode = false>
    class KDynamicResourceManager {
        NON_COPYABLE(KDynamicResourceManager);
        NON_MOVEABLE(KDynamicResourceManager);
        public:
            using DynamicSlabType = KDynamicSlabHeap<T, ClearNode>;
        private:
            KDynamicPageManager *m_page_allocator{};
            DynamicSlabType *m_slab_heap{};
        public:
            constexpr KDynamicResourceManager() = default;

            constexpr ALWAYS_INLINE KVirtualAddress GetAddress() const { return m_slab_heap->GetAddress(); }
            constexpr ALWAYS_INLINE size_t GetSize() const { return m_slab_heap->GetSize(); }
            constexpr ALWAYS_INLINE size_t GetUsed() const { return m_slab_heap->GetUsed(); }
            constexpr ALWAYS_INLINE size_t GetPeak() const { return m_slab_heap->GetPeak(); }
            constexpr ALWAYS_INLINE size_t GetCount() const { return m_slab_heap->GetCount(); }

            ALWAYS_INLINE void Initialize(KDynamicPageManager *page_allocator, DynamicSlabType *slab_heap) {
                m_page_allocator = page_allocator;
                m_slab_heap      = slab_heap;
            }

            T *Allocate() const {
                return m_slab_heap->Allocate(m_page_allocator);
            }

            ALWAYS_INLINE void Free(T *t) const {
                m_slab_heap->Free(t);
            }
    };

    class KBlockInfoManager       : public KDynamicResourceManager<KBlockInfo>{};
    class KMemoryBlockSlabManager : public KDynamicResourceManager<KMemoryBlock>{};

    using KBlockInfoSlabHeap   = typename KBlockInfoManager::DynamicSlabType;
    using KMemoryBlockSlabHeap = typename KMemoryBlockSlabManager::DynamicSlabType;

}
