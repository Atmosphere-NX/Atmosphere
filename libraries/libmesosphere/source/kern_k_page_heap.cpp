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

    void KPageHeap::Initialize(KVirtualAddress address, size_t size, KVirtualAddress metadata_address, size_t metadata_size, const size_t *block_shifts, size_t num_block_shifts) {
        /* Check our assumptions. */
        MESOSPHERE_ASSERT(util::IsAligned(GetInteger(address), PageSize));
        MESOSPHERE_ASSERT(util::IsAligned(size, PageSize));
        MESOSPHERE_ASSERT(0 < num_block_shifts && num_block_shifts <= NumMemoryBlockPageShifts);
        const KVirtualAddress metadata_end = metadata_address + metadata_size;

        /* Set our members. */
        this->heap_address = address;
        this->heap_size = size;
        this->num_blocks = num_block_shifts;

        /* Setup bitmaps. */
        u64 *cur_bitmap_storage = GetPointer<u64>(metadata_address);
        for (size_t i = 0; i < num_block_shifts; i++) {
            const size_t cur_block_shift  = block_shifts[i];
            const size_t next_block_shift = (i != num_block_shifts - 1) ? block_shifts[i + 1] : 0;
            cur_bitmap_storage = this->blocks[i].Initialize(this->heap_address, this->heap_size, cur_block_shift, next_block_shift, cur_bitmap_storage);
        }

        /* Ensure we didn't overextend our bounds. */
        MESOSPHERE_ABORT_UNLESS(KVirtualAddress(cur_bitmap_storage) <= metadata_end);
    }

    size_t KPageHeap::CalculateMetadataOverheadSize(size_t region_size, const size_t *block_shifts, size_t num_block_shifts) {
        size_t overhead_size = 0;
        for (size_t i = 0; i < num_block_shifts; i++) {
            const size_t cur_block_shift  = block_shifts[i];
            const size_t next_block_shift = (i != num_block_shifts - 1) ? block_shifts[i + 1] : 0;
            overhead_size += KPageHeap::Block::CalculateMetadataOverheadSize(region_size, cur_block_shift, next_block_shift);
        }
        return util::AlignUp(overhead_size, PageSize);
    }

}
