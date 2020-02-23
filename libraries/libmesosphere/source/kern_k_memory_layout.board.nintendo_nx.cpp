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
#include <mesosphere.hpp>

namespace ams::kern {

    namespace {

        constexpr uintptr_t DramPhysicalAddress = 0x80000000;
        constexpr size_t ReservedEarlyDramSize  = 0x60000;

        ALWAYS_INLINE bool SetupUartPhysicalMemoryRegion() {
            #if   defined(MESOSPHERE_DEBUG_LOG_USE_UART_A)
                return KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x70006000, 0x40, KMemoryRegionType_Uart | KMemoryRegionAttr_ShouldKernelMap);
            #elif defined(MESOSPHERE_DEBUG_LOG_USE_UART_B)
                return KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x70006040, 0x40, KMemoryRegionType_Uart | KMemoryRegionAttr_ShouldKernelMap);
            #elif defined(MESOSPHERE_DEBUG_LOG_USE_UART_C)
                return KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x70006200, 0x100, KMemoryRegionType_Uart | KMemoryRegionAttr_ShouldKernelMap);
            #elif defined(MESOSPHERE_DEBUG_LOG_USE_UART_D)
                return KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x70006300, 0x100, KMemoryRegionType_Uart | KMemoryRegionAttr_ShouldKernelMap);
            #else
                #error "Unknown Debug UART device!"
            #endif
        }

    }

    namespace init {

        void SetupDevicePhysicalMemoryRegions() {
            /* TODO: Give these constexpr defines somewhere? */
            MESOSPHERE_INIT_ABORT_UNLESS(SetupUartPhysicalMemoryRegion());
            MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x70019000, 0x1000,    KMemoryRegionType_MemoryController          | KMemoryRegionAttr_NoUserMap));
            MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x7001C000, 0x1000,    KMemoryRegionType_MemoryController0         | KMemoryRegionAttr_NoUserMap));
            MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x7001D000, 0x1000,    KMemoryRegionType_MemoryController1         | KMemoryRegionAttr_NoUserMap));
            MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x7000E000, 0x400,     KMemoryRegionType_None                      | KMemoryRegionAttr_NoUserMap));
            MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x7000E400, 0xC00,     KMemoryRegionType_PowerManagementController | KMemoryRegionAttr_NoUserMap));
            MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x50040000, 0x1000,    KMemoryRegionType_None                      | KMemoryRegionAttr_NoUserMap));
            MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x50041000, 0x1000,    KMemoryRegionType_InterruptDistributor      | KMemoryRegionAttr_ShouldKernelMap));
            MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x50042000, 0x1000,    KMemoryRegionType_InterruptCpuInterface     | KMemoryRegionAttr_ShouldKernelMap));
            MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x50043000, 0x1D000,   KMemoryRegionType_None                      | KMemoryRegionAttr_NoUserMap));
            MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x6000F000, 0x1000,    KMemoryRegionType_None                      | KMemoryRegionAttr_NoUserMap));
            MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x6001DC00, 0x400,     KMemoryRegionType_None                      | KMemoryRegionAttr_NoUserMap));
        }

        void SetupDramPhysicalMemoryRegions() {
            const size_t intended_memory_size                   = KSystemControl::Init::GetIntendedMemorySize();
            const KPhysicalAddress physical_memory_base_address = KSystemControl::Init::GetKernelPhysicalBaseAddress(DramPhysicalAddress);

            /* Insert blocks into the tree. */
            MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(GetInteger(physical_memory_base_address), intended_memory_size,  KMemoryRegionType_Dram));
            MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(GetInteger(physical_memory_base_address), ReservedEarlyDramSize, KMemoryRegionType_DramReservedEarly));
        }

    }

}