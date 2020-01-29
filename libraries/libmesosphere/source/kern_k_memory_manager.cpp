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

        constexpr size_t g_memory_block_page_shifts[] = { 0xC, 0x10, 0x15, 0x16, 0x19, 0x1D, 0x1E };
        constexpr size_t NumMemoryBlockPageShifts = util::size(g_memory_block_page_shifts);

    }

    size_t KMemoryManager::Impl::CalculateMetadataOverheadSize(size_t region_size) {
        const size_t ref_count_size = (region_size / PageSize) * sizeof(u16);
        const size_t bitmap_size    = (util::AlignUp((region_size / PageSize), BITSIZEOF(u64)) / BITSIZEOF(u64)) * sizeof(u64);
        const size_t page_heap_size = KPageHeap::CalculateMetadataOverheadSize(region_size, g_memory_block_page_shifts, NumMemoryBlockPageShifts);
        return util::AlignUp(page_heap_size + bitmap_size + ref_count_size, PageSize);
    }

}
