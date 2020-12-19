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
#include <mesosphere/kern_slab_helpers.hpp>
#include <mesosphere/kern_k_dynamic_slab_heap.hpp>

namespace ams::kern {

    namespace impl {

        class PageTablePage {
            private:
                u8 m_buffer[PageSize];
        };
        static_assert(sizeof(PageTablePage) == PageSize);

    }

    class KPageTableManager : public KDynamicSlabHeap<impl::PageTablePage> {
        public:
            using RefCount = u16;
            static constexpr size_t PageTableSize = sizeof(impl::PageTablePage);
            static_assert(PageTableSize == PageSize);
        private:
            using BaseHeap = KDynamicSlabHeap<impl::PageTablePage>;
        private:
            RefCount *m_ref_counts;
        public:
            static constexpr size_t CalculateReferenceCountSize(size_t size) {
                return (size / PageSize) * sizeof(RefCount);
            }
        public:
            constexpr KPageTableManager() : BaseHeap(), m_ref_counts() { /* ... */ }
        private:
            void Initialize(RefCount *rc) {
                m_ref_counts = rc;
                for (size_t i = 0; i < this->GetSize() / PageSize; i++) {
                    m_ref_counts[i] = 0;
                }
            }

            constexpr RefCount *GetRefCountPointer(KVirtualAddress addr) const {
                return std::addressof(m_ref_counts[(addr - this->GetAddress()) / PageSize]);
            }
        public:
            void Initialize(KDynamicPageManager *page_allocator, RefCount *rc) {
                BaseHeap::Initialize(page_allocator);
                this->Initialize(rc);
            }

            void Initialize(KDynamicPageManager *page_allocator, size_t object_count, RefCount *rc) {
                BaseHeap::Initialize(page_allocator, object_count);
                this->Initialize(rc);
            }

            KVirtualAddress Allocate() {
                return KVirtualAddress(BaseHeap::Allocate());
            }

            void Free(KVirtualAddress addr) {
                /* Ensure all pages in the heap are zero. */
                cpu::ClearPageToZero(GetVoidPointer(addr));

                /* Free the page. */
                BaseHeap::Free(GetPointer<impl::PageTablePage>(addr));
            }

            RefCount GetRefCount(KVirtualAddress addr) const {
                MESOSPHERE_ASSERT(this->IsInRange(addr));
                return *this->GetRefCountPointer(addr);
            }

            void Open(KVirtualAddress addr, int count) {
                MESOSPHERE_ASSERT(this->IsInRange(addr));

                *this->GetRefCountPointer(addr) += count;

                MESOSPHERE_ABORT_UNLESS(this->GetRefCount(addr) > 0);
            }

            bool Close(KVirtualAddress addr, int count) {
                MESOSPHERE_ASSERT(this->IsInRange(addr));
                MESOSPHERE_ABORT_UNLESS(this->GetRefCount(addr) >= count);

                *this->GetRefCountPointer(addr) -= count;
                return this->GetRefCount(addr) == 0;
            }

            constexpr bool IsInPageTableHeap(KVirtualAddress addr) const {
                return this->IsInRange(addr);
            }
    };

}
