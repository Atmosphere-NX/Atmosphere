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
#include "secmon_spinlock.hpp"
#include "secmon_map.hpp"
#include "smc/secmon_smc_info.hpp"

namespace ams::secmon {

    namespace {

        constexpr inline const uintptr_t BootCodeAddress = MemoryRegionVirtualTzramBootCode.GetAddress();
        constexpr inline const size_t    BootCodeSize    = MemoryRegionVirtualTzramBootCode.GetSize();

        constinit uintptr_t g_smc_user_page_physical_address = 0;
        constinit uintptr_t g_ams_iram_page_physical_address = 0;
        constinit uintptr_t g_ams_user_page_physical_address = 0;

        constinit SpinLockType g_ams_iram_page_spin_lock = {};
        constinit SpinLockType g_ams_user_page_spin_lock = {};

        using namespace ams::mmu;

        constexpr inline PageTableMappingAttribute MappingAttributesEl3NonSecureRwData = AddMappingAttributeIndex(PageTableMappingAttributes_El3NonSecureRwData, MemoryAttributeIndexNormal);
        constexpr inline PageTableMappingAttribute MappingAttributesEl3NonSecureDevice = AddMappingAttributeIndex(PageTableMappingAttributes_El3NonSecureRwData, MemoryAttributeIndexDevice);

        constexpr void UnmapBootCodeImpl(u64 *l1, u64 *l2, u64 *l3, uintptr_t boot_code, size_t boot_code_size) {
            /* Unmap the L3 entries corresponding to the boot code. */
            AMS_UNUSED(l1, l2);
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

        constexpr void MapAtmosphereIramPageImpl(u64 *l3, uintptr_t address) {
            /* Set the L3 entry. */
            SetL3BlockEntry(l3, MemoryRegionVirtualAtmosphereIramPage.GetAddress(), address, MemoryRegionVirtualAtmosphereIramPage.GetSize(), MappingAttributesEl3NonSecureDevice);
        }

        constexpr void UnmapAtmosphereIramPageImpl(u64 *l3) {
            /* Unmap the L3 entry. */
            InvalidateL3Entries(l3, MemoryRegionVirtualAtmosphereIramPage.GetAddress(), MemoryRegionVirtualAtmosphereIramPage.GetSize());
        }

        constexpr void MapAtmosphereUserPageImpl(u64 *l3, uintptr_t address) {
            /* Set the L3 entry. */
            SetL3BlockEntry(l3, MemoryRegionVirtualAtmosphereUserPage.GetAddress(), address, MemoryRegionVirtualAtmosphereUserPage.GetSize(), MappingAttributesEl3NonSecureRwData);
        }

        constexpr void UnmapAtmosphereUserPageImpl(u64 *l3) {
            /* Unmap the L3 entry. */
            InvalidateL3Entries(l3, MemoryRegionVirtualAtmosphereUserPage.GetAddress(), MemoryRegionVirtualAtmosphereUserPage.GetSize());
        }

        constexpr void MapDramForMarikoProgramImpl(u64 *l1, u64 *l2, u64 *l3) {
            /* Map the L1 entry corresponding to the mariko program dram entry. */
            AMS_UNUSED(l2, l3);
            SetL1BlockEntry(l1, MemoryRegionDramForMarikoProgram.GetAddress(), MemoryRegionDramForMarikoProgram.GetAddress(), MemoryRegionDramForMarikoProgram.GetSize(), MappingAttributesEl3NonSecureRwData);
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

    size_t GetPhysicalMemorySize() {
        switch (smc::GetPhysicalMemorySize()) {
            case pkg1::MemorySize_4GB: return 4_GB;
            case pkg1::MemorySize_6GB: return 6_GB;
            case pkg1::MemorySize_8GB: return 8_GB;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    bool IsPhysicalMemoryAddress(uintptr_t address) {
        return (address - MemoryRegionDram.GetAddress()) < GetPhysicalMemorySize();
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
            if (!IsPhysicalMemoryAddress(address)) {
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

        /* Ensure that the page is no longer in cache. */
        hw::FlushDataCache(MemoryRegionVirtualSmcUserPage.GetPointer<void>(), MemoryRegionVirtualSmcUserPage.GetSize());
        hw::DataSynchronizationBarrierInnerShareable();

        u64 * const l2_l3 = MemoryRegionVirtualTzramL2L3PageTable.GetPointer<u64>();

        UnmapSmcUserPageImpl(l2_l3);

        /* Ensure the mappings are consistent. */
        secmon::EnsureMappingConsistency(MemoryRegionVirtualSmcUserPage.GetAddress());

        g_smc_user_page_physical_address = 0;
    }

    uintptr_t MapAtmosphereIramPage(uintptr_t address) {
        /* Acquire the ams iram spinlock. */
        AcquireSpinLock(g_ams_iram_page_spin_lock);
        auto lock_guard = SCOPE_GUARD { ReleaseSpinLock(g_ams_iram_page_spin_lock); };

        /* Validate that the page is an IRAM page. */
        if (!MemoryRegionPhysicalIram.Contains(address, 1)) {
            return 0;
        }

        /* Validate that the page isn't a secure monitor debug page. */
        if (MemoryRegionPhysicalIramSecureMonitorDebug.Contains(address, 1)) {
            return 0;
        }

        /* Validate that the page is aligned. */
        if (!util::IsAligned(address, 4_KB)) {
            return 0;
        }

        /* Map the page. */
        g_ams_iram_page_physical_address = address;

        u64 * const l2_l3 = MemoryRegionVirtualTzramL2L3PageTable.GetPointer<u64>();

        MapAtmosphereIramPageImpl(l2_l3, address);

        /* Ensure the mappings are consistent. */
        secmon::EnsureMappingConsistency(MemoryRegionVirtualAtmosphereIramPage.GetAddress());

        /* Hold the lock. */
        lock_guard.Cancel();

        return MemoryRegionVirtualAtmosphereIramPage.GetAddress();
    }

    void UnmapAtmosphereIramPage() {
        /* Can't unmap if nothing's unmapped. */
        if (g_ams_iram_page_physical_address == 0) {
            return;
        }

        /* Ensure that the page is no longer in cache. */
        hw::FlushDataCache(MemoryRegionVirtualAtmosphereIramPage.GetPointer<void>(), MemoryRegionVirtualAtmosphereIramPage.GetSize());
        hw::DataSynchronizationBarrierInnerShareable();

        /* Unmap the page. */
        u64 * const l2_l3 = MemoryRegionVirtualTzramL2L3PageTable.GetPointer<u64>();

        UnmapAtmosphereIramPageImpl(l2_l3);

        /* Ensure the mappings are consistent. */
        secmon::EnsureMappingConsistency(MemoryRegionVirtualAtmosphereIramPage.GetAddress());

        /* Release the page. */
        g_ams_iram_page_physical_address = 0;

        ReleaseSpinLock(g_ams_iram_page_spin_lock);
    }

    uintptr_t MapAtmosphereUserPage(uintptr_t address) {
        /* Acquire the ams user spinlock. */
        AcquireSpinLock(g_ams_user_page_spin_lock);
        auto lock_guard = SCOPE_GUARD { ReleaseSpinLock(g_ams_user_page_spin_lock); };

        /* Validate that the page is a dram page. */
        if (!IsPhysicalMemoryAddress(address)) {
            return 0;
        }

        /* Validate that the page is aligned. */
        if (!util::IsAligned(address, 4_KB)) {
            return 0;
        }

        /* Map the page. */
        g_ams_user_page_physical_address = address;

        u64 * const l2_l3 = MemoryRegionVirtualTzramL2L3PageTable.GetPointer<u64>();

        MapAtmosphereUserPageImpl(l2_l3, address);

        /* Ensure the mappings are consistent. */
        secmon::EnsureMappingConsistency(MemoryRegionVirtualAtmosphereUserPage.GetAddress());

        /* Hold the lock. */
        lock_guard.Cancel();

        return MemoryRegionVirtualAtmosphereUserPage.GetAddress();
    }

    void UnmapAtmosphereUserPage() {
        /* Can't unmap if nothing's unmapped. */
        if (g_ams_user_page_physical_address == 0) {
            return;
        }

        /* Ensure that the page is no longer in cache. */
        hw::FlushDataCache(MemoryRegionVirtualAtmosphereUserPage.GetPointer<void>(), MemoryRegionVirtualAtmosphereUserPage.GetSize());
        hw::DataSynchronizationBarrierInnerShareable();

        /* Unmap the page. */
        u64 * const l2_l3 = MemoryRegionVirtualTzramL2L3PageTable.GetPointer<u64>();

        UnmapAtmosphereUserPageImpl(l2_l3);

        /* Ensure the mappings are consistent. */
        secmon::EnsureMappingConsistency(MemoryRegionVirtualAtmosphereUserPage.GetAddress());

        /* Release the page. */
        g_ams_user_page_physical_address = 0;

        ReleaseSpinLock(g_ams_user_page_spin_lock);
    }

    void MapDramForMarikoProgram() {
        /* Get the tables. */
        u64 * const l1    = MemoryRegionVirtualTzramL1PageTable.GetPointer<u64>();
        u64 * const l2_l3 = MemoryRegionVirtualTzramL2L3PageTable.GetPointer<u64>();

        /* Map. */
        MapDramForMarikoProgramImpl(l1, l2_l3, l2_l3);

        /* Ensure the mappings are consistent. */
        secmon::EnsureMappingConsistency();
    }

}
