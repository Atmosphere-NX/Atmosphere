/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
#include <exosphere.hpp>
#include "secmon_cache.hpp"
#include "secmon_map.hpp"

namespace ams::secmon {

    namespace {

        using namespace ams::mmu;

        constexpr void UnmapBootCodeImpl(u64 *l1, u64 *l2, u64 *l3, uintptr_t boot_code, size_t boot_code_size) {
            /* Unmap the L3 entries corresponding to the boot code. */
            InvalidateL3Entries(l3, boot_code, boot_code_size);
        }

        constexpr void UnmapTzramImpl(u64 *l1, u64 *l2, u64 *l3) {
            /* Unmap the L3 entries corresponding to tzram. */
            InvalidateL3Entries(l3, MemoryRegionPhysicalTzram.GetAddress(),   MemoryRegionPhysicalTzram.GetSize());

            /* Unmap the L2 entries corresponding to those L3 entries. */
            InvalidateL2Entries(l2, MemoryRegionPhysicalTzramL2.GetAddress(), MemoryRegionPhysicalTzramL2.GetSize());

            /* Unmap the L1 entry corresponding to to those L2 entries. */
            InvalidateL1Entries(l1, MemoryRegionPhysical.GetAddress(),        MemoryRegionPhysical.GetSize());
        }

    }

    void UnmapBootCode() {
        /* Get the tables. */
        u64 * const l1    = MemoryRegionVirtualTzramL1PageTable.GetPointer<u64>();
        u64 * const l2_l3 = MemoryRegionVirtualTzramL2L3PageTable.GetPointer<u64>();

        /* Get the boot code region. */
        const uintptr_t boot_code   = MemoryRegionVirtualTzramBootCode.GetAddress();
        const size_t boot_code_size = MemoryRegionVirtualTzramBootCode.GetSize();

        /* Clear the boot code. */
        util::ClearMemory(reinterpret_cast<void *>(boot_code), boot_code_size);

        /* Unmap. */
        UnmapBootCodeImpl(l1, l2_l3, l2_l3, boot_code, boot_code_size);

        /* Ensure the mappings are consistent. */
        secmon::EnsureMappingConsistency();
    }

    void UnmapTzram() {
        /* Get the tables. */
        u64 * const l1    = MemoryRegionVirtualTzramL1PageTable.GetPointer<u64>();
        u64 * const l2_l3 = MemoryRegionVirtualTzramL2L3PageTable.GetPointer<u64>();

        /* Unmap. */
        UnmapTzramImpl(l1, l2_l3, l2_l3);

        /* Ensure the mappings are consistent. */
        secmon::EnsureMappingConsistency();
    }

}
