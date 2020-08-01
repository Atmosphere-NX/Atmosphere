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

    bool KMemoryRegionTree::Insert(uintptr_t address, size_t size, u32 type_id, u32 new_attr, u32 old_attr) {
        /* Locate the memory region that contains the address. */
        auto it = this->FindContainingRegion(address);

        /* We require that the old attr is correct. */
        if (it->GetAttributes() != old_attr) {
            return false;
        }

        /* We further require that the region can be split from the old region. */
        const uintptr_t inserted_region_end = address + size;
        const uintptr_t inserted_region_last = inserted_region_end - 1;
        if (it->GetLastAddress() < inserted_region_last) {
            return false;
        }

        /* Further, we require that the type id is a valid transformation. */
        if (!it->CanDerive(type_id)) {
            return false;
        }

        /* Cache information from the region before we remove it. */
        KMemoryRegion *cur_region = std::addressof(*it);
        const uintptr_t old_address = it->GetAddress();
        const size_t    old_size    = it->GetSize();
        const uintptr_t old_end     = old_address + old_size;
        const uintptr_t old_last    = old_end - 1;
        const uintptr_t old_pair    = it->GetPairAddress();
        const u32       old_type    = it->GetType();

        /* Erase the existing region from the tree. */
        this->erase(it);

        /* If we need to insert a region before the region, do so. */
        if (old_address != address) {
            new (cur_region) KMemoryRegion(old_address, address - old_address, old_pair, old_attr, old_type);
            this->insert(*cur_region);
            cur_region = KMemoryLayout::GetMemoryRegionAllocator().Allocate();
        }

        /* Insert a new region. */
        const uintptr_t new_pair = (old_pair != std::numeric_limits<uintptr_t>::max()) ? old_pair + (address - old_address) : old_pair;
        new (cur_region) KMemoryRegion(address, size, new_pair, new_attr, type_id);
        this->insert(*cur_region);

        /* If we need to insert a region after the region, do so. */
        if (old_last != inserted_region_last) {
            const uintptr_t after_pair = (old_pair != std::numeric_limits<uintptr_t>::max()) ? old_pair + (inserted_region_end - old_address) : old_pair;
            this->insert(*KMemoryLayout::GetMemoryRegionAllocator().Create(inserted_region_end, old_end - inserted_region_end, after_pair, old_attr, old_type));
        }

        return true;
    }

    KVirtualAddress KMemoryRegionTree::GetRandomAlignedRegion(size_t size, size_t alignment, u32 type_id) {
        /* We want to find the total extents of the type id. */
        const auto extents = this->GetDerivedRegionExtents(type_id);

        /* Ensure that our alignment is correct. */
        MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(extents.GetAddress(), alignment));

        const uintptr_t first_address = extents.GetAddress();
        const uintptr_t last_address  = extents.GetLastAddress();

        while (true) {
            const uintptr_t candidate = util::AlignDown(KSystemControl::Init::GenerateRandomRange(first_address, last_address), alignment);

            /* Ensure that the candidate doesn't overflow with the size. */
            if (!(candidate < candidate + size)) {
                continue;
            }

            const uintptr_t candidate_last = candidate + size - 1;

            /* Ensure that the candidate fits within the region. */
            if (candidate_last > last_address) {
                continue;
            }

            /* Locate the candidate region, and ensure it fits. */
            const KMemoryRegion *candidate_region = std::addressof(*this->FindContainingRegion(candidate));
            if (candidate_last > candidate_region->GetLastAddress()) {
                continue;
            }

            /* Ensure that the region has the correct type id. */
            if (candidate_region->GetType() != type_id)
                continue;

            return candidate;
        }
    }

    void KMemoryLayout::InitializeLinearMemoryRegionTrees(KPhysicalAddress aligned_linear_phys_start, KVirtualAddress linear_virtual_start) {
        /* Set static differences. */
        s_linear_phys_to_virt_diff = GetInteger(linear_virtual_start) - GetInteger(aligned_linear_phys_start);
        s_linear_virt_to_phys_diff = GetInteger(aligned_linear_phys_start) - GetInteger(linear_virtual_start);

        /* Initialize linear trees. */
        for (auto &region : GetPhysicalMemoryRegionTree()) {
            if (!region.HasTypeAttribute(KMemoryRegionAttr_LinearMapped)) {
                continue;
            }
            GetPhysicalLinearMemoryRegionTree().insert(*GetMemoryRegionAllocator().Create(region.GetAddress(), region.GetSize(), region.GetAttributes(), region.GetType()));
        }

        for (auto &region : GetVirtualMemoryRegionTree()) {
            if (!region.IsDerivedFrom(KMemoryRegionType_Dram)) {
                continue;
            }
            GetVirtualLinearMemoryRegionTree().insert(*GetMemoryRegionAllocator().Create(region.GetAddress(), region.GetSize(), region.GetAttributes(), region.GetType()));
        }
    }

    namespace init {

        namespace {

            constexpr PageTableEntry KernelRwDataAttribute(PageTableEntry::Permission_KernelRW, PageTableEntry::PageAttribute_NormalMemory, PageTableEntry::Shareable_InnerShareable, PageTableEntry::MappingFlag_Mapped);

            constexpr size_t CarveoutAlignment             = 0x20000;
            constexpr size_t CarveoutSizeMax               = 512_MB - CarveoutAlignment;

            constexpr size_t CoreLocalRegionAlign          = PageSize;
            constexpr size_t CoreLocalRegionSize           = PageSize * (1 + cpu::NumCores);
            constexpr size_t CoreLocalRegionSizeWithGuards = CoreLocalRegionSize + 2 * PageSize;
            constexpr size_t CoreLocalRegionBoundsAlign    = 1_GB;
            static_assert(CoreLocalRegionSize == sizeof(KCoreLocalRegion));

            KVirtualAddress GetCoreLocalRegionVirtualAddress() {
                while (true) {
                    const uintptr_t candidate_start = GetInteger(KMemoryLayout::GetVirtualMemoryRegionTree().GetRandomAlignedRegion(CoreLocalRegionSizeWithGuards, CoreLocalRegionAlign, KMemoryRegionType_None));
                    const uintptr_t candidate_end   = candidate_start + CoreLocalRegionSizeWithGuards;
                    const uintptr_t candidate_last  = candidate_end - 1;

                    const KMemoryRegion *containing_region = std::addressof(*KMemoryLayout::GetVirtualMemoryRegionTree().FindContainingRegion(candidate_start));

                    if (candidate_last > containing_region->GetLastAddress()) {
                        continue;
                    }

                    if (containing_region->GetType() != KMemoryRegionType_None) {
                        continue;
                    }

                    if (util::AlignDown(candidate_start, CoreLocalRegionBoundsAlign) != util::AlignDown(candidate_last, CoreLocalRegionBoundsAlign)) {
                        continue;
                    }

                    if (containing_region->GetAddress() > util::AlignDown(candidate_start, CoreLocalRegionBoundsAlign)) {
                        continue;
                    }

                    if (util::AlignUp(candidate_last, CoreLocalRegionBoundsAlign) - 1 > containing_region->GetLastAddress()) {
                        continue;
                    }

                    return candidate_start + PageSize;
                }

            }

            void InsertPoolPartitionRegionIntoBothTrees(size_t start, size_t size, KMemoryRegionType phys_type, KMemoryRegionType virt_type, u32 &cur_attr) {
                const u32 attr = cur_attr++;
                MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(start, size, phys_type, attr));
                MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetVirtualMemoryRegionTree().Insert(KMemoryLayout::GetPhysicalMemoryRegionTree().FindFirstRegionByTypeAttr(phys_type, attr)->GetPairAddress(), size, virt_type, attr));
            }

        }

        void SetupCoreLocalRegionMemoryRegions(KInitialPageTable &page_table, KInitialPageAllocator &page_allocator) {
            const KVirtualAddress core_local_virt_start = GetCoreLocalRegionVirtualAddress();
            MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetVirtualMemoryRegionTree().Insert(GetInteger(core_local_virt_start), CoreLocalRegionSize, KMemoryRegionType_CoreLocal));

            /* Allocate a page for each core. */
            KPhysicalAddress core_local_region_start_phys[cpu::NumCores] = {};
            for (size_t i = 0; i < cpu::NumCores; i++) {
                core_local_region_start_phys[i] = page_allocator.Allocate();
            }

            /* Allocate an l1 page table for each core. */
            KPhysicalAddress core_l1_ttbr1_phys[cpu::NumCores] = {};
            core_l1_ttbr1_phys[0] = util::AlignDown(cpu::GetTtbr1El1(), PageSize);
            for (size_t i = 1; i < cpu::NumCores; i++) {
                core_l1_ttbr1_phys[i] = page_allocator.Allocate();
                std::memcpy(reinterpret_cast<void *>(GetInteger(core_l1_ttbr1_phys[i])), reinterpret_cast<void *>(GetInteger(core_l1_ttbr1_phys[0])), PageSize);
            }

            /* Use the l1 page table for each core to map the core local region for each core. */
            for (size_t i = 0; i < cpu::NumCores; i++) {
                KInitialPageTable temp_pt(core_l1_ttbr1_phys[i], KInitialPageTable::NoClear{});
                temp_pt.Map(core_local_virt_start, PageSize, core_local_region_start_phys[i], KernelRwDataAttribute, page_allocator);
                for (size_t j = 0; j < cpu::NumCores; j++) {
                    temp_pt.Map(core_local_virt_start + (j + 1) * PageSize, PageSize, core_local_region_start_phys[j], KernelRwDataAttribute, page_allocator);
                }

                /* Setup the InitArguments. */
                SetInitArguments(static_cast<s32>(i), core_local_region_start_phys[i], GetInteger(core_l1_ttbr1_phys[i]));
            }

            /* Ensure the InitArguments are flushed to cache. */
            StoreInitArguments();
        }

        void SetupPoolPartitionMemoryRegions() {
            /* Start by identifying the extents of the DRAM memory region. */
            const auto dram_extents = KMemoryLayout::GetMainMemoryPhysicalExtents();

            const uintptr_t pool_end = dram_extents.GetEndAddress() - KTraceBufferSize;

            /* Get Application and Applet pool sizes. */
            const size_t application_pool_size       = KSystemControl::Init::GetApplicationPoolSize();
            const size_t applet_pool_size            = KSystemControl::Init::GetAppletPoolSize();
            const size_t unsafe_system_pool_min_size = KSystemControl::Init::GetMinimumNonSecureSystemPoolSize();

            /* Find the start of the kernel DRAM region. */
            const uintptr_t kernel_dram_start = KMemoryLayout::GetPhysicalMemoryRegionTree().FindFirstDerivedRegion(KMemoryRegionType_DramKernel)->GetAddress();
            MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(kernel_dram_start, CarveoutAlignment));

            /* Find the start of the pool partitions region. */
            const uintptr_t pool_partitions_start = KMemoryLayout::GetPhysicalMemoryRegionTree().FindFirstRegionByTypeAttr(KMemoryRegionType_DramPoolPartition)->GetAddress();

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
                total_overhead_size += KMemoryManager::CalculateMetadataOverheadSize(application_pool_size);
            } else {
                const size_t first_application_pool_size  = dram_midpoint - application_pool_start;
                const size_t second_application_pool_size = application_pool_start + application_pool_size - dram_midpoint;
                InsertPoolPartitionRegionIntoBothTrees(application_pool_start, first_application_pool_size, KMemoryRegionType_DramApplicationPool, KMemoryRegionType_VirtualDramApplicationPool, cur_pool_attr);
                InsertPoolPartitionRegionIntoBothTrees(dram_midpoint, second_application_pool_size, KMemoryRegionType_DramApplicationPool, KMemoryRegionType_VirtualDramApplicationPool, cur_pool_attr);
                total_overhead_size += KMemoryManager::CalculateMetadataOverheadSize(first_application_pool_size);
                total_overhead_size += KMemoryManager::CalculateMetadataOverheadSize(second_application_pool_size);
            }

            /* Insert the applet pool. */
            InsertPoolPartitionRegionIntoBothTrees(applet_pool_start, applet_pool_size, KMemoryRegionType_DramAppletPool, KMemoryRegionType_VirtualDramAppletPool, cur_pool_attr);
            total_overhead_size += KMemoryManager::CalculateMetadataOverheadSize(applet_pool_size);

            /* Insert the nonsecure system pool. */
            InsertPoolPartitionRegionIntoBothTrees(unsafe_system_pool_start, unsafe_system_pool_size, KMemoryRegionType_DramSystemNonSecurePool, KMemoryRegionType_VirtualDramSystemNonSecurePool, cur_pool_attr);
            total_overhead_size += KMemoryManager::CalculateMetadataOverheadSize(unsafe_system_pool_size);

            /* Insert the metadata pool. */
            total_overhead_size += KMemoryManager::CalculateMetadataOverheadSize((unsafe_system_pool_start - pool_partitions_start) - total_overhead_size);
            const uintptr_t metadata_pool_start = unsafe_system_pool_start - total_overhead_size;
            const size_t    metadata_pool_size  = total_overhead_size;
            u32 metadata_pool_attr = 0;
            InsertPoolPartitionRegionIntoBothTrees(metadata_pool_start, metadata_pool_size, KMemoryRegionType_DramMetadataPool, KMemoryRegionType_VirtualDramMetadataPool, metadata_pool_attr);

            /* Insert the system pool. */
            const uintptr_t system_pool_size = metadata_pool_start - pool_partitions_start;
            InsertPoolPartitionRegionIntoBothTrees(pool_partitions_start, system_pool_size, KMemoryRegionType_DramSystemPool, KMemoryRegionType_VirtualDramSystemPool, cur_pool_attr);

        }

    }


}
