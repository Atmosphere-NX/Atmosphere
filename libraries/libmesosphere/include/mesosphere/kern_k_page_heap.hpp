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
#include <vapours.hpp>

namespace ams::kern {

    class KPageHeap {
        private:
            class Block {
                private:
                    class Bitmap {
                        /* TODO: This is a four-level bitmap tracking page usage. */
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
                public:
                    static constexpr size_t CalculateMetadataOverheadSize(size_t region_size, size_t cur_block_shift, size_t next_block_shift) {
                        const size_t cur_block_size  = (1ul << cur_block_shift);
                        const size_t next_block_size = (1ul << next_block_shift);
                        const size_t align = (next_block_shift != 0) ? next_block_size : cur_block_size;
                        return Bitmap::CalculateMetadataOverheadSize((align * 2 + util::AlignUp(region_size, align)) / cur_block_size);
                    }
            };
        public:
            static size_t CalculateMetadataOverheadSize(size_t region_size, const size_t *block_shifts, size_t num_block_shifts);
    };

}
