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
#include "lmem_impl_unit_heap.hpp"

namespace ams::lmem::impl {

    namespace {

        constexpr size_t MinimumAlignment = 4;

        constexpr inline bool IsValidHeapHandle(HeapHandle handle) {
            return handle->magic == UnitHeapMagic;
        }

        constexpr inline UnitHeapHead *GetUnitHeapHead(HeapHead *heap_head) {
            return &heap_head->impl_head.unit_heap_head;
        }

        constexpr inline const UnitHeapHead *GetUnitHeapHead(const HeapHead *heap_head) {
            return &heap_head->impl_head.unit_heap_head;
        }

        inline UnitHead *PopUnit(UnitHeapList *list) {
            if (UnitHead *block = list->head; block != nullptr) {
                list->head = block->next;
                return block;
            } else {
                return nullptr;
            }
        }

        inline void PushUnit(UnitHeapList *list, UnitHead *block) {
            block->next = list->head;
            list->head = block;
        }

    }

    HeapHandle CreateUnitHeap(void *address, size_t size, size_t unit_size, s32 alignment, u16 option, InfoPlacement info_placement, HeapCommonHead *heap_head) {
        AMS_ASSERT(address != nullptr);

        /* Correct alignment, validate. */
        if (alignment == 1 || alignment == 2) {
            alignment = 4;
        }
        AMS_ASSERT(util::IsAligned(alignment, MinimumAlignment));
        AMS_ASSERT(static_cast<s32>(MinimumAlignment) <= alignment);
        AMS_ASSERT(unit_size >= sizeof(uintptr_t));

        /* Setup heap metadata. */
        UnitHeapHead *unit_heap = nullptr;
        void *heap_start        = nullptr;
        void *heap_end          = nullptr;
        if (heap_head == nullptr) {
            /* Internal heap metadata. */
            if (info_placement == InfoPlacement_Head) {
                heap_head  = reinterpret_cast<HeapHead *>(util::AlignUp(address, MinimumAlignment));
                unit_heap  = GetUnitHeapHead(heap_head);
                heap_end   = util::AlignDown(reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(address) + size), MinimumAlignment);
                heap_start = util::AlignUp(reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(heap_head) + sizeof(HeapHead)), alignment);
            } else if (info_placement == InfoPlacement_Tail) {
                heap_end   = util::AlignDown(reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(address) + size - sizeof(HeapHead)), MinimumAlignment);
                heap_head  = reinterpret_cast<HeapHead *>(heap_end);
                unit_heap  = GetUnitHeapHead(heap_head);
                heap_start = util::AlignUp(address, alignment);
            } else {
                AMS_ASSERT(false);
            }
        } else {
            /* External heap metadata. */
            unit_heap  = GetUnitHeapHead(heap_head);
            heap_end   = util::AlignDown(reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(address) + size), MinimumAlignment);
            heap_start = util::AlignUp(address, alignment);
        }

        /* Correct unit size. */
        unit_size = util::AlignUp(unit_size, alignment);

        /* Don't allow a heap with start after end. */
        if (heap_start > heap_end) {
            return nullptr;
        }

        /* Don't allow a heap with no units. */
        size_t max_units = GetPointerDifference(heap_start, heap_end) / unit_size;
        if (max_units == 0) {
            return nullptr;
        }

        /* Set real heap end. */
        heap_end = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(heap_start) + max_units * unit_size);

        /* Initialize the parent heap. */
        InitializeHeapHead(heap_head, UnitHeapMagic, heap_start, heap_end, option);

        /* Initialize the actual unit heap. */
        {
            unit_heap->free_list.head = reinterpret_cast<UnitHead *>(heap_start);
            unit_heap->unit_size      = unit_size;
            unit_heap->alignment      = alignment;
            unit_heap->num_units      = 0;

            /* Create the new units. */
            UnitHead *cur_tail = unit_heap->free_list.head;
            for (size_t i = 0; i < max_units - 1; i++) {
                cur_tail->next = reinterpret_cast<UnitHead *>(reinterpret_cast<uintptr_t>(cur_tail) + unit_size);
                cur_tail       = cur_tail->next;
            }
            cur_tail->next = nullptr;
        }

        /* Return the heap header as handle. */
        return heap_head;
    }

    void DestroyUnitHeap(HeapHandle handle) {
        AMS_ASSERT(IsValidHeapHandle(handle));

        /* Validate that the heap has no living units. */
        UnitHeapHead *unit_heap = GetUnitHeapHead(handle);
        if (unit_heap->free_list.head != nullptr) {
            AMS_ASSERT(unit_heap->num_units == 0);
            unit_heap->free_list.head = nullptr;
        }

        FinalizeHeap(handle);
    }

    void InvalidateUnitHeap(HeapHandle handle) {
        AMS_ASSERT(IsValidHeapHandle(handle));
        GetUnitHeapHead(handle)->free_list.head = nullptr;
    }

    void ExtendUnitHeap(HeapHandle handle, size_t size) {
        AMS_ASSERT(IsValidHeapHandle(handle));

        /* Find the current tail unit, and insert <end of heap> as next. */
        UnitHeapHead *unit_heap = GetUnitHeapHead(handle);
        UnitHead *cur_tail;
        if (unit_heap->free_list.head != nullptr) {
            cur_tail = unit_heap->free_list.head;

            while (cur_tail->next != nullptr) {
                cur_tail = cur_tail->next;
            }

            cur_tail->next = reinterpret_cast<UnitHead *>(handle->heap_end);
            cur_tail       = cur_tail->next;
            cur_tail->next = nullptr;
        } else {
            /* All units are allocated, so set the free list to be at the end of the heap area. */
            unit_heap->free_list.head = reinterpret_cast<UnitHead *>(handle->heap_end);
            cur_tail                  = unit_heap->free_list.head;
            cur_tail->next            = nullptr;
        }

        /* Calculate new unit extents. */
        void *new_units_start = handle->heap_end;
        void *new_units_end   = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(new_units_start) + size);
        size_t num_new_units  = GetPointerDifference(new_units_start, new_units_end) / unit_heap->unit_size;
        AMS_ASSERT(num_new_units > 0);

        /* Create the new units. */
        for (size_t i = 0; i < num_new_units - 1; i++) {
            cur_tail->next = reinterpret_cast<UnitHead *>(reinterpret_cast<uintptr_t>(cur_tail) + unit_heap->unit_size);
            cur_tail       = cur_tail->next;
        }
        cur_tail->next = nullptr;

        /* Note that the heap is bigger. */
        handle->heap_end = new_units_end;
    }

    void *AllocateFromUnitHeap(HeapHandle handle) {
        AMS_ASSERT(IsValidHeapHandle(handle));

        /* Allocate a unit. */
        UnitHeapHead *unit_heap = GetUnitHeapHead(handle);
        UnitHead *unit = PopUnit(&unit_heap->free_list);
        if (unit != nullptr) {
            /* Fill memory with pattern for debug, if needed. */
            FillAllocatedMemory(handle, unit, unit_heap->unit_size);

            /* Note that we allocated a unit. */
            unit_heap->num_units++;
        }

        return unit;
    }

    void FreeToUnitHeap(HeapHandle handle, void *block) {
        AMS_ASSERT(IsValidHeapHandle(handle));

        /* Allow Free(nullptr) to succeed. */
        if (block == nullptr) {
            return;
        }

        /* Fill memory with pattern for debug, if needed. */
        UnitHeapHead *unit_heap = GetUnitHeapHead(handle);
        FillFreedMemory(handle, block, unit_heap->unit_size);

        /* Push the unit onto the free list. */
        PushUnit(&unit_heap->free_list, reinterpret_cast<UnitHead *>(block));

        /* Note that we freed a unit. */
        unit_heap->num_units--;
    }

    size_t GetUnitHeapUnitSize(HeapHandle handle) {
        AMS_ASSERT(IsValidHeapHandle(handle));
        return GetUnitHeapHead(handle)->unit_size;
    }

    s32    GetUnitHeapAlignment(HeapHandle handle) {
        AMS_ASSERT(IsValidHeapHandle(handle));
        return GetUnitHeapHead(handle)->alignment;
    }

    size_t GetUnitHeapFreeCount(HeapHandle handle) {
        AMS_ASSERT(IsValidHeapHandle(handle));

        size_t count = 0;
        for (UnitHead *cur = GetUnitHeapHead(handle)->free_list.head; cur != nullptr; cur = cur->next) {
            count++;
        }
        return count;
    }

    size_t GetUnitHeapUsedCount(HeapHandle handle) {
        AMS_ASSERT(IsValidHeapHandle(handle));
        return GetUnitHeapHead(handle)->num_units;
    }

    size_t GetUnitHeapRequiredSize(size_t unit_size, size_t unit_count, s32 alignment, bool internal_metadata) {
        /* Nintendo does not round up alignment here, even though they do so in CreateUnitHeap. */
        /* We will round up alignment to return more accurate results. */
        if (alignment == 1 || alignment == 2) {
            alignment = 4;
        }

        AMS_ASSERT(util::IsAligned(alignment, MinimumAlignment));
        AMS_ASSERT(static_cast<s32>(MinimumAlignment) <= alignment);
        AMS_ASSERT(unit_size >= sizeof(uintptr_t));

        return (alignment - 1) + util::AlignUp(unit_size, alignment) + (internal_metadata ? sizeof(HeapHead) : 0);
    }

}
