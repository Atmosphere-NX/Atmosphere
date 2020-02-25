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
#pragma once
#include <stratosphere.hpp>
#include "lmem_impl_common_heap.hpp"

namespace ams::lmem::impl {

    HeapHandle  CreateExpHeap(void *address, size_t size, u32 option);
    void        DestroyExpHeap(HeapHandle handle);
    MemoryRange AdjustExpHeap(HeapHandle handle);

    void *AllocateFromExpHeap(HeapHandle handle, size_t size, s32 alignment);
    void  FreeToExpHeap(HeapHandle handle, void *block);

    size_t ResizeExpHeapMemoryBlock(HeapHandle handle, void *block, size_t size);

    size_t GetExpHeapTotalFreeSize(HeapHandle handle);
    size_t GetExpHeapAllocatableSize(HeapHandle handle, s32 alignment);

    AllocationMode GetExpHeapAllocationMode(HeapHandle handle);
    AllocationMode SetExpHeapAllocationMode(HeapHandle handle, AllocationMode new_mode);

    bool GetExpHeapUseMarginsOfAlignment(HeapHandle handle);
    bool SetExpHeapUseMarginsOfAlignment(HeapHandle handle, bool use_margins);

    u16  GetExpHeapGroupId(HeapHandle handle);
    u16  SetExpHeapGroupId(HeapHandle handle, u16 group_id);

    size_t GetExpHeapMemoryBlockSize(const void *memory_block);
    u16 GetExpHeapMemoryBlockGroupId(const void *memory_block);
    AllocationDirection GetExpHeapMemoryBlockAllocationDirection(const void *memory_block);

    void VisitExpHeapAllocatedBlocks(HeapHandle handle, HeapVisitor visitor, uintptr_t user_data);

}