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
            #if   defined(MESOSPHERE_DEBUG_LOG_USE_UART)
                switch (KSystemControl::Init::GetDebugLogUartPort()) {
                    case 0:  return KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x70006000, 0x40,  KMemoryRegionType_Uart | KMemoryRegionAttr_ShouldKernelMap);
                    case 1:  return KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x70006040, 0x40,  KMemoryRegionType_Uart | KMemoryRegionAttr_ShouldKernelMap);
                    case 2:  return KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x70006200, 0x100, KMemoryRegionType_Uart | KMemoryRegionAttr_ShouldKernelMap);
                    case 3:  return KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x70006300, 0x100, KMemoryRegionType_Uart | KMemoryRegionAttr_ShouldKernelMap);
                    default: return false;
                }
            #elif defined(MESOSPHERE_DEBUG_LOG_USE_IRAM_RINGBUFFER)
                return true;
            #else
                #error "Unknown Debug UART device!"
            #endif
        }

        ALWAYS_INLINE bool SetupPowerManagementControllerMemoryRegion() {
            /* For backwards compatibility, the PMC must remain mappable on < 2.0.0. */
            const auto rtc_restrict_attr = GetTargetFirmware() >= TargetFirmware_2_0_0 ? KMemoryRegionAttr_NoUserMap : static_cast<KMemoryRegionAttr>(0);
            const auto pmc_restrict_attr = GetTargetFirmware() >= TargetFirmware_2_0_0 ? KMemoryRegionAttr_NoUserMap : KMemoryRegionAttr_ShouldKernelMap;

            return KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x7000E000, 0x400, KMemoryRegionType_None                      | rtc_restrict_attr) &&
                   KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x7000E400, 0xC00, KMemoryRegionType_PowerManagementController | pmc_restrict_attr);
        }

        void InsertPoolPartitionRegionIntoBothTrees(size_t start, size_t size, KMemoryRegionType phys_type, KMemoryRegionType virt_type, u32 &cur_attr) {
            const u32 attr = cur_attr++;
            MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(start, size, phys_type, attr));
            const KMemoryRegion *phys = KMemoryLayout::GetPhysicalMemoryRegionTree().FindByTypeAndAttribute(phys_type, attr);
            MESOSPHERE_INIT_ABORT_UNLESS(phys != nullptr);
            MESOSPHERE_INIT_ABORT_UNLESS(phys->GetEndAddress() != 0);
            MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetVirtualMemoryRegionTree().Insert(phys->GetPairAddress(), size, virt_type, attr));
        }

    }

    namespace init {

        void SetupDevicePhysicalMemoryRegions() {
            /* TODO: Give these constexpr defines somewhere? */
            MESOSPHERE_INIT_ABORT_UNLESS(SetupUartPhysicalMemoryRegion());
            MESOSPHERE_INIT_ABORT_UNLESS(SetupPowerManagementControllerMemoryRegion());
            MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x70019000, 0x1000,  KMemoryRegionType_MemoryController          | KMemoryRegionAttr_NoUserMap));
            MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x7001C000, 0x1000,  KMemoryRegionType_MemoryController0         | KMemoryRegionAttr_NoUserMap));
            MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x7001D000, 0x1000,  KMemoryRegionType_MemoryController1         | KMemoryRegionAttr_NoUserMap));
            MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x50040000, 0x1000,  KMemoryRegionType_None                      | KMemoryRegionAttr_NoUserMap));
            MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x50041000, 0x1000,  KMemoryRegionType_InterruptDistributor      | KMemoryRegionAttr_ShouldKernelMap));
            MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x50042000, 0x1000,  KMemoryRegionType_InterruptCpuInterface     | KMemoryRegionAttr_ShouldKernelMap));
            MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x50043000, 0x1D000, KMemoryRegionType_None                      | KMemoryRegionAttr_NoUserMap));

            /* Map IRAM unconditionally, to support debug-logging-to-iram build config. */
            MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x40000000, 0x40000, KMemoryRegionType_LegacyLpsIram | KMemoryRegionAttr_ShouldKernelMap));

            if (GetTargetFirmware() >= TargetFirmware_2_0_0) {
                /* Prevent mapping the bpmp exception vectors or the ipatch region. */
                MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x6000F000, 0x1000, KMemoryRegionType_None                      | KMemoryRegionAttr_NoUserMap));
                MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x6001DC00, 0x400,  KMemoryRegionType_None                      | KMemoryRegionAttr_NoUserMap));
            } else {
                /* Map devices required for legacy lps driver. */
                MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x6000F000, 0x1000, KMemoryRegionType_LegacyLpsExceptionVectors | KMemoryRegionAttr_ShouldKernelMap));
                MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x60007000, 0x1000, KMemoryRegionType_LegacyLpsFlowController   | KMemoryRegionAttr_ShouldKernelMap));
                MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x60004000, 0x1000, KMemoryRegionType_LegacyLpsPrimaryICtlr     | KMemoryRegionAttr_ShouldKernelMap));
                MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x60001000, 0x1000, KMemoryRegionType_LegacyLpsSemaphore        | KMemoryRegionAttr_ShouldKernelMap));
                MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x70016000, 0x1000, KMemoryRegionType_LegacyLpsAtomics          | KMemoryRegionAttr_ShouldKernelMap));
                MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x60006000, 0x1000, KMemoryRegionType_LegacyLpsClkRst           | KMemoryRegionAttr_ShouldKernelMap));
            }
        }

        void SetupDramPhysicalMemoryRegions() {
            const size_t intended_memory_size                   = KSystemControl::Init::GetIntendedMemorySize();
            const KPhysicalAddress physical_memory_base_address = KSystemControl::Init::GetKernelPhysicalBaseAddress(DramPhysicalAddress);

            /* Insert blocks into the tree. */
            MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(GetInteger(physical_memory_base_address), intended_memory_size,  KMemoryRegionType_Dram));
            MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(GetInteger(physical_memory_base_address), ReservedEarlyDramSize, KMemoryRegionType_DramReservedEarly));
        }

        void SetupPoolPartitionMemoryRegions() {
            /* Start by identifying the extents of the DRAM memory region. */
            const auto dram_extents = KMemoryLayout::GetMainMemoryPhysicalExtents();
            MESOSPHERE_INIT_ABORT_UNLESS(dram_extents.GetEndAddress() != 0);

            /* Determine the end of the pool region. */
            const uintptr_t pool_end = dram_extents.GetEndAddress() - KTraceBufferSize;

            /* Find the start of the kernel DRAM region. */
            const KMemoryRegion *kernel_dram_region = KMemoryLayout::GetPhysicalMemoryRegionTree().FindFirstDerived(KMemoryRegionType_DramKernelBase);
            MESOSPHERE_INIT_ABORT_UNLESS(kernel_dram_region != nullptr);

            const uintptr_t kernel_dram_start = kernel_dram_region->GetAddress();
            MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(kernel_dram_start, CarveoutAlignment));

            /* Find the start of the pool partitions region. */
            const KMemoryRegion *pool_partitions_region = KMemoryLayout::GetPhysicalMemoryRegionTree().FindByTypeAndAttribute(KMemoryRegionType_DramPoolPartition, 0);
            MESOSPHERE_INIT_ABORT_UNLESS(pool_partitions_region != nullptr);
            const uintptr_t pool_partitions_start = pool_partitions_region->GetAddress();

            /* Setup the pool partition layouts. */
            if (GetTargetFirmware() >= TargetFirmware_5_0_0) {
                /* On 5.0.0+, setup modern 4-pool-partition layout. */

                /* Get Application and Applet pool sizes. */
                const size_t application_pool_size       = KSystemControl::Init::GetApplicationPoolSize();
                const size_t applet_pool_size            = KSystemControl::Init::GetAppletPoolSize();
                const size_t unsafe_system_pool_min_size = KSystemControl::Init::GetMinimumNonSecureSystemPoolSize();

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
            } else {
                /* On < 5.0.0, setup a legacy 2-pool layout for backwards compatibility. */

                static_assert(KMemoryManager::Pool_Count == 4);
                static_assert(KMemoryManager::Pool_Unsafe == KMemoryManager::Pool_Application);
                static_assert(KMemoryManager::Pool_Secure == KMemoryManager::Pool_System);

                /* Get Secure pool size. */
                const size_t secure_pool_size = [] ALWAYS_INLINE_LAMBDA (auto target_firmware) -> size_t {
                    constexpr size_t LegacySecureKernelSize = 8_MB;          /* KPageBuffer pages, other small kernel allocations. */
                    constexpr size_t LegacySecureMiscSize   = 1_MB;          /* Miscellaneous pages for secure process mapping. */
                    constexpr size_t LegacySecureHeapSize   = 24_MB;         /* Heap pages for secure process mapping (fs). */
                    constexpr size_t LegacySecureEsSize     = 1_MB + 232_KB; /* Size for additional secure process (es, 4.0.0+). */

                    /* The baseline size for the secure region is enough to cover any allocations the kernel might make. */
                    size_t size = LegacySecureKernelSize;

                    /* If on 2.0.0+, initial processes will fall within the secure region. */
                    if (target_firmware >= TargetFirmware_2_0_0) {
                        /* Account for memory used directly for the processes. */
                        size += GetInitialProcessesSecureMemorySize();

                        /* Account for heap and transient memory used by the processes. */
                        size += LegacySecureHeapSize + LegacySecureMiscSize;
                    }

                    /* If on 4.0.0+, any process may use secure memory via a create process flag. */
                    /* In process this is used for es alone, and the secure pool's size should be */
                    /* increased to accommodate es's binary. */
                    if (target_firmware >= TargetFirmware_4_0_0) {
                        size += LegacySecureEsSize;
                    }

                    return size;
                }(GetTargetFirmware());

                /* Calculate the overhead for the secure and (defunct) applet/non-secure-system pools. */
                size_t total_overhead_size = KMemoryManager::CalculateManagementOverheadSize(secure_pool_size);

                /* Calculate the overhead for (an amount larger than) the unsafe pool. */
                const size_t approximate_total_overhead_size = total_overhead_size + KMemoryManager::CalculateManagementOverheadSize((pool_end - pool_partitions_start) - secure_pool_size - total_overhead_size) + 2 * PageSize;

                /* Determine the start of the unsafe region. */
                const uintptr_t unsafe_memory_start = util::AlignUp(pool_partitions_start + secure_pool_size + approximate_total_overhead_size, CarveoutAlignment);

                /* Determine the start of the pool regions. */
                const uintptr_t application_pool_start = unsafe_memory_start;

                /* Determine the pool sizes. */
                const size_t application_pool_size = pool_end - application_pool_start;

                /* We want to arrange application pool depending on where the middle of dram is. */
                const uintptr_t dram_midpoint = (dram_extents.GetAddress() + dram_extents.GetEndAddress()) / 2;
                u32 cur_pool_attr = 0;
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

                /* Insert the secure pool. */
                InsertPoolPartitionRegionIntoBothTrees(pool_partitions_start, secure_pool_size, KMemoryRegionType_DramSystemPool, KMemoryRegionType_VirtualDramSystemPool, cur_pool_attr);

                /* Insert the pool management region. */
                MESOSPHERE_INIT_ABORT_UNLESS(total_overhead_size <= approximate_total_overhead_size);

                const uintptr_t pool_management_start = pool_partitions_start + secure_pool_size;
                const size_t    pool_management_size  = unsafe_memory_start - pool_management_start;
                MESOSPHERE_INIT_ABORT_UNLESS(total_overhead_size <= pool_management_size);

                u32 pool_management_attr = 0;
                InsertPoolPartitionRegionIntoBothTrees(pool_management_start, pool_management_size, KMemoryRegionType_DramPoolManagement, KMemoryRegionType_VirtualDramPoolManagement, pool_management_attr);
            }
        }

    }

}