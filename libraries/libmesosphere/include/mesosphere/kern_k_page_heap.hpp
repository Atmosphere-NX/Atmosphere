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
        private:
            class Block {
                private:
                    class Bitmap {
                        public:
                            static constexpr size_t MaxDepth = 4;
                        private:
                            u64 *bit_storages[MaxDepth];
                            size_t num_bits;
                            size_t depth;
                        public:
                            constexpr Bitmap() : bit_storages(), num_bits(), depth() { /* ... */ }

                            u64 *Initialize(u64 *storage, size_t size) {
                                /* Initially, everything is un-set. */
                                this->num_bits = 0;

                                /* Calculate the needed bitmap depth. */
                                this->depth = static_cast<size_t>(GetRequiredDepth(size));
                                MESOSPHERE_ASSERT(this->depth <= MaxDepth);

                                /* Set the bitmap pointers. */
                                for (s32 d = static_cast<s32>(this->depth) - 1; d >= 0; d--) {
                                    this->bit_storages[d] = storage;
                                    size = util::AlignUp(size, BITSIZEOF(u64)) / BITSIZEOF(u64);
                                    storage += size;
                                }

                                return storage;
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

                    u64 *Initialize(KVirtualAddress addr, size_t size, size_t bs, size_t nbs, u64 *bit_storage) {
                        /* Set shifts. */
                        this->block_shift = bs;
                        this->next_block_shift = nbs;

                        /* Align up the address. */
                        KVirtualAddress end = addr + size;
                        const size_t align = (this->next_block_shift != 0) ? (1ul << this->next_block_shift) : (this->block_shift);
                        addr = util::AlignDown(GetInteger(addr), align);
                        end  = util::AlignUp(GetInteger(end), align);

                        this->heap_address = addr;
                        this->end_offset   = (end - addr) / (1ul << this->block_shift);
                        return this->bitmap.Initialize(bit_storage, this->end_offset);
                    }
                public:
                    static constexpr size_t CalculateMetadataOverheadSize(size_t region_size, size_t cur_block_shift, size_t next_block_shift) {
                        const size_t cur_block_size  = (1ul << cur_block_shift);
                        const size_t next_block_size = (1ul << next_block_shift);
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
        public:
            constexpr KPageHeap() : heap_address(), heap_size(), used_size(), num_blocks(), blocks() { /* ... */ }

            void Initialize(KVirtualAddress heap_address, size_t heap_size, KVirtualAddress metadata_address, size_t metadata_size) {
                return Initialize(heap_address, heap_size, metadata_address, metadata_size, MemoryBlockPageShifts, NumMemoryBlockPageShifts);
            }
        private:
            static size_t CalculateMetadataOverheadSize(size_t region_size, const size_t *block_shifts, size_t num_block_shifts);
        public:
            static size_t CalculateMetadataOverheadSize(size_t region_size) {
                return CalculateMetadataOverheadSize(region_size, MemoryBlockPageShifts, NumMemoryBlockPageShifts);
            }
    };

}
