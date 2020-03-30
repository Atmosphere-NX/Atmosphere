/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include <stratosphere.hpp>
#include "lmem_impl_exp_heap.hpp"

namespace ams::lmem::impl {

    namespace {

        constexpr u16 FreeBlockMagic = 0x4652; /* FR */
        constexpr u16 UsedBlockMagic = 0x5544; /* UD */

        constexpr u16 DefaultGroupId = 0x00;
        constexpr u16 MaxGroupId     = 0xFF;

        constexpr size_t MinimumAlignment        = 4;
        constexpr size_t MaximumPaddingalignment = 0x80;

        constexpr AllocationMode DefaultAllocationMode = AllocationMode_FirstFit;

        constexpr size_t MinimumFreeBlockSize    = 4;

        struct MemoryRegion {
            void *start;
            void *end;
        };

        constexpr inline bool IsValidHeapHandle(HeapHandle handle) {
            return handle->magic == ExpHeapMagic;
        }

        constexpr inline ExpHeapHead *GetExpHeapHead(HeapHead *heap_head) {
            return &heap_head->impl_head.exp_heap_head;
        }

        constexpr inline const ExpHeapHead *GetExpHeapHead(const HeapHead *heap_head) {
            return &heap_head->impl_head.exp_heap_head;
        }

        constexpr inline HeapHead *GetHeapHead(ExpHeapHead *exp_heap_head) {
            return util::GetParentPointer<&HeapHead::impl_head>(util::GetParentPointer<&ImplementationHeapHead::exp_heap_head>(exp_heap_head));
        }

        constexpr inline const HeapHead *GetHeapHead(const ExpHeapHead *exp_heap_head) {
            return util::GetParentPointer<&HeapHead::impl_head>(util::GetParentPointer<&ImplementationHeapHead::exp_heap_head>(exp_heap_head));
        }

        constexpr inline void *GetExpHeapMemoryStart(ExpHeapHead *exp_heap_head) {
            return reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(exp_heap_head) + sizeof(ImplementationHeapHead));
        }

        constexpr inline void *GetMemoryBlockStart(ExpHeapMemoryBlockHead *head) {
            return reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(head) + sizeof(*head));
        }

        constexpr inline const void *GetMemoryBlockStart(const ExpHeapMemoryBlockHead *head) {
            return reinterpret_cast<const void *>(reinterpret_cast<uintptr_t>(head) + sizeof(*head));
        }

        constexpr inline void *GetMemoryBlockEnd(ExpHeapMemoryBlockHead *head) {
            return reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(GetMemoryBlockStart(head)) + head->block_size);
        }

        constexpr inline const void *GetMemoryBlockEnd(const ExpHeapMemoryBlockHead *head) {
            return reinterpret_cast<const void *>(reinterpret_cast<uintptr_t>(GetMemoryBlockStart(head)) + head->block_size);
        }

        constexpr inline ExpHeapMemoryBlockHead *GetHeadForMemoryBlock(const void *block) {
            return reinterpret_cast<ExpHeapMemoryBlockHead *>(reinterpret_cast<uintptr_t>(block) - sizeof(ExpHeapMemoryBlockHead));
        }

        constexpr inline bool IsValidUsedMemoryBlock(const HeapHead *heap, const void *block) {
            /* Block must fall within the heap range. */
            if (heap != nullptr) {
                if (block < heap->heap_start || heap->heap_end <= block) {
                    return false;
                }
            }

            /* Block magic must be used. */
            const ExpHeapMemoryBlockHead *head = GetHeadForMemoryBlock(block);
            if (head->magic != UsedBlockMagic) {
                return false;
            }

            /* End of block must remain within the heap range. */
            if (heap != nullptr) {
                if (reinterpret_cast<uintptr_t>(block) + head->block_size > reinterpret_cast<uintptr_t>(heap->heap_end)) {
                    return false;
                }
            }

            return true;
        }

        constexpr inline u16 GetMemoryBlockAlignmentPadding(const ExpHeapMemoryBlockHead *block_head) {
            return static_cast<u16>((block_head->attributes >> 8) & 0x7F);
        }

        inline void SetMemoryBlockAlignmentPadding(ExpHeapMemoryBlockHead *block_head, u16 padding) {
            block_head->attributes &= ~static_cast<decltype(block_head->attributes)>(0x7F << 8);
            block_head->attributes |= static_cast<decltype(block_head->attributes)>(padding & 0x7F) << 8;
        }

        constexpr inline u16 GetMemoryBlockGroupId(const ExpHeapMemoryBlockHead *block_head) {
            return static_cast<u16>(block_head->attributes & 0xFF);
        }

        inline void SetMemoryBlockGroupId(ExpHeapMemoryBlockHead *block_head, u16 group_id) {
            block_head->attributes &= ~static_cast<decltype(block_head->attributes)>(0xFF);
            block_head->attributes |= static_cast<decltype(block_head->attributes)>(group_id & 0xFF);
        }

        constexpr inline AllocationDirection GetMemoryBlockAllocationDirection(const ExpHeapMemoryBlockHead *block_head) {
            return static_cast<AllocationDirection>((block_head->attributes >> 15) & 1);
        }

        inline void SetMemoryBlockAllocationDirection(ExpHeapMemoryBlockHead *block_head, AllocationDirection dir) {
            block_head->attributes &= ~static_cast<decltype(block_head->attributes)>(0x8000);
            block_head->attributes |= static_cast<decltype(block_head->attributes)>(dir) << 15;
        }

        inline void GetMemoryBlockRegion(MemoryRegion *out, ExpHeapMemoryBlockHead *head)  {
            out->start = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(head) - GetMemoryBlockAlignmentPadding(head));
            out->end   = GetMemoryBlockEnd(head);
        }

        constexpr inline AllocationMode GetAllocationModeImpl(const ExpHeapHead *head) {
            return static_cast<AllocationMode>(head->mode);
        }

        inline void SetAllocationModeImpl(ExpHeapHead *head, AllocationMode mode) {
            head->mode = mode;
        }

        inline ExpHeapMemoryBlockHead *InitializeMemoryBlock(const MemoryRegion &region, u16 magic) {
            ExpHeapMemoryBlockHead *block = reinterpret_cast<ExpHeapMemoryBlockHead *>(region.start);

            /* Ensure all member constructors are called. */
            new (block) ExpHeapMemoryBlockHead;

            block->magic = magic;
            block->attributes = 0;
            block->block_size = GetPointerDifference(GetMemoryBlockStart(block), region.end);

            return block;
        }

        inline ExpHeapMemoryBlockHead *InitializeFreeMemoryBlock(const MemoryRegion &region) {
            return InitializeMemoryBlock(region, FreeBlockMagic);
        }

        inline ExpHeapMemoryBlockHead *InitializeUsedMemoryBlock(const MemoryRegion &region) {
            return InitializeMemoryBlock(region, UsedBlockMagic);
        }

        HeapHead *InitializeExpHeap(void *start, void *end, u32 option) {
            HeapHead *heap_head = reinterpret_cast<HeapHead *>(start);
            ExpHeapHead *exp_heap_head = GetExpHeapHead(heap_head);

            /* Initialize the parent heap. */
            InitializeHeapHead(heap_head, ExpHeapMagic, GetExpHeapMemoryStart(exp_heap_head), end, option);

            /* Call exp heap member constructors. */
            new (&exp_heap_head->free_list) ExpHeapMemoryBlockList;
            new (&exp_heap_head->used_list) ExpHeapMemoryBlockList;

            /* Set exp heap fields. */
            exp_heap_head->group_id = DefaultGroupId;
            exp_heap_head->use_alignment_margins = false;
            SetAllocationModeImpl(exp_heap_head, DefaultAllocationMode);

            /* Initialize memory block. */
            {
                MemoryRegion region{ .start = heap_head->heap_start, .end = heap_head->heap_end, };
                exp_heap_head->free_list.push_back(*InitializeFreeMemoryBlock(region));
            }

            return heap_head;
        }

        bool CoalesceFreedRegion(ExpHeapHead *head, const MemoryRegion *region) {
            auto prev_free_block_it = head->free_list.end();
            MemoryRegion free_region = *region;

            /* Locate the block. */
            for (auto it = head->free_list.begin(); it != head->free_list.end(); it++) {
                ExpHeapMemoryBlockHead *cur_free_block = &*it;

                if (cur_free_block < region->start) {
                    prev_free_block_it = it;
                    continue;
                }

                /* Coalesce block after, if possible. */
                if (cur_free_block == region->end) {
                    free_region.end = GetMemoryBlockEnd(cur_free_block);
                    it = head->free_list.erase(it);

                    /* Fill the memory with a pattern, for debug. */
                    FillUnallocatedMemory(GetHeapHead(head), cur_free_block, sizeof(ExpHeapMemoryBlockHead));
                }

                break;
            }

            /* We'll want to insert after the previous free block. */
            auto insertion_it = head->free_list.begin();
            if (prev_free_block_it != head->free_list.end()) {
                /* There's a previous free block, so we want to insert as the next iterator. */
                if (GetMemoryBlockEnd(&*prev_free_block_it) == region->start) {
                    /* We can coalesce, so do so. */
                    free_region.start = &*prev_free_block_it;
                    insertion_it = head->free_list.erase(prev_free_block_it);
                } else {
                    /* We can't coalesce, so just select the next iterator. */
                    insertion_it = (++prev_free_block_it);
                }
            }

            /* Ensure region is big enough for a block. */
            /* NOTE: Nintendo does not check against minimum block size here, only header size. */
            /* We will check against minimum block size, to avoid the creation of zero-size blocks. */
            if (GetPointerDifference(free_region.start, free_region.end) < sizeof(ExpHeapMemoryBlockHead) + MinimumFreeBlockSize) {
                return false;
            }

            /* Fill the memory with a pattern, for debug. */
            FillFreedMemory(GetHeapHead(head), free_region.start, GetPointerDifference(free_region.start, free_region.end));

            /* Insert the new memory block. */
            head->free_list.insert(insertion_it, *InitializeFreeMemoryBlock(free_region));

            return true;
        }

        void *ConvertFreeBlockToUsedBlock(ExpHeapHead *head, ExpHeapMemoryBlockHead *block_head, void *block, size_t size, AllocationDirection direction) {
            /* Calculate freed memory regions. */
            MemoryRegion free_region_front;
            GetMemoryBlockRegion(&free_region_front, block_head);
            MemoryRegion free_region_back{ .start = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(block) + size), .end = free_region_front.end, };

            /* Adjust end of head region. */
            free_region_front.end = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(block) - sizeof(ExpHeapMemoryBlockHead));

            /* Remove the old block. */
            auto old_block_it = head->free_list.erase(head->free_list.iterator_to(*block_head));

            /* If the front margins are big enough (and we're allowed to do so), make a new block. */
            if ((GetPointerDifference(free_region_front.start, free_region_front.end) < sizeof(ExpHeapMemoryBlockHead) + MinimumFreeBlockSize) ||
                (direction == AllocationDirection_Front && !head->use_alignment_margins && GetPointerDifference(free_region_front.start, free_region_front.end) < MaximumPaddingalignment)) {
                /* There isn't enough space for a new block, or else we're not allowed to make one. */
                free_region_front.end = free_region_front.start;
            } else {
                /* Make a new block! */
                head->free_list.insert(old_block_it, *InitializeFreeMemoryBlock(free_region_front));
            }

            /* If the back margins are big enough (and we're allowed to do so), make a new block. */
            if ((GetPointerDifference(free_region_back.start, free_region_back.end) < sizeof(ExpHeapMemoryBlockHead) + MinimumFreeBlockSize) ||
                (direction == AllocationDirection_Back && !head->use_alignment_margins && GetPointerDifference(free_region_back.start, free_region_back.end) < MaximumPaddingalignment)) {
                /* There isn't enough space for a new block, or else we're not allowed to make one. */
                free_region_back.end = free_region_back.start;
            } else {
                /* Make a new block! */
                head->free_list.insert(old_block_it, *InitializeFreeMemoryBlock(free_region_back));
            }

            /* Fill the memory with a pattern, for debug. */
            FillAllocatedMemory(GetHeapHead(head), free_region_front.end, GetPointerDifference(free_region_front.end, free_region_back.start));

            {
                /* Create the used block */
                MemoryRegion used_region{ .start = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(block) - sizeof(ExpHeapMemoryBlockHead)), .end = free_region_back.start };

                ExpHeapMemoryBlockHead *used_block = InitializeUsedMemoryBlock(used_region);

                /* Insert it into the used list. */
                head->used_list.push_back(*used_block);
                SetMemoryBlockAllocationDirection(used_block, direction);
                SetMemoryBlockAlignmentPadding(used_block, static_cast<u16>(GetPointerDifference(free_region_front.end, used_block)));
                SetMemoryBlockGroupId(used_block, head->group_id);
            }

            return block;
        }

        void *AllocateFromHead(HeapHead *heap, size_t size, s32 alignment) {
            ExpHeapHead *exp_heap_head = GetExpHeapHead(heap);

            const bool is_first_fit = GetAllocationModeImpl(exp_heap_head) == AllocationMode_FirstFit;

            /* Choose a block. */
            ExpHeapMemoryBlockHead *found_block_head = nullptr;
            void *found_block = nullptr;
            size_t best_size = std::numeric_limits<size_t>::max();

            for (auto it = exp_heap_head->free_list.begin(); it != exp_heap_head->free_list.end(); it++) {
                const uintptr_t absolute_block_start = reinterpret_cast<uintptr_t>(GetMemoryBlockStart(&*it));
                const uintptr_t block_start          = util::AlignUp(absolute_block_start, alignment);
                const size_t    block_offset         = block_start - absolute_block_start;

                if (it->block_size >= size + block_offset && best_size > it->block_size) {
                    found_block_head = &*it;
                    found_block      = reinterpret_cast<void *>(block_start);
                    best_size        = it->block_size;

                    if (is_first_fit || best_size == size) {
                        break;
                    }
                }
            }

            /* If we didn't find a block, return nullptr. */
            if (found_block_head == nullptr) {
                return nullptr;
            }

            return ConvertFreeBlockToUsedBlock(exp_heap_head, found_block_head, found_block, size, AllocationDirection_Front);
        }

        void *AllocateFromTail(HeapHead *heap, size_t size, s32 alignment) {
            ExpHeapHead *exp_heap_head = GetExpHeapHead(heap);

            const bool is_first_fit = GetAllocationModeImpl(exp_heap_head) == AllocationMode_FirstFit;

            /* Choose a block. */
            ExpHeapMemoryBlockHead *found_block_head = nullptr;
            void *found_block = nullptr;
            size_t best_size = std::numeric_limits<size_t>::max();

            for (auto it = exp_heap_head->free_list.rbegin(); it != exp_heap_head->free_list.rend(); it++) {
                const uintptr_t absolute_block_start = reinterpret_cast<uintptr_t>(GetMemoryBlockStart(&*it));
                const uintptr_t block_start          = util::AlignUp(absolute_block_start, alignment);
                const size_t    block_offset         = block_start - absolute_block_start;

                if (it->block_size >= size + block_offset && best_size > it->block_size) {
                    found_block_head = &*it;
                    found_block      = reinterpret_cast<void *>(block_start);
                    best_size        = it->block_size;

                    if (is_first_fit || best_size == size) {
                        break;
                    }
                }
            }

            /* If we didn't find a block, return nullptr. */
            if (found_block_head == nullptr) {
                return nullptr;
            }

            return ConvertFreeBlockToUsedBlock(exp_heap_head, found_block_head, found_block, size, AllocationDirection_Back);
        }

    }

    HeapHandle CreateExpHeap(void *address, size_t size, u32 option) {
        const uintptr_t uptr_end = util::AlignDown(reinterpret_cast<uintptr_t>(address) + size, MinimumAlignment);
        const uintptr_t uptr_start = util::AlignUp(reinterpret_cast<uintptr_t>(address), MinimumAlignment);

        if (uptr_start > uptr_end || GetPointerDifference(uptr_start, uptr_end) < sizeof(ExpHeapMemoryBlockHead) + MinimumFreeBlockSize) {
            return nullptr;
        }

        return InitializeExpHeap(reinterpret_cast<void *>(uptr_start), reinterpret_cast<void *>(uptr_end), option);
    }

    void DestroyExpHeap(HeapHandle handle) {
        AMS_ASSERT(IsValidHeapHandle(handle));

        FinalizeHeap(handle);
    }

    MemoryRange AdjustExpHeap(HeapHandle handle) {
        AMS_ASSERT(IsValidHeapHandle(handle));

        HeapHead *heap_head = handle;
        ExpHeapHead *exp_heap_head = GetExpHeapHead(heap_head);

        /* If there's no free blocks, we can't do anything. */
        if (exp_heap_head->free_list.empty()) {
            return MakeMemoryRange(handle->heap_end, 0);
        }

        /* Get the memory block end, make sure it really is the last block. */
        ExpHeapMemoryBlockHead *block = &exp_heap_head->free_list.back();
        void * const block_start = GetMemoryBlockStart(block);
        const size_t block_size = block->block_size;
        void * const block_end   = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(block_start) + block_size);

        if (block_end != handle->heap_end) {
            return MakeMemoryRange(handle->heap_end, 0);
        }

        /* Remove the memory block. */
        exp_heap_head->free_list.pop_back();

        const size_t freed_size = block_size + sizeof(ExpHeapMemoryBlockHead);
        heap_head->heap_end = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(heap_head->heap_end) - freed_size);
        return MakeMemoryRange(heap_head->heap_end, freed_size);
    }

    void *AllocateFromExpHeap(HeapHandle handle, size_t size, s32 alignment) {
        AMS_ASSERT(IsValidHeapHandle(handle));

        /* Fix up alignments less than 4. */
        if (alignment == 1 || alignment == 2) {
            alignment = 4;
        } else if (alignment == -1 || alignment == -2) {
            alignment = -4;
        }

        /* Ensure the alignment is valid. */
        const s32 abs_alignment = std::abs(alignment);
        AMS_ASSERT((abs_alignment & (abs_alignment - 1)) == 0);
        AMS_ASSERT(MinimumAlignment <= static_cast<size_t>(abs_alignment));

        /* Fix size to be correctly aligned. */
        if (size == 0) {
            size = 1;
        }
        size = util::AlignUp(size, MinimumAlignment);

        /* Allocate a memory block. */
        void *allocated_memory = nullptr;
        if (alignment >= 0) {
            allocated_memory = AllocateFromHead(handle, size, alignment);
        } else {
            allocated_memory = AllocateFromTail(handle, size, -alignment);
        }

        return allocated_memory;
    }

    void FreeToExpHeap(HeapHandle handle, void *mem_block) {
        /* Ensure this is actually a valid heap and a valid memory block we allocated. */
        AMS_ASSERT(IsValidHeapHandle(handle));
        AMS_ASSERT(IsValidUsedMemoryBlock(handle, mem_block));

        /* TODO: Nintendo does not allow FreeToExpHeap(nullptr). Should we? */

        /* Get block pointers. */
        HeapHead *heap_head = handle;
        ExpHeapHead *exp_heap_head = GetExpHeapHead(heap_head);
        ExpHeapMemoryBlockHead *block = GetHeadForMemoryBlock(mem_block);
        MemoryRegion region;

        /* Erase the heap from the used list, and coalesce it with adjacent blocks. */
        GetMemoryBlockRegion(&region, block);
        exp_heap_head->used_list.erase(exp_heap_head->used_list.iterator_to(*block));
        AMS_ASSERT(CoalesceFreedRegion(exp_heap_head, &region));
    }

    size_t ResizeExpHeapMemoryBlock(HeapHandle handle, void *mem_block, size_t size) {
        /* Ensure this is actually a valid heap and a valid memory block we allocated. */
        AMS_ASSERT(IsValidHeapHandle(handle));
        AMS_ASSERT(IsValidUsedMemoryBlock(handle, mem_block));

        ExpHeapHead *exp_heap_head = GetExpHeapHead(handle);
        ExpHeapMemoryBlockHead *block_head = GetHeadForMemoryBlock(mem_block);
        const size_t original_block_size = block_head->block_size;

        /* It's possible that there's no actual resizing being done. */
        size = util::AlignUp(size, MinimumAlignment);
        if (size == original_block_size) {
            return size;
        }

        /* We're resizing one way or the other. */
        if (size > original_block_size) {
            /* We want to try to make the block bigger. */

            /* Find the free block after this one. */
            void * const cur_block_end = GetMemoryBlockEnd(block_head);
            ExpHeapMemoryBlockHead *next_block_head = nullptr;

            for (auto it = exp_heap_head->free_list.begin(); it != exp_heap_head->free_list.end(); it++) {
                if (&*it == cur_block_end) {
                    next_block_head = &*it;
                    break;
                }
            }

            /* If we can't get a big enough allocation using the next block, give up. */
            if (next_block_head == nullptr || size > original_block_size + sizeof(ExpHeapMemoryBlockHead) + next_block_head->block_size) {
                return 0;
            }

            /* Grow the block to encompass the next block. */
            {
                /* Get block region. */
                MemoryRegion new_free_region;
                GetMemoryBlockRegion(&new_free_region, next_block_head);

                /* Remove the next block from the free list. */
                auto insertion_it = exp_heap_head->free_list.erase(exp_heap_head->free_list.iterator_to(*next_block_head));

                /* Figure out the new block extents. */
                void *old_start = new_free_region.start;
                new_free_region.start = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(mem_block) + size);

                /* Only maintain the new free region as a memory block candidate if it can hold a header. */
                /* NOTE: Nintendo does not check against minimum block size here, only header size. */
                /* We will check against minimum block size, to avoid the creation of zero-size blocks. */
                if (GetPointerDifference(new_free_region.start, new_free_region.end) < sizeof(ExpHeapMemoryBlockHead) + MinimumFreeBlockSize) {
                    new_free_region.start = new_free_region.end;
                }

                /* Adjust block sizes. */
                block_head->block_size = GetPointerDifference(mem_block, new_free_region.start);
                if (GetPointerDifference(new_free_region.start, new_free_region.end) >= sizeof(ExpHeapMemoryBlockHead) + MinimumFreeBlockSize) {
                    exp_heap_head->free_list.insert(insertion_it, *InitializeFreeMemoryBlock(new_free_region));
                }

                /* Fill the memory with a pattern, for debug. */
                FillAllocatedMemory(GetHeapHead(exp_heap_head), old_start, GetPointerDifference(old_start, new_free_region.start));
            }
        } else {
            /* We're shrinking the block. Nice and easy. */
            MemoryRegion new_free_region{ .start = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(mem_block)+ size), .end = GetMemoryBlockEnd(block_head) };

            /* Try to free the new memory. */
            block_head->block_size = size;
            if (!CoalesceFreedRegion(exp_heap_head, &new_free_region)) {
                /* We didn't shrink the block successfully, so restore the size. */
                block_head->block_size = original_block_size;
            }
        }

        return block_head->block_size;
    }

    size_t GetExpHeapTotalFreeSize(HeapHandle handle) {
        AMS_ASSERT(IsValidHeapHandle(handle));

        size_t total_size = 0;
        for (const auto &it : GetExpHeapHead(handle)->free_list) {
            total_size += it.block_size;
        }
        return total_size;
    }

    size_t GetExpHeapAllocatableSize(HeapHandle handle, s32 alignment) {
        AMS_ASSERT(IsValidHeapHandle(handle));

        /* Ensure alignment is positive. */
        alignment = std::abs(alignment);

        size_t max_size   = std::numeric_limits<size_t>::min();
        size_t min_offset = std::numeric_limits<size_t>::max();
        for (const auto &it : GetExpHeapHead(handle)->free_list) {
            const uintptr_t absolute_block_start = reinterpret_cast<uintptr_t>(GetMemoryBlockStart(&it));
            const uintptr_t block_start          = util::AlignUp(absolute_block_start, alignment);
            const uintptr_t block_end            = reinterpret_cast<uintptr_t>(GetMemoryBlockEnd(&it));

            if (block_start < block_end) {
                const size_t block_size = GetPointerDifference(block_start, block_end);
                const size_t offset     = GetPointerDifference(absolute_block_start, block_start);

                if (block_size > max_size || (block_size == max_size && offset < min_offset)) {
                    max_size = block_size;
                    min_offset = offset;
                }
            }
        }

        return max_size;
    }

    AllocationMode GetExpHeapAllocationMode(HeapHandle handle) {
        AMS_ASSERT(IsValidHeapHandle(handle));

        return GetAllocationModeImpl(GetExpHeapHead(handle));
    }

    AllocationMode SetExpHeapAllocationMode(HeapHandle handle, AllocationMode new_mode) {
        AMS_ASSERT(IsValidHeapHandle(handle));

        ExpHeapHead *exp_heap_head = GetExpHeapHead(handle);
        const AllocationMode old_mode = GetAllocationModeImpl(exp_heap_head);
        SetAllocationModeImpl(exp_heap_head, new_mode);
        return old_mode;
    }

    u16 GetExpHeapGroupId(HeapHandle handle) {
        AMS_ASSERT(IsValidHeapHandle(handle));

        return GetExpHeapHead(handle)->group_id;
    }

    u16 SetExpHeapGroupId(HeapHandle handle, u16 group_id) {
        AMS_ASSERT(IsValidHeapHandle(handle));
        AMS_ASSERT(group_id <= MaxGroupId);

        ExpHeapHead *exp_heap_head = GetExpHeapHead(handle);
        const u16 old_group_id = exp_heap_head->group_id;
        exp_heap_head->group_id = group_id;
        return old_group_id;
    }

    void VisitExpHeapAllocatedBlocks(HeapHandle handle, HeapVisitor visitor, uintptr_t user_data) {
        AMS_ASSERT(IsValidHeapHandle(handle));

        for (auto &it : GetExpHeapHead(handle)->used_list) {
            (*visitor)(GetMemoryBlockStart(&it), handle, user_data);
        }
    }

    size_t GetExpHeapMemoryBlockSize(const void *memory_block) {
        AMS_ASSERT(IsValidUsedMemoryBlock(nullptr, memory_block));

        return GetHeadForMemoryBlock(memory_block)->block_size;
    }

    u16 GetExpHeapMemoryBlockGroupId(const void *memory_block) {
        AMS_ASSERT(IsValidUsedMemoryBlock(nullptr, memory_block));

        return GetMemoryBlockGroupId(GetHeadForMemoryBlock(memory_block));
    }

    AllocationDirection GetExpHeapMemoryBlockAllocationDirection(const void *memory_block) {
        AMS_ASSERT(IsValidUsedMemoryBlock(nullptr, memory_block));

        return GetMemoryBlockAllocationDirection(GetHeadForMemoryBlock(memory_block));
    }

}
