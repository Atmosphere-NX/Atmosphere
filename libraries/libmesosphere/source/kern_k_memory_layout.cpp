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

        class KMemoryRegionAllocator {
            NON_COPYABLE(KMemoryRegionAllocator);
            NON_MOVEABLE(KMemoryRegionAllocator);
            public:
                static constexpr size_t MaxMemoryRegions = 200;
            private:
                KMemoryRegion region_heap[MaxMemoryRegions];
                size_t num_regions;
            public:
                constexpr ALWAYS_INLINE KMemoryRegionAllocator() : region_heap(), num_regions() { /* ... */ }
            public:
                template<typename... Args>
                ALWAYS_INLINE KMemoryRegion *Allocate(Args&&... args) {
                    /* Ensure we stay within the bounds of our heap. */
                    MESOSPHERE_INIT_ABORT_UNLESS(this->num_regions < MaxMemoryRegions);

                    /* Create the new region. */
                    KMemoryRegion *region = std::addressof(this->region_heap[this->num_regions++]);
                    new (region) KMemoryRegion(std::forward<Args>(args)...);

                    return region;
                }
        };

        constinit KMemoryRegionAllocator g_memory_region_allocator;

        template<typename... Args>
        ALWAYS_INLINE KMemoryRegion *AllocateRegion(Args&&... args) {
            return g_memory_region_allocator.Allocate(std::forward<Args>(args)...);
        }


    }

    void KMemoryRegionTree::InsertDirectly(uintptr_t address, uintptr_t last_address, u32 attr, u32 type_id) {
        this->insert(*AllocateRegion(address, last_address, attr, type_id));
    }

    bool KMemoryRegionTree::Insert(uintptr_t address, size_t size, u32 type_id, u32 new_attr, u32 old_attr) {
        /* Locate the memory region that contains the address. */
        KMemoryRegion *found = this->FindModifiable(address);

        /* We require that the old attr is correct. */
        if (found->GetAttributes() != old_attr) {
            return false;
        }

        /* We further require that the region can be split from the old region. */
        const uintptr_t inserted_region_end = address + size;
        const uintptr_t inserted_region_last = inserted_region_end - 1;
        if (found->GetLastAddress() < inserted_region_last) {
            return false;
        }

        /* Further, we require that the type id is a valid transformation. */
        if (!found->CanDerive(type_id)) {
            return false;
        }

        /* Cache information from the region before we remove it. */
        const uintptr_t old_address = found->GetAddress();
        const uintptr_t old_last    = found->GetLastAddress();
        const uintptr_t old_pair    = found->GetPairAddress();
        const u32       old_type    = found->GetType();

        /* Erase the existing region from the tree. */
        this->erase(this->iterator_to(*found));

        /* Insert the new region into the tree. */
        if (old_address == address) {
            /* Reuse the old object for the new region, if we can. */
            found->Reset(address, inserted_region_last, old_pair, new_attr, type_id);
            this->insert(*found);
        } else {
            /* If we can't re-use, adjust the old region. */
            found->Reset(old_address, address - 1, old_pair, old_attr, old_type);
            this->insert(*found);

            /* Insert a new region for the split. */
            const uintptr_t new_pair = (old_pair != std::numeric_limits<uintptr_t>::max()) ? old_pair + (address - old_address) : old_pair;
            this->insert(*AllocateRegion(address, inserted_region_last, new_pair, new_attr, type_id));
        }

        /* If we need to insert a region after the region, do so. */
        if (old_last != inserted_region_last) {
            const uintptr_t after_pair = (old_pair != std::numeric_limits<uintptr_t>::max()) ? old_pair + (inserted_region_end - old_address) : old_pair;
            this->insert(*AllocateRegion(inserted_region_end, old_last, after_pair, old_attr, old_type));
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

        const uintptr_t first_index = first_address / alignment;
        const uintptr_t last_index  = last_address / alignment;

        while (true) {
            const uintptr_t candidate = KSystemControl::Init::GenerateRandomRange(first_index, last_index) * alignment;

            /* Ensure that the candidate doesn't overflow with the size. */
            if (!(candidate < candidate + size)) {
                continue;
            }

            const uintptr_t candidate_last = candidate + size - 1;

            /* Ensure that the candidate fits within the region. */
            if (candidate_last > last_address) {
                continue;
            }

            /* Locate the candidate region, and ensure it fits and has the correct type id. */
            if (const auto &candidate_region = *this->Find(candidate); !(candidate_last <= candidate_region.GetLastAddress() && candidate_region.GetType() == type_id)) {
                continue;
            }

            return candidate;
        }
    }

    void KMemoryLayout::InitializeLinearMemoryRegionTrees(KPhysicalAddress aligned_linear_phys_start, KVirtualAddress linear_virtual_start) {
        /* Set static differences. */
        s_linear_phys_to_virt_diff = GetInteger(linear_virtual_start) - GetInteger(aligned_linear_phys_start);
        s_linear_virt_to_phys_diff = GetInteger(aligned_linear_phys_start) - GetInteger(linear_virtual_start);

        /* Initialize linear trees. */
        for (auto &region : GetPhysicalMemoryRegionTree()) {
            if (region.HasTypeAttribute(KMemoryRegionAttr_LinearMapped)) {
                GetPhysicalLinearMemoryRegionTree().InsertDirectly(region.GetAddress(), region.GetLastAddress(), region.GetAttributes(), region.GetType());
            }
        }

        for (auto &region : GetVirtualMemoryRegionTree()) {
            if (region.IsDerivedFrom(KMemoryRegionType_Dram)) {
                GetVirtualLinearMemoryRegionTree().InsertDirectly(region.GetAddress(), region.GetLastAddress(), region.GetAttributes(), region.GetType());
            }
        }
    }

    size_t KMemoryLayout::GetResourceRegionSizeForInit() {
        /* Calculate resource region size based on whether we allow extra threads. */
        const bool use_extra_resources = KSystemControl::Init::ShouldIncreaseThreadResourceLimit();
        size_t resource_region_size = KernelResourceSize + (use_extra_resources ? KernelSlabHeapAdditionalSize : 0);

        /* 10.0.0 reduced the slab heap gaps by 64K. */
        if (kern::GetTargetFirmware() < ams::TargetFirmware_10_0_0) {
            resource_region_size += (KernelSlabHeapGapsSizeDeprecated - KernelSlabHeapGapsSize);
        }

        return resource_region_size;
    }

}
