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

namespace ams::erpt::srv {

    extern lmem::HeapHandle g_heap_handle;

    class Allocator {
        public:
            void *operator new(size_t sz) noexcept { return lmem::AllocateFromExpHeap(g_heap_handle, sz); }
            void *operator new(size_t sz, size_t algn) noexcept { return lmem::AllocateFromExpHeap(g_heap_handle, sz, static_cast<s32>(algn)); }
            void *operator new[](size_t sz) noexcept { return lmem::AllocateFromExpHeap(g_heap_handle, sz); }
            void *operator new[](size_t sz, size_t algn) noexcept { return lmem::AllocateFromExpHeap(g_heap_handle, sz, static_cast<s32>(algn)); }

            void operator delete(void *p) noexcept { lmem::FreeToExpHeap(g_heap_handle, p); }
            void operator delete[](void *p) noexcept { lmem::FreeToExpHeap(g_heap_handle, p); }
    };

    inline void *Allocate(size_t sz) {
        return lmem::AllocateFromExpHeap(g_heap_handle, sz);
    }

    inline void *AllocateWithAlign(size_t sz, size_t align) {
        return lmem::AllocateFromExpHeap(g_heap_handle, sz, align);
    }

    inline void Deallocate(void *p) {
        return lmem::FreeToExpHeap(g_heap_handle, p);
    }

    inline void DeallocateWithSize(void *p, size_t size) {
        AMS_UNUSED(size);

        return lmem::FreeToExpHeap(g_heap_handle, p);
    }

}
