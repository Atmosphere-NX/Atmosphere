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
#include "secmon_setup.hpp"
#include "secmon_map.hpp"

namespace ams::secmon {

    namespace {

        constexpr inline const uintptr_t BootCodeAddress = MemoryRegionVirtualTzramBootCode.GetAddress();
        constexpr inline const size_t    BootCodeSize    = MemoryRegionVirtualTzramBootCode.GetSize();

        constinit uintptr_t g_smc_user_page_physical_address = 0;

        using namespace ams::mmu;

        constexpr inline PageTableMappingAttribute MappingAttributesEl3NonSecureRwData = AddMappingAttributeIndex(PageTableMappingAttributes_El3NonSecureRwData, MemoryAttributeIndexNormal);

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

        constexpr void MapSmcUserPageImpl(u64 *l3, uintptr_t address) {
            /* Set the L3 entry. */
            SetL3BlockEntry(l3, MemoryRegionVirtualSmcUserPage.GetAddress(), address, MemoryRegionVirtualSmcUserPage.GetSize(), MappingAttributesEl3NonSecureRwData);
        }

        constexpr void UnmapSmcUserPageImpl(u64 *l3) {
            /* Unmap the L3 entry. */
            InvalidateL3Entries(l3, MemoryRegionVirtualSmcUserPage.GetAddress(), MemoryRegionVirtualSmcUserPage.GetSize());
        }

        void ClearLow(uintptr_t address, size_t size) {
            /* Clear the low part. */
            util::ClearMemory(reinterpret_cast<void *>(address), size / 2);
        }

        void ClearHigh(uintptr_t address, size_t size) {
            /* Clear the high part. */
            util::ClearMemory(reinterpret_cast<void *>(address + size / 2), size / 2);
        }

    }

    void ClearBootCodeHigh() {
        ClearHigh(BootCodeAddress, BootCodeSize);
    }

    void UnmapBootCode() {
        /* Get the tables. */
        u64 * const l1    = MemoryRegionVirtualTzramL1PageTable.GetPointer<u64>();
        u64 * const l2_l3 = MemoryRegionVirtualTzramL2L3PageTable.GetPointer<u64>();

        /* Clear the low boot code region; high was already cleared by a previous call. */
        ClearLow(BootCodeAddress, BootCodeSize);

        /* Unmap. */
        UnmapBootCodeImpl(l1, l2_l3, l2_l3, BootCodeAddress, BootCodeSize);

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

    uintptr_t MapSmcUserPage(uintptr_t address) {
        if (g_smc_user_page_physical_address == 0) {
            if (!(MemoryRegionDram.GetAddress() <= address && address <= MemoryRegionDramHigh.GetEndAddress() - MemoryRegionVirtualSmcUserPage.GetSize())) {
                return 0;
            }
            if (!util::IsAligned(address, 4_KB)) {
                return 0;
            }

            g_smc_user_page_physical_address = address;

            u64 * const l2_l3 = MemoryRegionVirtualTzramL2L3PageTable.GetPointer<u64>();

            MapSmcUserPageImpl(l2_l3, address);

            /* Ensure the mappings are consistent. */
            secmon::EnsureMappingConsistency(MemoryRegionVirtualSmcUserPage.GetAddress());
        } else {
            AMS_ABORT_UNLESS(address == g_smc_user_page_physical_address);
        }

        return MemoryRegionVirtualSmcUserPage.GetAddress();
    }

    void UnmapSmcUserPage() {
        if (g_smc_user_page_physical_address == 0) {
            return;
        }

        u64 * const l2_l3 = MemoryRegionVirtualTzramL2L3PageTable.GetPointer<u64>();

        UnmapSmcUserPageImpl(l2_l3);

        /* Ensure the mappings are consistent. */
        secmon::EnsureMappingConsistency(MemoryRegionVirtualSmcUserPage.GetAddress());

        g_smc_user_page_physical_address = 0;
    }

}
