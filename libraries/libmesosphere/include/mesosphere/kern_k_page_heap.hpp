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
#pragma once
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_k_page_bitmap.hpp>

namespace ams::kern {

    class KPageHeap {
        private:
            static constexpr inline size_t MemoryBlockPageShifts[] = { 0xC, 0x10, 0x15, 0x16, 0x19, 0x1D, 0x1E };
            static constexpr size_t NumMemoryBlockPageShifts = util::size(MemoryBlockPageShifts);
        public:
            static constexpr s32 GetAlignedBlockIndex(size_t num_pages, size_t align_pages) {
                const size_t target_pages = std::max(num_pages, align_pages);
                for (size_t i = 0; i < NumMemoryBlockPageShifts; i++) {
                    if (target_pages <= (size_t(1) << MemoryBlockPageShifts[i]) / PageSize) {
                        return static_cast<s32>(i);
                    }
                }
                return -1;
            }

            static constexpr s32 GetBlockIndex(size_t num_pages) {
                for (s32 i = static_cast<s32>(NumMemoryBlockPageShifts) - 1; i >= 0; i--) {
                    if (num_pages >= (size_t(1) << MemoryBlockPageShifts[i]) / PageSize) {
                        return i;
                    }
                }
                return -1;
            }

            static constexpr size_t GetBlockSize(size_t index) {
                return size_t(1) << MemoryBlockPageShifts[index];
            }

            static constexpr size_t GetBlockNumPages(size_t index) {
                return GetBlockSize(index) / PageSize;
            }
        private:
            class Block {
                private:
                    KPageBitmap bitmap;
                    KVirtualAddress heap_address;
                    uintptr_t end_offset;
                    size_t block_shift;
                    size_t next_block_shift;
                public:
                    Block() : bitmap(), heap_address(), end_offset(), block_shift(), next_block_shift() { /* ... */ }

                    constexpr size_t GetShift() const { return this->block_shift; }
                    constexpr size_t GetNextShift() const { return this->next_block_shift; }
                    constexpr size_t GetSize() const { return u64(1) << this->GetShift(); }
                    constexpr size_t GetNumPages() const { return this->GetSize() / PageSize; }
                    constexpr size_t GetNumFreeBlocks() const { return this->bitmap.GetNumBits(); }
                    constexpr size_t GetNumFreePages() const { return this->GetNumFreeBlocks() * this->GetNumPages(); }

                    u64 *Initialize(KVirtualAddress addr, size_t size, size_t bs, size_t nbs, u64 *bit_storage) {
                        /* Set shifts. */
                        this->block_shift = bs;
                        this->next_block_shift = nbs;

                        /* Align up the address. */
                        KVirtualAddress end = addr + size;
                        const size_t align = (this->next_block_shift != 0) ? (u64(1) << this->next_block_shift) : (u64(1) << this->block_shift);
                        addr = util::AlignDown(GetInteger(addr), align);
                        end  = util::AlignUp(GetInteger(end), align);

                        this->heap_address = addr;
                        this->end_offset   = (end - addr) / (u64(1) << this->block_shift);
                        return this->bitmap.Initialize(bit_storage, this->end_offset);
                    }

                    KVirtualAddress PushBlock(KVirtualAddress address) {
                        /* Set the bit for the free block. */
                        size_t offset = (address - this->heap_address) >> this->GetShift();
                        this->bitmap.SetBit(offset);

                        /* If we have a next shift, try to clear the blocks below this one and return the new address. */
                        if (this->GetNextShift()) {
                            const size_t diff = u64(1) << (this->GetNextShift() - this->GetShift());
                            offset = util::AlignDown(offset, diff);
                            if (this->bitmap.ClearRange(offset, diff)) {
                                return this->heap_address + (offset << this->GetShift());
                            }
                        }

                        /* We couldn't coalesce, or we're already as big as possible. */
                        return Null<KVirtualAddress>;
                    }

                    KVirtualAddress PopBlock(bool random) {
                        /* Find a free block. */
                        ssize_t soffset = this->bitmap.FindFreeBlock(random);
                        if (soffset < 0) {
                            return Null<KVirtualAddress>;
                        }
                        const size_t offset = static_cast<size_t>(soffset);

                        /* Update our tracking and return it. */
                        this->bitmap.ClearBit(offset);
                        return this->heap_address + (offset << this->GetShift());
                    }
                public:
                    static constexpr size_t CalculateManagementOverheadSize(size_t region_size, size_t cur_block_shift, size_t next_block_shift) {
                        const size_t cur_block_size  = (u64(1) << cur_block_shift);
                        const size_t next_block_size = (u64(1) << next_block_shift);
                        const size_t align = (next_block_shift != 0) ? next_block_size : cur_block_size;
                        return KPageBitmap::CalculateManagementOverheadSize((align * 2 + util::AlignUp(region_size, align)) / cur_block_size);
                    }
            };
        private:
            KVirtualAddress heap_address;
            size_t heap_size;
            size_t used_size;
            size_t num_blocks;
            Block blocks[NumMemoryBlockPageShifts];
        private:
            void Initialize(KVirtualAddress heap_address, size_t heap_size, KVirtualAddress management_address, size_t management_size, const size_t *block_shifts, size_t num_block_shifts);
            size_t GetNumFreePages() const;

            void FreeBlock(KVirtualAddress block, s32 index);
        public:
            KPageHeap() : heap_address(), heap_size(), used_size(), num_blocks(), blocks() { /* ... */ }

            constexpr KVirtualAddress GetAddress() const { return this->heap_address; }
            constexpr size_t GetSize() const { return this->heap_size; }
            constexpr KVirtualAddress GetEndAddress() const { return this->GetAddress() + this->GetSize(); }
            constexpr size_t GetPageOffset(KVirtualAddress block) const { return (block - this->GetAddress()) / PageSize; }
            constexpr size_t GetPageOffsetToEnd(KVirtualAddress block) const { return (this->GetEndAddress() - block) / PageSize; }

            void Initialize(KVirtualAddress heap_address, size_t heap_size, KVirtualAddress management_address, size_t management_size) {
                return Initialize(heap_address, heap_size, management_address, management_size, MemoryBlockPageShifts, NumMemoryBlockPageShifts);
            }

            size_t GetFreeSize() const { return this->GetNumFreePages() * PageSize; }
            void DumpFreeList() const;

            void UpdateUsedSize() {
                this->used_size = this->heap_size - (this->GetNumFreePages() * PageSize);
            }

            KVirtualAddress AllocateBlock(s32 index, bool random);
            void Free(KVirtualAddress addr, size_t num_pages);
        private:
            static size_t CalculateManagementOverheadSize(size_t region_size, const size_t *block_shifts, size_t num_block_shifts);
        public:
            static size_t CalculateManagementOverheadSize(size_t region_size) {
                return CalculateManagementOverheadSize(region_size, MemoryBlockPageShifts, NumMemoryBlockPageShifts);
            }
    };

}
