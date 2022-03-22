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
#include <stratosphere.hpp>
#include "impl/lmem_impl_unit_heap.hpp"

namespace ams::lmem {

    HeapHandle CreateUnitHeap(void *address, size_t size, size_t unit_size, u32 option) {
        HeapHandle handle = impl::CreateUnitHeap(address, size, unit_size, DefaultAlignment, static_cast<u16>(option), InfoPlacement_Head, nullptr);
        if (option & CreateOption_ThreadSafe) {
            os::InitializeSdkMutex(std::addressof(handle->mutex));
        }
        return handle;
    }

    HeapHandle CreateUnitHeap(void *address, size_t size, size_t unit_size, u32 option, s32 alignment, InfoPlacement info_placement) {
        HeapHandle handle = impl::CreateUnitHeap(address, size, unit_size, alignment, static_cast<u16>(option), info_placement, nullptr);
        if (option & CreateOption_ThreadSafe) {
            os::InitializeSdkMutex(std::addressof(handle->mutex));
        }
        return handle;
    }

    HeapHandle CreateUnitHeap(void *address, size_t size, size_t unit_size, u32 option, s32 alignment, HeapCommonHead *heap_head) {
        HeapHandle handle = impl::CreateUnitHeap(address, size, unit_size, alignment, static_cast<u16>(option), InfoPlacement_Head, heap_head);
        if (option & CreateOption_ThreadSafe) {
            os::InitializeSdkMutex(std::addressof(handle->mutex));
        }
        return handle;
    }

    void DestroyUnitHeap(HeapHandle handle) {
        impl::DestroyUnitHeap(handle);
    }

    void InvalidateUnitHeap(HeapHandle handle) {
        impl::ScopedHeapLock lk(handle);
        impl::InvalidateUnitHeap(handle);
    }

    void ExtendUnitHeap(HeapHandle handle, size_t size) {
        impl::ScopedHeapLock lk(handle);
        impl::ExtendUnitHeap(handle, size);
    }

    void *AllocateFromUnitHeap(HeapHandle handle) {
        impl::ScopedHeapLock lk(handle);
        return impl::AllocateFromUnitHeap(handle);
    }

    void FreeToUnitHeap(HeapHandle handle, void *block) {
        impl::ScopedHeapLock lk(handle);
        impl::FreeToUnitHeap(handle, block);
    }

    size_t GetUnitHeapUnitSize(HeapHandle handle) {
        /* Nintendo doesn't acquire a lock here. */
        return impl::GetUnitHeapUnitSize(handle);
    }

    s32    GetUnitHeapAlignment(HeapHandle handle) {
        /* Nintendo doesn't acquire a lock here. */
        return impl::GetUnitHeapAlignment(handle);
    }

    size_t GetUnitHeapFreeCount(HeapHandle handle) {
        impl::ScopedHeapLock lk(handle);
        return impl::GetUnitHeapFreeCount(handle);
    }

    size_t GetUnitHeapUsedCount(HeapHandle handle) {
        impl::ScopedHeapLock lk(handle);
        return impl::GetUnitHeapUsedCount(handle);
    }

    size_t GetUnitHeapRequiredSize(size_t unit_size, size_t unit_count, s32 alignment, bool internal_metadata) {
        return impl::GetUnitHeapRequiredSize(unit_size, unit_count, alignment, internal_metadata);
    }

}
