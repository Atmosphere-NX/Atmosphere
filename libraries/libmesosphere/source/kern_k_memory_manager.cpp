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

        constexpr KMemoryManager::Pool GetPoolFromMemoryRegionType(u32 type) {
            switch (type) {
                case KMemoryRegionType_VirtualDramApplicationPool:     return KMemoryManager::Pool_Application;
                case KMemoryRegionType_VirtualDramAppletPool:          return KMemoryManager::Pool_Applet;
                case KMemoryRegionType_VirtualDramSystemPool:          return KMemoryManager::Pool_System;
                case KMemoryRegionType_VirtualDramSystemNonSecurePool: return KMemoryManager::Pool_SystemNonSecure;
                MESOSPHERE_UNREACHABLE_DEFAULT_CASE();
            }
        }

    }

    void KMemoryManager::Initialize(KVirtualAddress metadata_region, size_t metadata_region_size) {
        /* Clear the metadata region to zero. */
        const KVirtualAddress metadata_region_end = metadata_region + metadata_region_size;
        std::memset(GetVoidPointer(metadata_region), 0, metadata_region_size);

        /* Traverse the virtual memory layout tree, initializing each manager as appropriate. */
        while (true) {
            /* Locate the region that should initialize the current manager. */
            const KMemoryRegion *region = nullptr;
            for (const auto &it : KMemoryLayout::GetVirtualMemoryRegionTree()) {
                /* We only care about regions that we need to create managers for. */
                if (!it.IsDerivedFrom(KMemoryRegionType_VirtualDramManagedPool)) {
                    continue;
                }

                /* We want to initialize the managers in order. */
                if (it.GetAttributes() != this->num_managers) {
                    continue;
                }

                region = std::addressof(it);
                break;
            }

            /* If we didn't find a region, then we're done initializing managers. */
            if (region == nullptr) {
                break;
            }

            /* Ensure that the region is correct. */
            MESOSPHERE_ASSERT(region->GetAddress() != Null<decltype(region->GetAddress())>);
            MESOSPHERE_ASSERT(region->GetSize()    > 0);
            MESOSPHERE_ASSERT(region->GetEndAddress() >= region->GetAddress());
            MESOSPHERE_ASSERT(region->IsDerivedFrom(KMemoryRegionType_VirtualDramManagedPool));
            MESOSPHERE_ASSERT(region->GetAttributes() == this->num_managers);

            /* Initialize a new manager for the region. */
            const Pool pool = GetPoolFromMemoryRegionType(region->GetType());
            Impl *manager = std::addressof(this->managers[this->num_managers++]);
            MESOSPHERE_ABORT_UNLESS(this->num_managers <= util::size(this->managers));

            const size_t cur_size = manager->Initialize(region, pool, metadata_region, metadata_region_end);
            metadata_region += cur_size;
            MESOSPHERE_ABORT_UNLESS(metadata_region <= metadata_region_end);

            /* Insert the manager into the pool list. */
            if (this->pool_managers_tail[pool] == nullptr) {
                this->pool_managers_head[pool] = manager;
            } else {
                this->pool_managers_tail[pool]->SetNext(manager);
                manager->SetPrev(this->pool_managers_tail[pool]);
            }
            this->pool_managers_tail[pool] = manager;
        }
    }

    KVirtualAddress KMemoryManager::AllocateContinuous(size_t num_pages, size_t align_pages, u32 option) {
        /* Early return if we're allocating no pages. */
        if (num_pages == 0) {
            return Null<KVirtualAddress>;
        }

        /* Lock the pool that we're allocating from. */
        const auto [pool, dir] = DecodeOption(option);
        KScopedLightLock lk(this->pool_locks[pool]);

        /* Choose a heap based on our page size request. */
        const s32 heap_index = KPageHeap::GetAlignedBlockIndex(num_pages, align_pages);

        /* Loop, trying to iterate from each block. */
        Impl *chosen_manager = nullptr;
        KVirtualAddress allocated_block = Null<KVirtualAddress>;
        for (chosen_manager = this->GetFirstManager(pool, dir); chosen_manager != nullptr; chosen_manager = this->GetNextManager(chosen_manager, dir)) {
            allocated_block = chosen_manager->AllocateBlock(heap_index, true);
            if (allocated_block != Null<KVirtualAddress>) {
                break;
            }
        }

        /* If we failed to allocate, quit now. */
        if (allocated_block == Null<KVirtualAddress>) {
            return Null<KVirtualAddress>;
        }

        /* If we allocated more than we need, free some. */
        const size_t allocated_pages = KPageHeap::GetBlockNumPages(heap_index);
        if (allocated_pages > num_pages) {
            chosen_manager->Free(allocated_block + num_pages * PageSize, allocated_pages - num_pages);
        }

        /* Maintain the optimized memory bitmap, if we should. */
        if (this->has_optimized_process[pool]) {
            chosen_manager->TrackAllocationForOptimizedProcess(allocated_block, num_pages);
        }

        return allocated_block;
    }

    Result KMemoryManager::AllocatePageGroupImpl(KPageGroup *out, size_t num_pages, Pool pool, Direction dir, bool optimize, bool random) {
        /* Choose a heap based on our page size request. */
        const s32 heap_index = KPageHeap::GetBlockIndex(num_pages);
        R_UNLESS(0 <= heap_index, svc::ResultOutOfMemory());

        /* Ensure that we don't leave anything un-freed. */
        auto group_guard = SCOPE_GUARD {
            for (const auto &it : *out) {
                auto &manager = this->GetManager(it.GetAddress());
                const size_t num_pages = std::min(it.GetNumPages(), (manager.GetEndAddress() - it.GetAddress()) / PageSize);
                manager.Free(it.GetAddress(), num_pages);
            }
            out->Finalize();
        };

        /* Keep allocating until we've allocated all our pages. */
        for (s32 index = heap_index; index >= 0 && num_pages > 0; index--) {
            const size_t pages_per_alloc = KPageHeap::GetBlockNumPages(index);
            for (Impl *cur_manager = this->GetFirstManager(pool, dir); cur_manager != nullptr; cur_manager = this->GetNextManager(cur_manager, dir)) {
                while (num_pages >= pages_per_alloc) {
                    /* Allocate a block. */
                    KVirtualAddress allocated_block = cur_manager->AllocateBlock(index, random);
                    if (allocated_block == Null<KVirtualAddress>) {
                        break;
                    }

                    /* Safely add it to our group. */
                    {
                        auto block_guard = SCOPE_GUARD { cur_manager->Free(allocated_block, pages_per_alloc); };
                        R_TRY(out->AddBlock(allocated_block, pages_per_alloc));
                        block_guard.Cancel();
                    }

                    /* Maintain the optimized memory bitmap, if we should. */
                    if (optimize) {
                        cur_manager->TrackAllocationForOptimizedProcess(allocated_block, pages_per_alloc);
                    }

                    num_pages -= pages_per_alloc;
                }
            }
        }

        /* Only succeed if we allocated as many pages as we wanted. */
        MESOSPHERE_ASSERT(num_pages >= 0);
        R_UNLESS(num_pages == 0, svc::ResultOutOfMemory());

        /* We succeeded! */
        group_guard.Cancel();
        return ResultSuccess();
    }

    Result KMemoryManager::Allocate(KPageGroup *out, size_t num_pages, u32 option) {
        MESOSPHERE_ASSERT(out != nullptr);
        MESOSPHERE_ASSERT(out->GetNumPages() == 0);

        /* Early return if we're allocating no pages. */
        R_SUCCEED_IF(num_pages == 0);

        /* Lock the pool that we're allocating from. */
        const auto [pool, dir] = DecodeOption(option);
        KScopedLightLock lk(this->pool_locks[pool]);

        /* Allocate the page group. */
        return this->AllocatePageGroupImpl(out, num_pages, pool, dir, this->has_optimized_process[pool], true);
    }

    size_t KMemoryManager::Impl::Initialize(const KMemoryRegion *region, Pool p, KVirtualAddress metadata, KVirtualAddress metadata_end) {
        /* Calculate metadata sizes. */
        const size_t ref_count_size      = (region->GetSize() / PageSize) * sizeof(u16);
        const size_t optimize_map_size   = (util::AlignUp((region->GetSize() / PageSize), BITSIZEOF(u64)) / BITSIZEOF(u64)) * sizeof(u64);
        const size_t manager_size        = util::AlignUp(optimize_map_size + ref_count_size, PageSize);
        const size_t page_heap_size      = KPageHeap::CalculateMetadataOverheadSize(region->GetSize());
        const size_t total_metadata_size = manager_size + page_heap_size;
        MESOSPHERE_ABORT_UNLESS(manager_size <= total_metadata_size);
        MESOSPHERE_ABORT_UNLESS(metadata + total_metadata_size <= metadata_end);
        MESOSPHERE_ABORT_UNLESS(util::IsAligned(total_metadata_size, PageSize));

        /* Setup region. */
        this->pool = p;
        this->metadata_region = metadata;
        this->page_reference_counts = GetPointer<RefCount>(metadata + optimize_map_size);
        MESOSPHERE_ABORT_UNLESS(util::IsAligned(GetInteger(this->metadata_region), PageSize));

        /* Initialize the manager's KPageHeap. */
        this->heap.Initialize(region->GetAddress(), region->GetSize(), metadata + manager_size, page_heap_size);

        /* Free the memory to the heap. */
        this->heap.Free(region->GetAddress(), region->GetSize() / PageSize);

        /* Update the heap's used size. */
        this->heap.UpdateUsedSize();

        return total_metadata_size;
    }

    void KMemoryManager::Impl::TrackAllocationForOptimizedProcess(KVirtualAddress block, size_t num_pages) {
        size_t offset = this->heap.GetPageOffset(block);
        const size_t last = offset + num_pages - 1;
        u64 *optimize_map = GetPointer<u64>(this->metadata_region);
        while (offset <= last) {
            optimize_map[offset / BITSIZEOF(u64)] &= ~(u64(1) << (offset % BITSIZEOF(u64)));
            offset++;
        }
    }

    size_t KMemoryManager::Impl::CalculateMetadataOverheadSize(size_t region_size) {
        const size_t ref_count_size     = (region_size / PageSize) * sizeof(u16);
        const size_t optimize_map_size  = (util::AlignUp((region_size / PageSize), BITSIZEOF(u64)) / BITSIZEOF(u64)) * sizeof(u64);
        const size_t manager_meta_size  = util::AlignUp(optimize_map_size + ref_count_size, PageSize);
        const size_t page_heap_size     = KPageHeap::CalculateMetadataOverheadSize(region_size);
        return manager_meta_size + page_heap_size;
    }

}
