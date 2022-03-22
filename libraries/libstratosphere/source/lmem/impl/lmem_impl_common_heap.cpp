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
#include "lmem_impl_common_heap.hpp"

namespace ams::lmem::impl {

    namespace {

        u32 g_fill_values[FillType_Count] = {
            0xC3C3C3C3, /* FillType_Unallocated */
            0xF3F3F3F3, /* FillType_Allocated */
            0xD3D3D3D3, /* FillType_Freed */
        };

    }

    void InitializeHeapHead(HeapHead *out, u32 magic, void *start, void *end, u32 option) {
        /* Call member constructors. */
        std::construct_at(std::addressof(out->list_node));
        std::construct_at(std::addressof(out->child_list));

        /* Set fields. */
        out->magic = magic;
        out->heap_start = start;
        out->heap_end = end;
        out->option = static_cast<u8>(option);

        /* Fill memory with pattern if needed. */
        FillUnallocatedMemory(out, start, GetPointerDifference(start, end));
    }

    void FinalizeHeap(HeapHead *heap) {
        /* Nothing actually needs to be done here. */
        AMS_UNUSED(heap);
    }

    bool ContainsAddress(HeapHandle handle, const void *address) {
        const uintptr_t uptr_handle = reinterpret_cast<uintptr_t>(handle);
        const uintptr_t uptr_start  = reinterpret_cast<uintptr_t>(handle->heap_start);
        const uintptr_t uptr_end    = reinterpret_cast<uintptr_t>(handle->heap_end);
        const uintptr_t uptr_addr   = reinterpret_cast<uintptr_t>(address);

        if (uptr_start - sizeof(HeapHead) == uptr_handle) {
            /* The heap head is at the start of the managed memory. */
            return uptr_handle <= uptr_addr && uptr_addr < uptr_end;
        } else if (uptr_handle == uptr_end) {
            /* The heap head is at the end of the managed memory. */
            return uptr_start  <= uptr_addr && uptr_addr < uptr_end + sizeof(HeapHead);
        } else {
            /* Heap head is somewhere unrelated to managed memory. */
            return uptr_start  <= uptr_addr && uptr_addr < uptr_end;
        }
    }

    size_t GetHeapTotalSize(HeapHandle handle) {
        const uintptr_t uptr_start  = reinterpret_cast<uintptr_t>(handle->heap_start);
        const uintptr_t uptr_end    = reinterpret_cast<uintptr_t>(handle->heap_end);

        if (ContainsAddress(handle, reinterpret_cast<const void *>(handle))) {
            /* The heap metadata is contained within the heap, either before or after. */
            return static_cast<size_t>(uptr_end - uptr_start + sizeof(HeapHead));
        } else {
            /* The heap metadata is not contained within the heap. */
            return static_cast<size_t>(uptr_end - uptr_start);
        }
    }

    u32 GetDebugFillValue(FillType type) {
        return g_fill_values[type];
    }

    u32 SetDebugFillValue(FillType type, u32 value) {
        const u32 old_value = g_fill_values[type];
        g_fill_values[type] = value;
        return old_value;
    }


}