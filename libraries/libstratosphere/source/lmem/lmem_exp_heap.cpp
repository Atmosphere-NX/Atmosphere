/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include "impl/lmem_impl_exp_heap.hpp"

namespace ams::lmem {

    HeapHandle CreateExpHeap(void *address, size_t size, u32 option) {
        HeapHandle handle = impl::CreateExpHeap(address, size, option);
        if (option & CreateOption_ThreadSafe) {
            os::InitializeMutex(std::addressof(handle->mutex), false, 0);
        }
        return handle;
    }

    void DestroyExpHeap(HeapHandle handle) {
        if (handle->option & CreateOption_ThreadSafe) {
            os::FinalizeMutex(std::addressof(handle->mutex));
        }
        impl::DestroyExpHeap(handle);
    }

    MemoryRange AdjustExpHeap(HeapHandle handle) {
        impl::ScopedHeapLock lk(handle);
        return impl::AdjustExpHeap(handle);
    }

    void *AllocateFromExpHeap(HeapHandle handle, size_t size) {
        impl::ScopedHeapLock lk(handle);
        return impl::AllocateFromExpHeap(handle, size, DefaultAlignment);
    }

    void *AllocateFromExpHeap(HeapHandle handle, size_t size, s32 alignment) {
        impl::ScopedHeapLock lk(handle);
        return impl::AllocateFromExpHeap(handle, size, alignment);
    }

    void FreeToExpHeap(HeapHandle handle, void *block) {
        impl::ScopedHeapLock lk(handle);
        impl::FreeToExpHeap(handle, block);
    }

    size_t ResizeExpHeapMemoryBlock(HeapHandle handle, void *block, size_t size) {
        impl::ScopedHeapLock lk(handle);
        return impl::ResizeExpHeapMemoryBlock(handle, block, size);
    }

    size_t GetExpHeapTotalFreeSize(HeapHandle handle) {
        impl::ScopedHeapLock lk(handle);
        return impl::GetExpHeapTotalFreeSize(handle);
    }

    size_t GetExpHeapAllocatableSize(HeapHandle handle, s32 alignment) {
        impl::ScopedHeapLock lk(handle);
        return impl::GetExpHeapAllocatableSize(handle, alignment);
    }

    AllocationMode GetExpHeapAllocationMode(HeapHandle handle) {
        impl::ScopedHeapLock lk(handle);
        return impl::GetExpHeapAllocationMode(handle);
    }

    AllocationMode SetExpHeapAllocationMode(HeapHandle handle, AllocationMode new_mode) {
        impl::ScopedHeapLock lk(handle);
        return impl::SetExpHeapAllocationMode(handle, new_mode);
    }

    bool GetExpHeapUseMarginsOfAlignment(HeapHandle handle) {
        impl::ScopedHeapLock lk(handle);
        return impl::GetExpHeapUseMarginsOfAlignment(handle);
    }

    bool SetExpHeapUseMarginsOfAlignment(HeapHandle handle, bool use_margins) {
        impl::ScopedHeapLock lk(handle);
        return impl::SetExpHeapUseMarginsOfAlignment(handle, use_margins);
    }

    u16 GetExpHeapGroupId(HeapHandle handle) {
        impl::ScopedHeapLock lk(handle);
        return impl::GetExpHeapGroupId(handle);
    }

    u16 SetExpHeapGroupId(HeapHandle handle, u16 group_id) {
        impl::ScopedHeapLock lk(handle);
        return impl::SetExpHeapGroupId(handle, group_id);
    }

    size_t GetExpHeapMemoryBlockSize(const void *memory_block) {
        return impl::GetExpHeapMemoryBlockSize(memory_block);
    }

    u16 GetExpHeapMemoryBlockGroupId(const void *memory_block) {
        return impl::GetExpHeapMemoryBlockGroupId(memory_block);
    }

    AllocationDirection GetExpHeapMemoryBlockAllocationDirection(const void *memory_block) {
        return impl::GetExpHeapMemoryBlockAllocationDirection(memory_block);
    }

    void VisitExpHeapAllocatedBlocks(HeapHandle handle, HeapVisitor visitor, uintptr_t user_data) {
        impl::ScopedHeapLock lk(handle);
        impl::VisitExpHeapAllocatedBlocks(handle, visitor, user_data);
    }

}
