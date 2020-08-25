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

        constexpr size_t CarveoutAlignment      = 0x20000;
        constexpr size_t CarveoutSizeMax        = 512_MB - CarveoutAlignment;

        ALWAYS_INLINE bool SetupUartPhysicalMemoryRegion() {
            #if   defined(MESOSPHERE_DEBUG_LOG_USE_UART_A)
                return KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x70006000, 0x40,  KMemoryRegionType_Uart | KMemoryRegionAttr_ShouldKernelMap);
            #elif defined(MESOSPHERE_DEBUG_LOG_USE_UART_B)
                return KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x70006040, 0x40,  KMemoryRegionType_Uart | KMemoryRegionAttr_ShouldKernelMap);
            #elif defined(MESOSPHERE_DEBUG_LOG_USE_UART_C)
                return KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x70006200, 0x100, KMemoryRegionType_Uart | KMemoryRegionAttr_ShouldKernelMap);
            #elif defined(MESOSPHERE_DEBUG_LOG_USE_UART_D)
                return KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x70006300, 0x100, KMemoryRegionType_Uart | KMemoryRegionAttr_ShouldKernelMap);
            #else
                #error "Unknown Debug UART device!"
            #endif
        }

        ALWAYS_INLINE bool SetupPowerManagementControllerMemoryRegion() {
            /* For backwards compatibility, the PMC must remain mappable on < 2.0.0. */
            const auto restrict_attr = GetTargetFirmware() >= TargetFirmware_2_0_0 ? KMemoryRegionAttr_NoUserMap : static_cast<KMemoryRegionAttr>(0);

            return KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x7000E000, 0x400, KMemoryRegionType_None                      | restrict_attr) &&
                   KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x7000E400, 0xC00, KMemoryRegionType_PowerManagementController | restrict_attr);
        }

        void InsertPoolPartitionRegionIntoBothTrees(size_t start, size_t size, KMemoryRegionType phys_type, KMemoryRegionType virt_type, u32 &cur_attr) {
            const u32 attr = cur_attr++;
            MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(start, size, phys_type, attr));
            const KMemoryRegion *phys = KMemoryLayout::GetPhysicalMemoryRegionTree().FindByTypeAndAttribute(phys_type, attr);
            MESOSPHERE_INIT_ABORT_UNLESS(phys != nullptr);
            MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetVirtualMemoryRegionTree().Insert(phys->GetPairAddress(), size, virt_type, attr));
        }

    }

    namespace init {

        namespace {

            void SetupPoolPartitionMemoryRegionsImpl() {
                /* Start by identifying the extents of the DRAM memory region. */
                const auto dram_extents = KMemoryLayout::GetMainMemoryPhysicalExtents();

                const uintptr_t pool_end = dram_extents.GetEndAddress() - KTraceBufferSize;

                /* Get Application and Applet pool sizes. */
                const size_t application_pool_size       = KSystemControl::Init::GetApplicationPoolSize();
                const size_t applet_pool_size            = KSystemControl::Init::GetAppletPoolSize();
                const size_t unsafe_system_pool_min_size = KSystemControl::Init::GetMinimumNonSecureSystemPoolSize();

                /* Find the start of the kernel DRAM region. */
                const KMemoryRegion *kernel_dram_region = KMemoryLayout::GetPhysicalMemoryRegionTree().FindFirstDerived(KMemoryRegionType_DramKernelBase);
                MESOSPHERE_INIT_ABORT_UNLESS(kernel_dram_region != nullptr);

                const uintptr_t kernel_dram_start = kernel_dram_region->GetAddress();
                MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(kernel_dram_start, CarveoutAlignment));

                /* Find the start of the pool partitions region. */
                const KMemoryRegion *pool_partitions_region = KMemoryLayout::GetPhysicalMemoryRegionTree().FindByTypeAndAttribute(KMemoryRegionType_DramPoolPartition, 0);
                MESOSPHERE_INIT_ABORT_UNLESS(pool_partitions_region != nullptr);
                const uintptr_t pool_partitions_start = pool_partitions_region->GetAddress();

                /* Decide on starting addresses for our pools. */
                const uintptr_t application_pool_start   = pool_end - application_pool_size;
                const uintptr_t applet_pool_start        = application_pool_start - applet_pool_size;
                const uintptr_t unsafe_system_pool_start = std::min(kernel_dram_start + CarveoutSizeMax, util::AlignDown(applet_pool_start - unsafe_system_pool_min_size, CarveoutAlignment));
                const size_t    unsafe_system_pool_size  = applet_pool_start - unsafe_system_pool_start;

                /* We want to arrange application pool depending on where the middle of dram is. */
                const uintptr_t dram_midpoint = (dram_extents.GetAddress() + dram_extents.GetEndAddress()) / 2;
                u32 cur_pool_attr = 0;
                size_t total_overhead_size = 0;
                if (dram_extents.GetEndAddress() <= dram_midpoint || dram_midpoint <= application_pool_start) {
                    InsertPoolPartitionRegionIntoBothTrees(application_pool_start, application_pool_size, KMemoryRegionType_DramApplicationPool, KMemoryRegionType_VirtualDramApplicationPool, cur_pool_attr);
                    total_overhead_size += KMemoryManager::CalculateManagementOverheadSize(application_pool_size);
                } else {
                    const size_t first_application_pool_size  = dram_midpoint - application_pool_start;
                    const size_t second_application_pool_size = application_pool_start + application_pool_size - dram_midpoint;
                    InsertPoolPartitionRegionIntoBothTrees(application_pool_start, first_application_pool_size, KMemoryRegionType_DramApplicationPool, KMemoryRegionType_VirtualDramApplicationPool, cur_pool_attr);
                    InsertPoolPartitionRegionIntoBothTrees(dram_midpoint, second_application_pool_size, KMemoryRegionType_DramApplicationPool, KMemoryRegionType_VirtualDramApplicationPool, cur_pool_attr);
                    total_overhead_size += KMemoryManager::CalculateManagementOverheadSize(first_application_pool_size);
                    total_overhead_size += KMemoryManager::CalculateManagementOverheadSize(second_application_pool_size);
                }

                /* Insert the applet pool. */
                InsertPoolPartitionRegionIntoBothTrees(applet_pool_start, applet_pool_size, KMemoryRegionType_DramAppletPool, KMemoryRegionType_VirtualDramAppletPool, cur_pool_attr);
                total_overhead_size += KMemoryManager::CalculateManagementOverheadSize(applet_pool_size);

                /* Insert the nonsecure system pool. */
                InsertPoolPartitionRegionIntoBothTrees(unsafe_system_pool_start, unsafe_system_pool_size, KMemoryRegionType_DramSystemNonSecurePool, KMemoryRegionType_VirtualDramSystemNonSecurePool, cur_pool_attr);
                total_overhead_size += KMemoryManager::CalculateManagementOverheadSize(unsafe_system_pool_size);

                /* Insert the pool management region. */
                total_overhead_size += KMemoryManager::CalculateManagementOverheadSize((unsafe_system_pool_start - pool_partitions_start) - total_overhead_size);
                const uintptr_t pool_management_start = unsafe_system_pool_start - total_overhead_size;
                const size_t    pool_management_size  = total_overhead_size;
                u32 pool_management_attr = 0;
                InsertPoolPartitionRegionIntoBothTrees(pool_management_start, pool_management_size, KMemoryRegionType_DramPoolManagement, KMemoryRegionType_VirtualDramPoolManagement, pool_management_attr);

                /* Insert the system pool. */
                const uintptr_t system_pool_size = pool_management_start - pool_partitions_start;
                InsertPoolPartitionRegionIntoBothTrees(pool_partitions_start, system_pool_size, KMemoryRegionType_DramSystemPool, KMemoryRegionType_VirtualDramSystemPool, cur_pool_attr);
            }

            void SetupPoolPartitionMemoryRegionsDeprecatedImpl() {
                MESOSPHERE_UNIMPLEMENTED();
            }

        }

        void SetupDevicePhysicalMemoryRegions() {
            /* TODO: Give these constexpr defines somewhere? */
            MESOSPHERE_INIT_ABORT_UNLESS(SetupUartPhysicalMemoryRegion());
            MESOSPHERE_INIT_ABORT_UNLESS(SetupPowerManagementControllerMemoryRegion());
            MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x70019000, 0x1000,    KMemoryRegionType_MemoryController          | KMemoryRegionAttr_NoUserMap));
            MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x7001C000, 0x1000,    KMemoryRegionType_MemoryController0         | KMemoryRegionAttr_NoUserMap));
            MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x7001D000, 0x1000,    KMemoryRegionType_MemoryController1         | KMemoryRegionAttr_NoUserMap));
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

        void SetupPoolPartitionMemoryRegions() {
            if (GetTargetFirmware() >= TargetFirmware_5_0_0) {
                /* On 5.0.0+, setup modern 4-pool-partition layout. */
                SetupPoolPartitionMemoryRegionsImpl();
            } else {
                /* On < 5.0.0, setup a legacy 2-pool layout for backwards compatibility. */
                SetupPoolPartitionMemoryRegionsDeprecatedImpl();
            }
        }

    }

}