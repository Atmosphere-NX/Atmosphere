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

    HeapHandle  CreateUnitHeap(void *address, size_t size, size_t unit_size, s32 alignment, u16 option, InfoPlacement info_placement, HeapCommonHead *heap_head);
    void        DestroyUnitHeap(HeapHandle handle);

    void InvalidateUnitHeap(HeapHandle handle);
    void ExtendUnitHeap(HeapHandle handle, size_t size);

    void *AllocateFromUnitHeap(HeapHandle handle);
    void  FreeToUnitHeap(HeapHandle handle, void *block);

    size_t GetUnitHeapUnitSize(HeapHandle handle);
    s32    GetUnitHeapAlignment(HeapHandle handle);
    size_t GetUnitHeapFreeCount(HeapHandle handle);
    size_t GetUnitHeapUsedCount(HeapHandle handle);

    size_t GetUnitHeapRequiredSize(size_t unit_size, size_t unit_count, s32 alignment, bool internal_metadata);

}