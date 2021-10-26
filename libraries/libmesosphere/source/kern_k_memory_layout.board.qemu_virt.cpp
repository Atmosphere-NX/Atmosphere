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
#include <mesosphere.hpp>

namespace ams::kern {

    namespace {

        constexpr size_t ReservedEarlyDramSize  = 0x00080000;

        template<typename... T> requires (std::same_as<T, KMemoryRegionAttr> && ...)
        constexpr ALWAYS_INLINE KMemoryRegionType GetMemoryRegionType(KMemoryRegionType base, T... attr) {
            return util::FromUnderlying<KMemoryRegionType>(util::ToUnderlying(base) | (util::ToUnderlying<T>(attr) | ...));
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
            MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x08000000, 0x10000,  GetMemoryRegionType(KMemoryRegionType_InterruptDistributor,  KMemoryRegionAttr_ShouldKernelMap)));
            MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(0x08010000, 0x10000,  GetMemoryRegionType(KMemoryRegionType_InterruptCpuInterface, KMemoryRegionAttr_ShouldKernelMap)));
        }

        void SetupDramPhysicalMemoryRegions() {
            const size_t intended_memory_size                   = KSystemControl::Init::GetIntendedMemorySize();
            const KPhysicalAddress physical_memory_base_address = KSystemControl::Init::GetKernelPhysicalBaseAddress(ams::kern::MainMemoryAddress);

            /* Insert blocks into the tree. */
            MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(GetInteger(physical_memory_base_address), intended_memory_size,  KMemoryRegionType_Dram));
            MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(GetInteger(physical_memory_base_address), ReservedEarlyDramSize, KMemoryRegionType_DramReservedEarly));

            /* Insert the KTrace block at the end of Dram, if KTrace is enabled. */
            static_assert(!IsKTraceEnabled || KTraceBufferSize > 0);
            if constexpr (IsKTraceEnabled) {
                const KPhysicalAddress ktrace_buffer_phys_addr = physical_memory_base_address + intended_memory_size - KTraceBufferSize;
                MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(GetInteger(ktrace_buffer_phys_addr), KTraceBufferSize, KMemoryRegionType_KernelTraceBuffer));
            }
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

            /* Find the start of the pool partitions region. */
            const KMemoryRegion *pool_partitions_region = KMemoryLayout::GetPhysicalMemoryRegionTree().FindByTypeAndAttribute(KMemoryRegionType_DramPoolPartition, 0);
            MESOSPHERE_INIT_ABORT_UNLESS(pool_partitions_region != nullptr);
            const uintptr_t pool_partitions_start = pool_partitions_region->GetAddress();

            /* Setup the pool partition layouts. */
            /* Get Application and Applet pool sizes. */
            const size_t application_pool_size       = KSystemControl::Init::GetApplicationPoolSize();
            const size_t applet_pool_size            = KSystemControl::Init::GetAppletPoolSize();
            const size_t unsafe_system_pool_min_size = KSystemControl::Init::GetMinimumNonSecureSystemPoolSize();

            /* Decide on starting addresses for our pools. */
            const uintptr_t application_pool_start   = pool_end - application_pool_size;
            const uintptr_t applet_pool_start        = application_pool_start - applet_pool_size;
            const uintptr_t unsafe_system_pool_start = util::AlignDown(applet_pool_start - unsafe_system_pool_min_size, PageSize);
            const size_t    unsafe_system_pool_size  = applet_pool_start - unsafe_system_pool_start;

            /* We want to arrange application pool depending on where the middle of dram is. */
            const uintptr_t dram_midpoint = (dram_extents.GetAddress() + dram_extents.GetEndAddress()) / 2;
            u32 cur_pool_attr = 0;
            size_t total_overhead_size = 0;

            /* Insert the application pool. */
            if (application_pool_size > 0) {
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
            }

            /* Insert the applet pool. */
            if (applet_pool_size > 0) {
                InsertPoolPartitionRegionIntoBothTrees(applet_pool_start, applet_pool_size, KMemoryRegionType_DramAppletPool, KMemoryRegionType_VirtualDramAppletPool, cur_pool_attr);
                total_overhead_size += KMemoryManager::CalculateManagementOverheadSize(applet_pool_size);
            }

            /* Insert the nonsecure system pool. */
            if (unsafe_system_pool_size > 0) {
                InsertPoolPartitionRegionIntoBothTrees(unsafe_system_pool_start, unsafe_system_pool_size, KMemoryRegionType_DramSystemNonSecurePool, KMemoryRegionType_VirtualDramSystemNonSecurePool, cur_pool_attr);
                total_overhead_size += KMemoryManager::CalculateManagementOverheadSize(unsafe_system_pool_size);
            }

            /* Determine final total overhead size. */
            total_overhead_size += KMemoryManager::CalculateManagementOverheadSize((unsafe_system_pool_start - pool_partitions_start) - total_overhead_size);

            /* NOTE: Nintendo's kernel has layout [System, Management] but we have [Management, System]. This ensures the four UserPool regions are contiguous. */

            /* Insert the system pool. */
            const uintptr_t system_pool_start = pool_partitions_start + total_overhead_size;
            const size_t    system_pool_size  = unsafe_system_pool_start - system_pool_start;
            InsertPoolPartitionRegionIntoBothTrees(system_pool_start, system_pool_size, KMemoryRegionType_DramSystemPool, KMemoryRegionType_VirtualDramSystemPool, cur_pool_attr);

            /* Insert the pool management region. */
            const uintptr_t pool_management_start = pool_partitions_start;
            const size_t    pool_management_size  = total_overhead_size;
            u32 pool_management_attr = 0;
            InsertPoolPartitionRegionIntoBothTrees(pool_management_start, pool_management_size, KMemoryRegionType_DramPoolManagement, KMemoryRegionType_VirtualDramPoolManagement, pool_management_attr);
        }

    }

}