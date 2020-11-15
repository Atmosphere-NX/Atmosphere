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
#include "../secmon_setup.hpp"
#include "secmon_boot.hpp"

namespace ams::secmon::boot {

    namespace {

        using namespace ams::mmu;

        constexpr inline PageTableMappingAttribute MappingAttributesEl3SecureRoCode    = AddMappingAttributeIndex(PageTableMappingAttributes_El3SecureRoCode,    MemoryAttributeIndexNormal);
        constexpr inline PageTableMappingAttribute MappingAttributesEl3SecureRwCode    = AddMappingAttributeIndex(PageTableMappingAttributes_El3SecureRwCode,    MemoryAttributeIndexNormal);
        constexpr inline PageTableMappingAttribute MappingAttributesEl3SecureRoData    = AddMappingAttributeIndex(PageTableMappingAttributes_El3SecureRoData,    MemoryAttributeIndexNormal);
        constexpr inline PageTableMappingAttribute MappingAttributesEl3SecureRwData    = AddMappingAttributeIndex(PageTableMappingAttributes_El3SecureRwData,    MemoryAttributeIndexNormal);
        constexpr inline PageTableMappingAttribute MappingAttributesEl3NonSecureRwData = AddMappingAttributeIndex(PageTableMappingAttributes_El3NonSecureRwData, MemoryAttributeIndexNormal);

        constexpr inline PageTableMappingAttribute MappingAttributesEl3SecureDevice    = AddMappingAttributeIndex(PageTableMappingAttributes_El3SecureRwData,    MemoryAttributeIndexDevice);
        constexpr inline PageTableMappingAttribute MappingAttributesEl3NonSecureDevice = AddMappingAttributeIndex(PageTableMappingAttributes_El3NonSecureRwData, MemoryAttributeIndexDevice);


        constexpr void ClearMemory(volatile u64 *start, size_t size) {
            volatile u64 * const end = start + (size / sizeof(u64));

            for (volatile u64 *cur = start; cur < end; ++cur) {
                *cur = 0;
            }
        }

        constexpr void MakePageTablesImpl(u64 *l1, u64 *l2, u64 *l3) {
            /* Setup the L1 table. */
            {
                ClearMemory(l1, MemoryRegionPhysicalTzramL1PageTable.GetSize());

                /* Create an L1 table entry for the physical region. */
                SetL1TableEntry(l1, MemoryRegionPhysical.GetAddress(), MemoryRegionPhysicalTzramL2L3PageTable.GetAddress(), PageTableTableAttributes_El3SecureCode);
                static_assert(GetL1EntryIndex(MemoryRegionPhysical.GetAddress()) == 1);

                /* Create an L1 mapping entry for dram. */
                SetL1BlockEntry(l1, MemoryRegionDram.GetAddress(), MemoryRegionDram.GetAddress(), MemoryRegionDram.GetSize(), MappingAttributesEl3NonSecureRwData);

                /* Create an L1 table entry for the virtual region. */
                SetL1TableEntry(l1, MemoryRegionVirtual.GetAddress(), MemoryRegionPhysicalTzramL2L3PageTable.GetAddress(), PageTableTableAttributes_El3SecureCode);
            }

            /* Setup the L2 table. */
            {
                ClearMemory(l2, MemoryRegionPhysicalTzramL2L3PageTable.GetSize());

                /* Create an L2 table entry for the virtual region. */
                SetL2TableEntry(l2, MemoryRegionVirtualL2.GetAddress(), MemoryRegionPhysicalTzramL2L3PageTable.GetAddress(), PageTableTableAttributes_El3SecureCode);

                /* Create an L2 table entry for the physical iram region. */
                SetL2TableEntry(l2, MemoryRegionPhysicalIramL2.GetAddress(), MemoryRegionPhysicalTzramL2L3PageTable.GetAddress(), PageTableTableAttributes_El3SecureCode);

                /* Create an L2 table entry for the physical tzram region. */
                SetL2TableEntry(l2, MemoryRegionPhysicalTzramL2.GetAddress(), MemoryRegionPhysicalTzramL2L3PageTable.GetAddress(), PageTableTableAttributes_El3SecureCode);
            }

            /* Setup the L3 table. */
            {
                /* L2 and L3 share a page table. */
                if (l2 != l3) {
                    ClearMemory(l3, MemoryRegionPhysicalTzramL2L3PageTable.GetSize());
                }

                /* Identity-map TZRAM as rwx. */
                SetL3BlockEntry(l3, MemoryRegionPhysicalTzram.GetAddress(), MemoryRegionPhysicalTzram.GetAddress(), MemoryRegionPhysicalTzram.GetSize(), MappingAttributesEl3SecureRwCode);

                /* Identity-map IRAM boot code as rwx. */
                SetL3BlockEntry(l3, MemoryRegionPhysicalIramBootCode.GetAddress(), MemoryRegionPhysicalIramBootCode.GetAddress(), MemoryRegionPhysicalIramBootCode.GetSize(), MappingAttributesEl3SecureRwCode);

                #if defined(AMS_BUILD_FOR_DEBUGGING) || defined(AMS_BUILD_FOR_AUDITING)
                {
                    /* Map the debug code region as rwx. */
                    SetL3BlockEntry(l3, MemoryRegionVirtualDebugCode.GetAddress(), MemoryRegionPhysicalDebugCode.GetAddress(), MemoryRegionPhysicalDebugCode.GetSize(), MappingAttributesEl3SecureRwCode);


                    /* Map the DRAM debug code store region as rw. */
                    SetL3BlockEntry(l3, MemoryRegionVirtualDramDebugDataStore.GetAddress(), MemoryRegionPhysicalDramDebugDataStore.GetAddress(), MemoryRegionPhysicalDramDebugDataStore.GetSize(), MappingAttributesEl3NonSecureRwData);
                }
                #endif

                /* Map all devices. */
                {
                    #define MAP_DEVICE_REGION(_NAME_, _PREV_, _ADDRESS_, _SIZE_, _SECURE_) \
                        SetL3BlockEntry(l3, MemoryRegionVirtualDevice##_NAME_.GetAddress(), MemoryRegionPhysicalDevice##_NAME_.GetAddress(), MemoryRegionVirtualDevice##_NAME_.GetSize(), _SECURE_ ? MappingAttributesEl3SecureDevice : MappingAttributesEl3NonSecureDevice);

                    AMS_SECMON_FOREACH_DEVICE_REGION(MAP_DEVICE_REGION);

                    #undef MAP_DEVICE_REGION
                }

                /* Map the IRAM SC7 work region. */
                SetL3BlockEntry(l3, MemoryRegionVirtualIramSc7Work.GetAddress(), MemoryRegionPhysicalIramSc7Work.GetAddress(), MemoryRegionVirtualIramSc7Work.GetSize(), MappingAttributesEl3NonSecureDevice);

                /* Map the IRAM SC7 firmware region. */
                SetL3BlockEntry(l3, MemoryRegionVirtualIramSc7Firmware.GetAddress(), MemoryRegionPhysicalIramSc7Firmware.GetAddress(), MemoryRegionVirtualIramSc7Firmware.GetSize(), MappingAttributesEl3NonSecureDevice);

                /* Map the Debug region. */
                /* NOTE: This region is reserved for debug. By default it will be the last 0x8000 bytes of IRAM, but this is subject to change. */
                /*       If you are doing development work for exosphere, feel free to locally change this to whatever is useful. */
                SetL3BlockEntry(l3, MemoryRegionVirtualDebug.GetAddress(), MemoryRegionPhysicalIram.GetEndAddress() - 0x8000, 0x8000, MappingAttributesEl3SecureDevice);

                /* Map the TZRAM ro alias region. */
                SetL3BlockEntry(l3, MemoryRegionVirtualTzramReadOnlyAlias.GetAddress(), MemoryRegionPhysicalTzramReadOnlyAlias.GetAddress(), MemoryRegionVirtualTzramReadOnlyAlias.GetSize(), MappingAttributesEl3SecureRoData);

                /* Map the DRAM secure data store region. */
                SetL3BlockEntry(l3, MemoryRegionVirtualDramSecureDataStore.GetAddress(), MemoryRegionPhysicalDramSecureDataStore.GetAddress(), MemoryRegionVirtualDramSecureDataStore.GetSize(), MappingAttributesEl3NonSecureDevice);

                /* Map the program region as rwx. */
                SetL3BlockEntry(l3, MemoryRegionVirtualTzramProgram.GetAddress(), MemoryRegionPhysicalTzramProgram.GetAddress(), MemoryRegionVirtualTzramProgram.GetSize(), MappingAttributesEl3SecureRwCode);

                /* Map the mariko program region as rwx. */
                SetL3BlockEntry(l3, MemoryRegionVirtualTzramMarikoProgram.GetAddress(), MemoryRegionPhysicalTzramMarikoProgram.GetAddress(), MemoryRegionPhysicalTzramMarikoProgram.GetSize(), MappingAttributesEl3SecureRwCode);

                /* Map the mariko program region as rwx. */
                SetL3BlockEntry(l3, MemoryRegionVirtualTzramMarikoProgramStack.GetAddress(), MemoryRegionPhysicalTzramMarikoProgramStack.GetAddress(), MemoryRegionPhysicalTzramMarikoProgramStack.GetSize(), MappingAttributesEl3SecureRwData);

                /* Map the boot code region. */
                SetL3BlockEntry(l3, MemoryRegionVirtualTzramBootCode.GetAddress(), MemoryRegionPhysicalTzramBootCode.GetAddress(), MemoryRegionVirtualTzramBootCode.GetSize(), MappingAttributesEl3SecureRwCode);

                /* Map the volatile data regions regions. */
                SetL3BlockEntry(l3, MemoryRegionVirtualTzramVolatileData.GetAddress(), MemoryRegionPhysicalTzramVolatileData.GetAddress(), MemoryRegionVirtualTzramVolatileData.GetSize(), MappingAttributesEl3SecureRwData);
                SetL3BlockEntry(l3, MemoryRegionVirtualTzramVolatileStack.GetAddress(), MemoryRegionPhysicalTzramVolatileStack.GetAddress(), MemoryRegionVirtualTzramVolatileStack.GetSize(), MappingAttributesEl3SecureRwData);

                /* Map the configuration data. */
                SetL3BlockEntry(l3, MemoryRegionVirtualTzramConfigurationData.GetAddress(), MemoryRegionPhysicalTzramConfigurationData.GetAddress(), MemoryRegionVirtualTzramConfigurationData.GetSize(), MappingAttributesEl3SecureRwData);

                /* Map the page tables. */
                SetL3BlockEntry(l3, util::AlignDown(MemoryRegionVirtualTzramL1PageTable.GetAddress(), PageSize), util::AlignDown(MemoryRegionPhysicalTzramL1PageTable.GetAddress(), PageSize), PageSize, MappingAttributesEl3SecureRwData);
                SetL3BlockEntry(l3, MemoryRegionVirtualTzramL2L3PageTable.GetAddress(), MemoryRegionPhysicalTzramL2L3PageTable.GetAddress(), MemoryRegionVirtualTzramL2L3PageTable.GetSize(), MappingAttributesEl3SecureRwData);
            }
        }

        constexpr bool ValidateTzramPageTables() {
            u64 l1_table[MemoryRegionPhysicalTzramL1PageTable.GetSize() / sizeof(u64)]      = {};
            u64 l2_l3_table[MemoryRegionPhysicalTzramL2L3PageTable.GetSize() / sizeof(u64)] = {};

            MakePageTablesImpl(l1_table, l2_l3_table, l2_l3_table);

            return true;
        }

        static_assert(ValidateTzramPageTables());

    }

    void MakePageTable() {
        u64 * const l1    = MemoryRegionPhysicalTzramL1PageTable.GetPointer<u64>();
        u64 * const l2_l3 = MemoryRegionPhysicalTzramL2L3PageTable.GetPointer<u64>();
        MakePageTablesImpl(l1, l2_l3, l2_l3);
    }

}
