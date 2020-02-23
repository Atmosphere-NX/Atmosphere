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
                    class Bitmap {
                        public:
                            static constexpr size_t MaxDepth = 4;
                        private:
                            u64 *bit_storages[MaxDepth];
                            size_t num_bits;
                            size_t used_depths;
                        public:
                            constexpr Bitmap() : bit_storages(), num_bits(), used_depths() { /* ... */ }

                            constexpr size_t GetNumBits() const { return this->num_bits; }
                            constexpr s32 GetHighestDepthIndex() const { return static_cast<s32>(this->used_depths) - 1; }

                            u64 *Initialize(u64 *storage, size_t size) {
                                /* Initially, everything is un-set. */
                                this->num_bits = 0;

                                /* Calculate the needed bitmap depth. */
                                this->used_depths = static_cast<size_t>(GetRequiredDepth(size));
                                MESOSPHERE_ASSERT(this->used_depths <= MaxDepth);

                                /* Set the bitmap pointers. */
                                for (s32 depth = this->GetHighestDepthIndex(); depth >= 0; depth--) {
                                    this->bit_storages[depth] = storage;
                                    size = util::AlignUp(size, BITSIZEOF(u64)) / BITSIZEOF(u64);
                                    storage += size;
                                }

                                return storage;
                            }

                            ssize_t FindFreeBlock() const {
                                uintptr_t offset = 0;
                                s32 depth = 0;

                                do {
                                    const u64 v = this->bit_storages[depth][offset];
                                    if (v == 0) {
                                        /* If depth is bigger than zero, then a previous level indicated a block was free. */
                                        MESOSPHERE_ASSERT(depth == 0);
                                        return -1;
                                    }
                                    offset = offset * BITSIZEOF(u64) + __builtin_ctzll(v);
                                    ++depth;
                                } while (depth < static_cast<s32>(this->used_depths));

                                return static_cast<ssize_t>(offset);
                            }

                            void SetBit(size_t offset) {
                                this->SetBit(this->GetHighestDepthIndex(), offset);
                                this->num_bits++;
                            }

                            void ClearBit(size_t offset) {
                                this->ClearBit(this->GetHighestDepthIndex(), offset);
                                this->num_bits--;
                            }

                            bool ClearRange(size_t offset, size_t count) {
                                s32 depth = this->GetHighestDepthIndex();
                                u64 *bits = this->bit_storages[depth];
                                size_t bit_ind = offset / BITSIZEOF(u64);
                                if (AMS_LIKELY(count < BITSIZEOF(u64))) {
                                    const size_t shift = offset % BITSIZEOF(u64);
                                    MESOSPHERE_ASSERT(shift + count <= BITSIZEOF(u64));
                                    /* Check that all the bits are set. */
                                    const u64 mask = ((u64(1) << count) - 1) << shift;
                                    u64 v = bits[bit_ind];
                                    if ((v & mask) != mask) {
                                        return false;
                                    }

                                    /* Clear the bits. */
                                    v &= ~mask;
                                    bits[bit_ind] = v;
                                    if (v == 0) {
                                        this->ClearBit(depth - 1, bit_ind);
                                    }
                                } else {
                                    MESOSPHERE_ASSERT(offset % BITSIZEOF(u64) == 0);
                                    MESOSPHERE_ASSERT(count  % BITSIZEOF(u64) == 0);
                                    /* Check that all the bits are set. */
                                    size_t remaining = count;
                                    size_t i = 0;
                                    do {
                                        if (bits[bit_ind + i++] != ~u64(0)) {
                                            return false;
                                        }
                                        remaining -= BITSIZEOF(u64);
                                    } while (remaining > 0);

                                    /* Clear the bits. */
                                    remaining = count;
                                    i = 0;
                                    do {
                                        bits[bit_ind + i] = 0;
                                        this->ClearBit(depth - 1, bit_ind + i);
                                        i++;
                                        remaining -= BITSIZEOF(u64);
                                    } while (remaining > 0);
                                }

                                this->num_bits -= count;
                                return true;
                            }
                        private:
                            void SetBit(s32 depth, size_t offset) {
                                while (depth >= 0) {
                                    size_t ind   = offset / BITSIZEOF(u64);
                                    size_t which = offset % BITSIZEOF(u64);
                                    const u64 mask = u64(1) << which;

                                    u64 *bit = std::addressof(this->bit_storages[depth][ind]);
                                    u64 v = *bit;
                                    MESOSPHERE_ASSERT((v & mask) == 0);
                                    *bit = v | mask;
                                    if (v) {
                                        break;
                                    }
                                    offset = ind;
                                    depth--;
                                }
                            }

                            void ClearBit(s32 depth, size_t offset) {
                                while (depth >= 0) {
                                    size_t ind   = offset / BITSIZEOF(u64);
                                    size_t which = offset % BITSIZEOF(u64);
                                    const u64 mask = u64(1) << which;

                                    u64 *bit = std::addressof(this->bit_storages[depth][ind]);
                                    u64 v = *bit;
                                    MESOSPHERE_ASSERT((v & mask) != 0);
                                    v &= ~mask;
                                    *bit = v;
                                    if (v) {
                                        break;
                                    }
                                    offset = ind;
                                    depth--;
                                }
                            }
                        private:
                            static constexpr s32 GetRequiredDepth(size_t region_size) {
                                s32 depth = 0;
                                while (true) {
                                    region_size /= BITSIZEOF(u64);
                                    depth++;
                                    if (region_size == 0) {
                                        return depth;
                                    }
                                }
                            }
                        public:
                            static constexpr size_t CalculateMetadataOverheadSize(size_t region_size) {
                                size_t overhead_bits = 0;
                                for (s32 depth = GetRequiredDepth(region_size) - 1; depth >= 0; depth--) {
                                    region_size = util::AlignUp(region_size, BITSIZEOF(u64)) / BITSIZEOF(u64);
                                    overhead_bits += region_size;
                                }
                                return overhead_bits * sizeof(u64);
                            }
                    };
                private:
                    Bitmap bitmap;
                    KVirtualAddress heap_address;
                    uintptr_t end_offset;
                    size_t block_shift;
                    size_t next_block_shift;
                public:
                    constexpr Block() : bitmap(), heap_address(), end_offset(), block_shift(), next_block_shift() { /* ... */ }

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
                        const size_t align = (this->next_block_shift != 0) ? (u64(1) << this->next_block_shift) : (this->block_shift);
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

                    KVirtualAddress PopBlock() {
                        /* Find a free block. */
                        ssize_t soffset = this->bitmap.FindFreeBlock();
                        if (soffset < 0) {
                            return Null<KVirtualAddress>;
                        }
                        const size_t offset = static_cast<size_t>(soffset);

                        /* Update our tracking and return it. */
                        this->bitmap.ClearBit(offset);
                        return this->heap_address + (offset << this->GetShift());
                    }
                public:
                    static constexpr size_t CalculateMetadataOverheadSize(size_t region_size, size_t cur_block_shift, size_t next_block_shift) {
                        const size_t cur_block_size  = (u64(1) << cur_block_shift);
                        const size_t next_block_size = (u64(1) << next_block_shift);
                        const size_t align = (next_block_shift != 0) ? next_block_size : cur_block_size;
                        return Bitmap::CalculateMetadataOverheadSize((align * 2 + util::AlignUp(region_size, align)) / cur_block_size);
                    }
            };
        private:
            KVirtualAddress heap_address;
            size_t heap_size;
            size_t used_size;
            size_t num_blocks;
            Block blocks[NumMemoryBlockPageShifts];
        private:
            void Initialize(KVirtualAddress heap_address, size_t heap_size, KVirtualAddress metadata_address, size_t metadata_size, const size_t *block_shifts, size_t num_block_shifts);
            size_t GetNumFreePages() const;

            void FreeBlock(KVirtualAddress block, s32 index);
        public:
            constexpr KPageHeap() : heap_address(), heap_size(), used_size(), num_blocks(), blocks() { /* ... */ }

            constexpr KVirtualAddress GetAddress() const { return this->heap_address; }
            constexpr size_t GetSize() const { return this->heap_size; }
            constexpr KVirtualAddress GetEndAddress() const { return this->GetAddress() + this->GetSize(); }
            constexpr size_t GetPageOffset(KVirtualAddress block) const { return (block - this->GetAddress()) / PageSize; }

            void Initialize(KVirtualAddress heap_address, size_t heap_size, KVirtualAddress metadata_address, size_t metadata_size) {
                return Initialize(heap_address, heap_size, metadata_address, metadata_size, MemoryBlockPageShifts, NumMemoryBlockPageShifts);
            }

            void UpdateUsedSize() {
                this->used_size = this->heap_size - (this->GetNumFreePages() * PageSize);
            }

            KVirtualAddress AllocateBlock(s32 index);
            void Free(KVirtualAddress addr, size_t num_pages);
        private:
            static size_t CalculateMetadataOverheadSize(size_t region_size, const size_t *block_shifts, size_t num_block_shifts);
        public:
            static size_t CalculateMetadataOverheadSize(size_t region_size) {
                return CalculateMetadataOverheadSize(region_size, MemoryBlockPageShifts, NumMemoryBlockPageShifts);
            }
    };

}
