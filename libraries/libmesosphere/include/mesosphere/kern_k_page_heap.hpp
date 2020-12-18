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
                    KPageBitmap m_bitmap;
                    KVirtualAddress m_heap_address;
                    uintptr_t m_end_offset;
                    size_t m_block_shift;
                    size_t m_next_block_shift;
                public:
                    Block() : m_bitmap(), m_heap_address(), m_end_offset(), m_block_shift(), m_next_block_shift() { /* ... */ }

                    constexpr size_t GetShift() const { return m_block_shift; }
                    constexpr size_t GetNextShift() const { return m_next_block_shift; }
                    constexpr size_t GetSize() const { return u64(1) << this->GetShift(); }
                    constexpr size_t GetNumPages() const { return this->GetSize() / PageSize; }
                    constexpr size_t GetNumFreeBlocks() const { return m_bitmap.GetNumBits(); }
                    constexpr size_t GetNumFreePages() const { return this->GetNumFreeBlocks() * this->GetNumPages(); }

                    u64 *Initialize(KVirtualAddress addr, size_t size, size_t bs, size_t nbs, u64 *bit_storage) {
                        /* Set shifts. */
                        m_block_shift = bs;
                        m_next_block_shift = nbs;

                        /* Align up the address. */
                        KVirtualAddress end = addr + size;
                        const size_t align = (m_next_block_shift != 0) ? (u64(1) << m_next_block_shift) : (u64(1) << m_block_shift);
                        addr = util::AlignDown(GetInteger(addr), align);
                        end  = util::AlignUp(GetInteger(end), align);

                        m_heap_address = addr;
                        m_end_offset   = (end - addr) / (u64(1) << m_block_shift);
                        return m_bitmap.Initialize(bit_storage, m_end_offset);
                    }

                    KVirtualAddress PushBlock(KVirtualAddress address) {
                        /* Set the bit for the free block. */
                        size_t offset = (address - m_heap_address) >> this->GetShift();
                        m_bitmap.SetBit(offset);

                        /* If we have a next shift, try to clear the blocks below this one and return the new address. */
                        if (this->GetNextShift()) {
                            const size_t diff = u64(1) << (this->GetNextShift() - this->GetShift());
                            offset = util::AlignDown(offset, diff);
                            if (m_bitmap.ClearRange(offset, diff)) {
                                return m_heap_address + (offset << this->GetShift());
                            }
                        }

                        /* We couldn't coalesce, or we're already as big as possible. */
                        return Null<KVirtualAddress>;
                    }

                    KVirtualAddress PopBlock(bool random) {
                        /* Find a free block. */
                        ssize_t soffset = m_bitmap.FindFreeBlock(random);
                        if (soffset < 0) {
                            return Null<KVirtualAddress>;
                        }
                        const size_t offset = static_cast<size_t>(soffset);

                        /* Update our tracking and return it. */
                        m_bitmap.ClearBit(offset);
                        return m_heap_address + (offset << this->GetShift());
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
            KVirtualAddress m_heap_address;
            size_t m_heap_size;
            size_t m_used_size;
            size_t m_num_blocks;
            Block m_blocks[NumMemoryBlockPageShifts];
        private:
            void Initialize(KVirtualAddress heap_address, size_t heap_size, KVirtualAddress management_address, size_t management_size, const size_t *block_shifts, size_t num_block_shifts);
            size_t GetNumFreePages() const;

            void FreeBlock(KVirtualAddress block, s32 index);
        public:
            KPageHeap() : m_heap_address(), m_heap_size(), m_used_size(), m_num_blocks(), m_blocks() { /* ... */ }

            constexpr KVirtualAddress GetAddress() const { return m_heap_address; }
            constexpr size_t GetSize() const { return m_heap_size; }
            constexpr KVirtualAddress GetEndAddress() const { return this->GetAddress() + this->GetSize(); }
            constexpr size_t GetPageOffset(KVirtualAddress block) const { return (block - this->GetAddress()) / PageSize; }
            constexpr size_t GetPageOffsetToEnd(KVirtualAddress block) const { return (this->GetEndAddress() - block) / PageSize; }

            void Initialize(KVirtualAddress heap_address, size_t heap_size, KVirtualAddress management_address, size_t management_size) {
                return Initialize(heap_address, heap_size, management_address, management_size, MemoryBlockPageShifts, NumMemoryBlockPageShifts);
            }

            size_t GetFreeSize() const { return this->GetNumFreePages() * PageSize; }
            void DumpFreeList() const;

            void UpdateUsedSize() {
                m_used_size = m_heap_size - (this->GetNumFreePages() * PageSize);
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
