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
#include <mesosphere/kern_slab_helpers.hpp>
#include <mesosphere/kern_k_memory_layout.hpp>
#include <mesosphere/kern_k_dynamic_page_manager.hpp>

namespace ams::kern {

    class KDynamicPageManager;

    class KPageBuffer;

    class KPageBufferSlabHeap : protected impl::KSlabHeapImpl {
        public:
            static constexpr size_t BufferSize            = PageSize;
            static constinit inline size_t s_buffer_count = 0;
        private:
            size_t m_obj_size{};
        public:
            constexpr KPageBufferSlabHeap() = default;

            /* See kern_init_slab_setup.cpp for definition. */
            void Initialize(KDynamicPageManager &allocator);

            KPageBuffer *Allocate();
            void Free(KPageBuffer *pb);
    };

    class KPageBuffer {
        private:
            u8 m_buffer[KPageBufferSlabHeap::BufferSize];
        public:
            KPageBuffer() {
                std::memset(m_buffer, 0, sizeof(m_buffer));
            }

            ALWAYS_INLINE KPhysicalAddress GetPhysicalAddress() const {
                return KMemoryLayout::GetLinearPhysicalAddress(KVirtualAddress(this));
            }

            static ALWAYS_INLINE KPageBuffer *FromPhysicalAddress(KPhysicalAddress phys_addr) {
                const KVirtualAddress virt_addr = KMemoryLayout::GetLinearVirtualAddress(phys_addr);

                MESOSPHERE_ASSERT(util::IsAligned(GetInteger(phys_addr), PageSize));
                MESOSPHERE_ASSERT(util::IsAligned(GetInteger(virt_addr), PageSize));

                return GetPointer<KPageBuffer>(virt_addr);
            }
        private:
            static constinit inline KPageBufferSlabHeap s_slab_heap;
        public:
            static void InitializeSlabHeap(KDynamicPageManager &allocator) {
                s_slab_heap.Initialize(allocator);
            }

            static KPageBuffer *Allocate() {
                return s_slab_heap.Allocate();
            }

            static void Free(KPageBuffer *obj) {
                s_slab_heap.Free(obj);
            }

            template<size_t ExpectedSize>
            static ALWAYS_INLINE KPageBuffer *AllocateChecked() {
                /* Check that the allocation is valid. */
                MESOSPHERE_ABORT_UNLESS(sizeof(KPageBuffer) == ExpectedSize);

                return Allocate();
            }

            template<size_t ExpectedSize>
            static ALWAYS_INLINE void FreeChecked(KPageBuffer *obj) {
                /* Check that the free is valid. */
                MESOSPHERE_ABORT_UNLESS(sizeof(KPageBuffer) == ExpectedSize);

                return Free(obj);
            }
    };
    static_assert(sizeof(KPageBuffer) == KPageBufferSlabHeap::BufferSize);

    ALWAYS_INLINE KPageBuffer *KPageBufferSlabHeap::Allocate() {
        KPageBuffer *pb = static_cast<KPageBuffer *>(KSlabHeapImpl::Allocate());
        if (AMS_LIKELY(pb != nullptr)) {
            std::construct_at(pb);
        }
        return pb;
    }

    ALWAYS_INLINE void KPageBufferSlabHeap::Free(KPageBuffer *pb) {
        KSlabHeapImpl::Free(pb);
    }

}
