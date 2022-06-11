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
#include "impl/os_memory_heap_manager.hpp"

namespace ams::os {

    Result SetMemoryHeapSize(size_t size) {
        /* Check pre-conditions. */
        AMS_ASSERT(util::IsAligned(size, MemoryHeapUnitSize));

        /* Set the heap size. */
        R_RETURN(impl::GetMemoryHeapManager().SetHeapSize(size));
    }

    uintptr_t GetMemoryHeapAddress() {
        return impl::GetMemoryHeapManager().GetHeapAddress();
    }

    size_t GetMemoryHeapSize() {
        return impl::GetMemoryHeapManager().GetHeapSize();
    }

    Result AllocateMemoryBlock(uintptr_t *out_address, size_t size) {
        /* Check pre-conditions. */
        AMS_ASSERT(size > 0);
        AMS_ASSERT(util::IsAligned(size, MemoryBlockUnitSize));

        /* Allocate from heap. */
        R_RETURN(impl::GetMemoryHeapManager().AllocateFromHeap(out_address, size));
    }

    void FreeMemoryBlock(uintptr_t address, size_t size) {
        /* Get memory heap manager. */
        auto &manager = impl::GetMemoryHeapManager();

        /* Check pre-conditions. */
        AMS_ASSERT(util::IsAligned(address, MemoryBlockUnitSize));
        AMS_ASSERT(size > 0);
        AMS_ASSERT(util::IsAligned(size, MemoryBlockUnitSize));
        AMS_ABORT_UNLESS(manager.IsRegionInMemoryHeap(address, size));

        /* Release the memory block. */
        manager.ReleaseToHeap(address, size);
    }

}
