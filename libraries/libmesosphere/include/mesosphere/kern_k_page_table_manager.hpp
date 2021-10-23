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
#include <mesosphere/kern_k_page_table_slab_heap.hpp>
#include <mesosphere/kern_k_dynamic_resource_manager.hpp>

namespace ams::kern {

    class KPageTableManager : public KDynamicResourceManager<impl::PageTablePage, true> {
        public:
            using RefCount = KPageTableSlabHeap::RefCount;
            static constexpr size_t PageTableSize = KPageTableSlabHeap::PageTableSize;
        private:
            using BaseHeap = KDynamicResourceManager<impl::PageTablePage, true>;
        private:
            KPageTableSlabHeap *m_pt_heap;
        public:
            constexpr explicit KPageTableManager(util::ConstantInitializeTag) : m_pt_heap() { /* ... */ }
            explicit KPageTableManager() { /* ... */ }

            ALWAYS_INLINE void Initialize(KDynamicPageManager *page_allocator, KPageTableSlabHeap *pt_heap) {
                m_pt_heap = pt_heap;

                static_assert(std::derived_from<KPageTableSlabHeap, DynamicSlabType>);
                BaseHeap::Initialize(page_allocator, pt_heap);
            }

            KVirtualAddress Allocate() {
                return KVirtualAddress(BaseHeap::Allocate());
            }

            void Free(KVirtualAddress addr) {
                return BaseHeap::Free(GetPointer<impl::PageTablePage>(addr));
            }

            ALWAYS_INLINE RefCount GetRefCount(KVirtualAddress addr) const {
                return m_pt_heap->GetRefCount(addr);
            }

            ALWAYS_INLINE void Open(KVirtualAddress addr, int count) {
                return m_pt_heap->Open(addr, count);
            }

            ALWAYS_INLINE bool Close(KVirtualAddress addr, int count) {
                return m_pt_heap->Close(addr, count);
            }

            constexpr ALWAYS_INLINE bool IsInPageTableHeap(KVirtualAddress addr) const {
                return m_pt_heap->IsInRange(addr);
            }
    };

}
