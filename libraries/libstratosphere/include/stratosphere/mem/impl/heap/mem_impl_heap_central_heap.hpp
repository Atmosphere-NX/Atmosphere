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
#include <stratosphere/mem/impl/mem_impl_common.hpp>
#include <stratosphere/mem/impl/mem_impl_declarations.hpp>

namespace ams::mem::impl::heap {

    class CachedHeap;
    class TlsHeapCentral;

    using HeapWalkCallback = int (*)(void *ptr, size_t size, void *user_data);

    class CentralHeap final {
        NON_COPYABLE(CentralHeap);
        NON_MOVEABLE(CentralHeap);
        public:
            static constexpr size_t PageSize = 4_KB;
            static constexpr size_t MinimumAlignment = alignof(u64);
            using DestructorHandler = void (*)(void *start, void *end);
        private:
            TlsHeapCentral *m_tls_heap_central;
            bool m_use_virtual_memory;
            u32 m_option;
            u8 *m_start;
            u8 *m_end;
        public:
            constexpr CentralHeap() : m_tls_heap_central(), m_use_virtual_memory(), m_option(), m_start(), m_end() { /* ... */ }
            ~CentralHeap() { this->Finalize(); }

            errno_t Initialize(void *start, size_t size, u32 option);
            void    Finalize();

            ALWAYS_INLINE void *Allocate(size_t n) { return this->Allocate(n, MinimumAlignment); }
            void *Allocate(size_t n, size_t align);
            size_t  GetAllocationSize(const void *ptr);
            errno_t Free(void *p);
            errno_t FreeWithSize(void *p, size_t size);
            errno_t Reallocate(void *ptr, size_t size, void **p);
            errno_t Shrink(void *ptr, size_t size);

            bool MakeCache(CachedHeap *cached_heap);
            errno_t WalkAllocatedPointers(HeapWalkCallback callback, void *user_data);
            errno_t QueryV(int query, std::va_list vl);
            errno_t Query(int query, ...);
        private:
            errno_t QueryVImpl(int query, std::va_list *vl_ptr);
    };

    static_assert(sizeof(CentralHeap) <= sizeof(::ams::mem::impl::InternalCentralHeapStorage));
    static_assert(alignof(CentralHeap) <= alignof(void *));

}
