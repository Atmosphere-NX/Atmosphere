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

    bool KMemoryBlockTree::Insert(uintptr_t address, size_t size, u32 type_id, u32 new_attr, u32 old_attr) {
        /* Locate the memory block that contains the address. */
        auto it = this->FindContainingBlock(address);

        /* We require that the old attr is correct. */
        if (it->GetAttributes() != old_attr) {
            return false;
        }

        /* We further require that the block can be split from the old block. */
        const uintptr_t inserted_block_end = address + size;
        const uintptr_t inserted_block_last = inserted_block_end - 1;
        if (it->GetLastAddress() < inserted_block_last) {
            return false;
        }

        /* Further, we require that the type id is a valid transformation. */
        if (!it->CanDerive(type_id)) {
            return false;
        }

        /* Cache information from the block before we remove it. */
        KMemoryBlock *cur_block = std::addressof(*it);
        const uintptr_t old_address = it->GetAddress();
        const size_t    old_size    = it->GetSize();
        const uintptr_t old_end     = old_address + old_size;
        const uintptr_t old_last    = old_end - 1;
        const uintptr_t old_pair    = it->GetPairAddress();
        const u32       old_type    = it->GetType();

        /* Erase the existing block from the tree. */
        this->erase(it);

        /* If we need to insert a block before the region, do so. */
        if (old_address != address) {
            new (cur_block) KMemoryBlock(old_address, address - old_address, old_pair, old_attr, old_type);
            this->insert(*cur_block);
            cur_block = KMemoryLayout::GetMemoryBlockAllocator().Allocate();
        }

        /* Insert a new block. */
        const uintptr_t new_pair = (old_pair != std::numeric_limits<uintptr_t>::max()) ? old_pair + (address - old_address) : old_pair;
        new (cur_block) KMemoryBlock(address, size, new_pair, new_attr, type_id);
        this->insert(*cur_block);

        /* If we need to insert a block after the region, do so. */
        if (old_last != inserted_block_last) {
            const uintptr_t after_pair = (old_pair != std::numeric_limits<uintptr_t>::max()) ? old_pair + (inserted_block_end - old_address) : old_pair;
            this->insert(*KMemoryLayout::GetMemoryBlockAllocator().Create(inserted_block_end, old_end - inserted_block_end, after_pair, old_attr, old_type));
        }

        return true;
    }

    KVirtualAddress KMemoryBlockTree::GetRandomAlignedRegion(size_t size, size_t alignment, u32 type_id) {
        /* We want to find the total extents of the type id. */
        const auto extents = this->GetDerivedRegionExtents(type_id);

        /* Ensure that our alignment is correct. */
        MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(extents.first_block->GetAddress(), alignment));

        const uintptr_t first_address = extents.first_block->GetAddress();
        const uintptr_t last_address  = extents.last_block->GetLastAddress();

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

            /* Locate the candidate block, and ensure it fits. */
            const KMemoryBlock *candidate_block = std::addressof(*this->FindContainingBlock(candidate));
            if (candidate_last > candidate_block->GetLastAddress()) {
                continue;
            }

            /* Ensure that the block has the correct type id. */
            if (candidate_block->GetType() != type_id)
                continue;

            return candidate;
        }
    }

    void KMemoryLayout::InitializeLinearMemoryBlockTrees(KPhysicalAddress aligned_linear_phys_start, KVirtualAddress linear_virtual_start) {
        /* Set static differences. */
        s_linear_phys_to_virt_diff = GetInteger(linear_virtual_start) - GetInteger(aligned_linear_phys_start);
        s_linear_virt_to_phys_diff = GetInteger(aligned_linear_phys_start) - GetInteger(linear_virtual_start);

        /* Initialize linear trees. */
        for (auto &block : GetPhysicalMemoryBlockTree()) {
            if (!block.HasTypeAttribute(KMemoryRegionAttr_LinearMapped)) {
                continue;
            }
            GetPhysicalLinearMemoryBlockTree().insert(*GetMemoryBlockAllocator().Create(block.GetAddress(), block.GetSize(), block.GetAttributes(), block.GetType()));
        }

        for (auto &block : GetVirtualMemoryBlockTree()) {
            if (!block.IsDerivedFrom(KMemoryRegionType_Dram)) {
                continue;
            }
            GetVirtualLinearMemoryBlockTree().insert(*GetMemoryBlockAllocator().Create(block.GetAddress(), block.GetSize(), block.GetAttributes(), block.GetType()));
        }
    }

    namespace init {

        namespace {


            constexpr PageTableEntry KernelRwDataAttribute(PageTableEntry::Permission_KernelRW, PageTableEntry::PageAttribute_NormalMemory, PageTableEntry::Shareable_InnerShareable);

            constexpr size_t CoreLocalRegionAlign          = PageSize;
            constexpr size_t CoreLocalRegionSize           = PageSize * (1 + cpu::NumCores);
            constexpr size_t CoreLocalRegionSizeWithGuards = CoreLocalRegionSize + 2 * PageSize;
            constexpr size_t CoreLocalRegionBoundsAlign    = 1_GB;
            /* TODO: static_assert(CoreLocalRegionSize == sizeof(KCoreLocalRegion)); */

            KVirtualAddress GetCoreLocalRegionVirtualAddress() {
                while (true) {
                    const uintptr_t candidate_start = GetInteger(KMemoryLayout::GetVirtualMemoryBlockTree().GetRandomAlignedRegion(CoreLocalRegionSizeWithGuards, CoreLocalRegionAlign, KMemoryRegionType_None));
                    const uintptr_t candidate_end   = candidate_start + CoreLocalRegionSizeWithGuards;
                    const uintptr_t candidate_last  = candidate_end - 1;

                    const KMemoryBlock *containing_block = std::addressof(*KMemoryLayout::GetVirtualMemoryBlockTree().FindContainingBlock(candidate_start));

                    if (candidate_last > containing_block->GetLastAddress()) {
                        continue;
                    }

                    if (containing_block->GetType() != KMemoryRegionType_None) {
                        continue;
                    }

                    if (util::AlignDown(candidate_start, CoreLocalRegionBoundsAlign) != util::AlignDown(candidate_last, CoreLocalRegionBoundsAlign)) {
                        continue;
                    }

                    if (containing_block->GetAddress() > util::AlignDown(candidate_start, CoreLocalRegionBoundsAlign)) {
                        continue;
                    }

                    if (util::AlignUp(candidate_last, CoreLocalRegionBoundsAlign) - 1 > containing_block->GetLastAddress()) {
                        continue;
                    }

                    return candidate_start + PageSize;
                }

            }

        }

        void SetupCoreLocalRegionMemoryBlocks(KInitialPageTable &page_table, KInitialPageAllocator &page_allocator) {
            const KVirtualAddress core_local_virt_start = GetCoreLocalRegionVirtualAddress();
            MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetVirtualMemoryBlockTree().Insert(GetInteger(core_local_virt_start), CoreLocalRegionSize, KMemoryRegionType_CoreLocal));

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
                temp_pt.Map(core_local_virt_start, PageSize, core_l1_ttbr1_phys[i], KernelRwDataAttribute, page_allocator);
                for (size_t j = 0; j < cpu::NumCores; j++) {
                    temp_pt.Map(core_local_virt_start + (j + 1) * PageSize, PageSize, core_l1_ttbr1_phys[j], KernelRwDataAttribute, page_allocator);
                }

                /* Setup the InitArguments. */
                SetInitArguments(static_cast<s32>(i), core_local_region_start_phys[i], GetInteger(core_l1_ttbr1_phys[i]));
            }

            /* Ensure the InitArguments are flushed to cache. */
            StoreInitArguments();
        }

        void SetupPoolPartitionMemoryBlocks() {
            /* TODO */
        }

    }


}
