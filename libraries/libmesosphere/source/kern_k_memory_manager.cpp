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
            if ((type | KMemoryRegionType_VirtualDramApplicationPool) == type) {
                return KMemoryManager::Pool_Application;
            } else if ((type | KMemoryRegionType_VirtualDramAppletPool) == type) {
                return KMemoryManager::Pool_Applet;
            } else if ((type | KMemoryRegionType_VirtualDramSystemPool) == type) {
                return KMemoryManager::Pool_System;
            } else if ((type | KMemoryRegionType_VirtualDramSystemNonSecurePool) == type) {
                return KMemoryManager::Pool_SystemNonSecure;
            } else {
                MESOSPHERE_PANIC("InvalidMemoryRegionType for conversion to Pool");
            }
        }

    }

    void KMemoryManager::Initialize(KVirtualAddress management_region, size_t management_region_size) {
        /* Clear the management region to zero. */
        const KVirtualAddress management_region_end = management_region + management_region_size;
        std::memset(GetVoidPointer(management_region), 0, management_region_size);

        /* Traverse the virtual memory layout tree, initializing each manager as appropriate. */
        while (this->num_managers != MaxManagerCount) {
            /* Locate the region that should initialize the current manager. */
            uintptr_t region_address = 0;
            size_t region_size = 0;
            Pool region_pool = Pool_Count;
            for (const auto &it : KMemoryLayout::GetVirtualMemoryRegionTree()) {
                /* We only care about regions that we need to create managers for. */
                if (!it.IsDerivedFrom(KMemoryRegionType_VirtualDramUserPool)) {
                    continue;
                }

                /* We want to initialize the managers in order. */
                if (it.GetAttributes() != this->num_managers) {
                    continue;
                }

                /* Validate the region. */
                MESOSPHERE_ABORT_UNLESS(it.GetEndAddress() != 0);
                MESOSPHERE_ASSERT(it.GetAddress() != Null<decltype(it.GetAddress())>);
                MESOSPHERE_ASSERT(it.GetSize()    > 0);

                /* Update the region's extents. */
                if (region_address == 0) {
                    region_address = it.GetAddress();
                    region_size    = it.GetSize();
                    region_pool    = GetPoolFromMemoryRegionType(it.GetType());
                } else {
                    MESOSPHERE_ASSERT(it.GetAddress() == region_address + region_size);

                    /* Update the size. */
                    region_size = it.GetEndAddress() - region_address;
                    MESOSPHERE_ABORT_UNLESS(GetPoolFromMemoryRegionType(it.GetType()) == region_pool);
                }
            }

            /* If we didn't find a region, we're done. */
            if (region_size == 0) {
                break;
            }

            /* Initialize a new manager for the region. */
            Impl *manager = std::addressof(this->managers[this->num_managers++]);
            MESOSPHERE_ABORT_UNLESS(this->num_managers <= util::size(this->managers));

            const size_t cur_size = manager->Initialize(region_address, region_size, management_region, management_region_end, region_pool);
            management_region += cur_size;
            MESOSPHERE_ABORT_UNLESS(management_region <= management_region_end);

            /* Insert the manager into the pool list. */
            if (this->pool_managers_tail[region_pool] == nullptr) {
                this->pool_managers_head[region_pool] = manager;
            } else {
                this->pool_managers_tail[region_pool]->SetNext(manager);
                manager->SetPrev(this->pool_managers_tail[region_pool]);
            }
            this->pool_managers_tail[region_pool] = manager;
        }

        /* Free each region to its corresponding heap. */
        for (const auto &it : KMemoryLayout::GetVirtualMemoryRegionTree()) {
            if (it.IsDerivedFrom(KMemoryRegionType_VirtualDramUserPool)) {
                /* Check the region. */
                MESOSPHERE_ABORT_UNLESS(it.GetEndAddress() != 0);

                /* Free the memory to the heap. */
                this->managers[it.GetAttributes()].Free(it.GetAddress(), it.GetSize() / PageSize);
            }
        }

        /* Update the used size for all managers. */
        for (size_t i = 0; i < this->num_managers; ++i) {
            this->managers[i].UpdateUsedHeapSize();
        }
    }

    Result KMemoryManager::InitializeOptimizedMemory(u64 process_id, Pool pool) {
        /* Lock the pool. */
        KScopedLightLock lk(this->pool_locks[pool]);

        /* Check that we don't already have an optimized process. */
        R_UNLESS(!this->has_optimized_process[pool], svc::ResultBusy());

        /* Set the optimized process id. */
        this->optimized_process_ids[pool] = process_id;
        this->has_optimized_process[pool] = true;

        /* Clear the management area for the optimized process. */
        for (auto *manager = this->GetFirstManager(pool, Direction_FromFront); manager != nullptr; manager = this->GetNextManager(manager, Direction_FromFront)) {
            manager->InitializeOptimizedMemory();
        }

        return ResultSuccess();
    }

    void KMemoryManager::FinalizeOptimizedMemory(u64 process_id, Pool pool) {
        /* Lock the pool. */
        KScopedLightLock lk(this->pool_locks[pool]);

        /* If the process was optimized, clear it. */
        if (this->has_optimized_process[pool] && this->optimized_process_ids[pool] == process_id) {
            this->has_optimized_process[pool] = false;
        }
    }


    KVirtualAddress KMemoryManager::AllocateAndOpenContinuous(size_t num_pages, size_t align_pages, u32 option) {
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
            chosen_manager->TrackUnoptimizedAllocation(allocated_block, num_pages);
        }

        /* Open the first reference to the pages. */
        chosen_manager->OpenFirst(allocated_block, num_pages);

        return allocated_block;
    }

    Result KMemoryManager::AllocatePageGroupImpl(KPageGroup *out, size_t num_pages, Pool pool, Direction dir, bool unoptimized, bool random) {
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
                    if (unoptimized) {
                        cur_manager->TrackUnoptimizedAllocation(allocated_block, pages_per_alloc);
                    }

                    num_pages -= pages_per_alloc;
                }
            }
        }

        /* Only succeed if we allocated as many pages as we wanted. */
        R_UNLESS(num_pages == 0, svc::ResultOutOfMemory());

        /* We succeeded! */
        group_guard.Cancel();
        return ResultSuccess();
    }

    Result KMemoryManager::AllocateAndOpen(KPageGroup *out, size_t num_pages, u32 option) {
        MESOSPHERE_ASSERT(out != nullptr);
        MESOSPHERE_ASSERT(out->GetNumPages() == 0);

        /* Early return if we're allocating no pages. */
        R_SUCCEED_IF(num_pages == 0);

        /* Lock the pool that we're allocating from. */
        const auto [pool, dir] = DecodeOption(option);
        KScopedLightLock lk(this->pool_locks[pool]);

        /* Allocate the page group. */
        R_TRY(this->AllocatePageGroupImpl(out, num_pages, pool, dir, this->has_optimized_process[pool], true));

        /* Open the first reference to the pages. */
        for (const auto &block : *out) {
            KVirtualAddress cur_address = block.GetAddress();
            size_t remaining_pages      = block.GetNumPages();
            while (remaining_pages > 0) {
                /* Get the manager for the current address. */
                auto &manager = this->GetManager(cur_address);

                /* Process part or all of the block. */
                const size_t cur_pages = std::min(remaining_pages, manager.GetPageOffsetToEnd(cur_address));
                manager.OpenFirst(cur_address, cur_pages);

                /* Advance. */
                cur_address     += cur_pages * PageSize;
                remaining_pages -= cur_pages;
            }
        }

        return ResultSuccess();
    }

    Result KMemoryManager::AllocateAndOpenForProcess(KPageGroup *out, size_t num_pages, u32 option, u64 process_id, u8 fill_pattern) {
        MESOSPHERE_ASSERT(out != nullptr);
        MESOSPHERE_ASSERT(out->GetNumPages() == 0);

        /* Decode the option. */
        const auto [pool, dir] = DecodeOption(option);

        /* Allocate the memory. */
        bool optimized;
        {
            /* Lock the pool that we're allocating from. */
            KScopedLightLock lk(this->pool_locks[pool]);

            /* Check if we have an optimized process. */
            const bool has_optimized = this->has_optimized_process[pool];
            const bool is_optimized  = this->optimized_process_ids[pool] == process_id;

            /* Allocate the page group. */
            R_TRY(this->AllocatePageGroupImpl(out, num_pages, pool, dir, has_optimized && !is_optimized, false));

            /* Set whether we should optimize. */
            optimized = has_optimized && is_optimized;

            /* Open the first reference to the pages. */
            for (const auto &block : *out) {
                KVirtualAddress cur_address = block.GetAddress();
                size_t remaining_pages      = block.GetNumPages();
                while (remaining_pages > 0) {
                    /* Get the manager for the current address. */
                    auto &manager = this->GetManager(cur_address);

                    /* Process part or all of the block. */
                    const size_t cur_pages = std::min(remaining_pages, manager.GetPageOffsetToEnd(cur_address));
                    manager.OpenFirst(cur_address, cur_pages);

                    /* Advance. */
                    cur_address     += cur_pages * PageSize;
                    remaining_pages -= cur_pages;
                }
            }
        }

        /* Perform optimized memory tracking, if we should. */
        if (optimized) {
            /* Iterate over the allocated blocks. */
            for (const auto &block : *out) {
                /* Get the block extents. */
                const KVirtualAddress block_address = block.GetAddress();
                const size_t block_pages            = block.GetNumPages();

                /* If it has no pages, we don't need to do anything. */
                if (block_pages == 0) {
                    continue;
                }

                /* Fill all the pages that we need to fill. */
                bool any_new = false;
                {
                    KVirtualAddress cur_address = block_address;
                    size_t remaining_pages      = block_pages;
                    while (remaining_pages > 0) {
                        /* Get the manager for the current address. */
                        auto &manager = this->GetManager(cur_address);

                        /* Process part or all of the block. */
                        const size_t cur_pages = std::min(remaining_pages, manager.GetPageOffsetToEnd(cur_address));
                        any_new = manager.ProcessOptimizedAllocation(cur_address, cur_pages, fill_pattern);

                        /* Advance. */
                        cur_address     += cur_pages * PageSize;
                        remaining_pages -= cur_pages;
                    }
                }

                /* If there are new pages, update tracking for the allocation. */
                if (any_new) {
                    /* Update tracking for the allocation. */
                    KVirtualAddress cur_address = block_address;
                    size_t remaining_pages      = block_pages;
                    while (remaining_pages > 0) {
                        /* Get the manager for the current address. */
                        auto &manager = this->GetManager(cur_address);

                        /* Lock the pool for the manager. */
                        KScopedLightLock lk(this->pool_locks[manager.GetPool()]);

                        /* Track some or all of the current pages. */
                        const size_t cur_pages = std::min(remaining_pages, manager.GetPageOffsetToEnd(cur_address));
                        manager.TrackOptimizedAllocation(cur_address, cur_pages);

                        /* Advance. */
                        cur_address     += cur_pages * PageSize;
                        remaining_pages -= cur_pages;
                    }
                }
            }
        } else {
            /* Set all the allocated memory. */
            for (const auto &block : *out) {
                std::memset(GetVoidPointer(block.GetAddress()), fill_pattern, block.GetSize());
            }
        }

        return ResultSuccess();
    }

    size_t KMemoryManager::Impl::Initialize(uintptr_t address, size_t size, KVirtualAddress management, KVirtualAddress management_end, Pool p) {
        /* Calculate management sizes. */
        const size_t ref_count_size      = (size / PageSize) * sizeof(u16);
        const size_t optimize_map_size   = CalculateOptimizedProcessOverheadSize(size);
        const size_t manager_size        = util::AlignUp(optimize_map_size + ref_count_size, PageSize);
        const size_t page_heap_size      = KPageHeap::CalculateManagementOverheadSize(size);
        const size_t total_management_size = manager_size + page_heap_size;
        MESOSPHERE_ABORT_UNLESS(manager_size <= total_management_size);
        MESOSPHERE_ABORT_UNLESS(management + total_management_size <= management_end);
        MESOSPHERE_ABORT_UNLESS(util::IsAligned(total_management_size, PageSize));

        /* Setup region. */
        this->pool = p;
        this->management_region = management;
        this->page_reference_counts = GetPointer<RefCount>(management + optimize_map_size);
        MESOSPHERE_ABORT_UNLESS(util::IsAligned(GetInteger(this->management_region), PageSize));

        /* Initialize the manager's KPageHeap. */
        this->heap.Initialize(address, size, management + manager_size, page_heap_size);

        return total_management_size;
    }

    void KMemoryManager::Impl::TrackUnoptimizedAllocation(KVirtualAddress block, size_t num_pages) {
        /* Get the range we're tracking. */
        size_t offset = this->GetPageOffset(block);
        const size_t last = offset + num_pages - 1;

        /* Track. */
        u64 *optimize_map = GetPointer<u64>(this->management_region);
        while (offset <= last) {
            /* Mark the page as not being optimized-allocated. */
            optimize_map[offset / BITSIZEOF(u64)] &= ~(u64(1) << (offset % BITSIZEOF(u64)));

            offset++;
        }
    }

    void KMemoryManager::Impl::TrackOptimizedAllocation(KVirtualAddress block, size_t num_pages) {
        /* Get the range we're tracking. */
        size_t offset = this->GetPageOffset(block);
        const size_t last = offset + num_pages - 1;

        /* Track. */
        u64 *optimize_map = GetPointer<u64>(this->management_region);
        while (offset <= last) {
            /* Mark the page as being optimized-allocated. */
            optimize_map[offset / BITSIZEOF(u64)] |= (u64(1) << (offset % BITSIZEOF(u64)));

            offset++;
        }
    }

    bool KMemoryManager::Impl::ProcessOptimizedAllocation(KVirtualAddress block, size_t num_pages, u8 fill_pattern) {
        /* We want to return whether any pages were newly allocated. */
        bool any_new = false;

        /* Get the range we're processing. */
        size_t offset = this->GetPageOffset(block);
        const size_t last = offset + num_pages - 1;

        /* Process. */
        u64 *optimize_map = GetPointer<u64>(this->management_region);
        while (offset <= last) {
            /* Check if the page has been optimized-allocated before. */
            if ((optimize_map[offset / BITSIZEOF(u64)] & (u64(1) << (offset % BITSIZEOF(u64)))) == 0) {
                /* If not, it's new. */
                any_new = true;

                /* Fill the page. */
                std::memset(GetVoidPointer(this->heap.GetAddress() + offset * PageSize), fill_pattern, PageSize);
            }

            offset++;
        }

        /* Return the number of pages we processed. */
        return any_new;
    }

    size_t KMemoryManager::Impl::CalculateManagementOverheadSize(size_t region_size) {
        const size_t ref_count_size     = (region_size / PageSize) * sizeof(u16);
        const size_t optimize_map_size  = (util::AlignUp((region_size / PageSize), BITSIZEOF(u64)) / BITSIZEOF(u64)) * sizeof(u64);
        const size_t manager_meta_size  = util::AlignUp(optimize_map_size + ref_count_size, PageSize);
        const size_t page_heap_size     = KPageHeap::CalculateManagementOverheadSize(region_size);
        return manager_meta_size + page_heap_size;
    }

}
