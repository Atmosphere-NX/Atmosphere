/*
 * Copyright (c) Atmosphère-NX
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

    void KPageHeap::Initialize(KPhysicalAddress address, size_t size, KVirtualAddress management_address, size_t management_size, const size_t *block_shifts, size_t num_block_shifts) {
        /* Check our assumptions. */
        MESOSPHERE_ASSERT(util::IsAligned(GetInteger(address), PageSize));
        MESOSPHERE_ASSERT(util::IsAligned(size, PageSize));
        MESOSPHERE_ASSERT(0 < num_block_shifts && num_block_shifts <= NumMemoryBlockPageShifts);
        const KVirtualAddress management_end = management_address + management_size;

        /* Set our members. */
        m_heap_address = address;
        m_heap_size    = size;
        m_num_blocks   = num_block_shifts;

        /* Setup bitmaps. */
        u64 *cur_bitmap_storage = GetPointer<u64>(management_address);
        for (size_t i = 0; i < num_block_shifts; i++) {
            const size_t cur_block_shift  = block_shifts[i];
            const size_t next_block_shift = (i != num_block_shifts - 1) ? block_shifts[i + 1] : 0;
            cur_bitmap_storage = m_blocks[i].Initialize(m_heap_address, m_heap_size, cur_block_shift, next_block_shift, cur_bitmap_storage);
        }

        /* Ensure we didn't overextend our bounds. */
        MESOSPHERE_ABORT_UNLESS(KVirtualAddress(cur_bitmap_storage) <= management_end);
    }

    size_t KPageHeap::GetNumFreePages() const {
        size_t num_free = 0;

        for (size_t i = 0; i < m_num_blocks; i++) {
            num_free += m_blocks[i].GetNumFreePages();
        }

        return num_free;
    }

    KPhysicalAddress KPageHeap::AllocateByLinearSearch(s32 index) {
        const size_t needed_size = m_blocks[index].GetSize();

        for (s32 i = index; i < static_cast<s32>(m_num_blocks); i++) {
            if (const KPhysicalAddress addr = m_blocks[i].PopBlock(false); addr != Null<KPhysicalAddress>) {
                if (const size_t allocated_size = m_blocks[i].GetSize(); allocated_size > needed_size) {
                    this->Free(addr + needed_size, (allocated_size - needed_size) / PageSize);
                }
                return addr;
            }
        }

        return Null<KPhysicalAddress>;
    }

    KPhysicalAddress KPageHeap::AllocateByRandom(s32 index, size_t num_pages, size_t align_pages) {
        /* Get the size and required alignment. */
        const size_t needed_size = num_pages   * PageSize;
        const size_t align_size  = align_pages * PageSize;

        /* Determine meta-alignment of our desired alignment size. */
        const size_t align_shift = util::CountTrailingZeros(align_size);

        /* Decide on a block to allocate from. */
        constexpr size_t MinimumPossibleAlignmentsForRandomAllocation = 4;
        {
            /* By default, we'll want to look at all blocks larger than our current one. */
            s32 max_blocks = static_cast<s32>(m_num_blocks);

            /* Determine the maximum block we should try to allocate from. */
            size_t possible_alignments = 0;
            for (s32 i = index; i < max_blocks; ++i) {
                /* Add the possible alignments from blocks at the current size. */
                possible_alignments += (1 + ((m_blocks[i].GetSize() - needed_size) >> align_shift)) * m_blocks[i].GetNumFreeBlocks();

                /* If there are enough possible alignments, we don't need to look at larger blocks. */
                if (possible_alignments >= MinimumPossibleAlignmentsForRandomAllocation) {
                    max_blocks = i + 1;
                    break;
                }
            }

            /* If we have any possible alignments which require a larger block, we need to pick one. */
            if (possible_alignments > 0 && index + 1 < max_blocks) {
                /* Select a random alignment from the possibilities. */
                const size_t rnd = m_rng.GenerateRandom(possible_alignments);

                /* Determine which block corresponds to the random alignment we chose. */
                possible_alignments = 0;
                for (s32 i = index; i < max_blocks; ++i) {
                    /* Add the possible alignments from blocks at the current size. */
                    possible_alignments += (1 + ((m_blocks[i].GetSize() - needed_size) >> align_shift)) * m_blocks[i].GetNumFreeBlocks();

                    /* If the current block gets us to our random choice, use the current block. */
                    if (rnd < possible_alignments) {
                        index = i;
                        break;
                    }
                }
            }
        }

        /* Pop a block from the index we selected. */
        if (KPhysicalAddress addr = m_blocks[index].PopBlock(true); addr != Null<KPhysicalAddress>) {
            /* Determine how much size we have left over. */
            if (const size_t leftover_size = m_blocks[index].GetSize() - needed_size; leftover_size > 0) {
                /* Determine how many valid alignments we can have. */
                const size_t possible_alignments = 1 + (leftover_size >> align_shift);

                /* Select a random valid alignment. */
                const size_t random_offset = m_rng.GenerateRandom(possible_alignments) << align_shift;

                /* Free memory before the random offset. */
                if (random_offset != 0) {
                    this->Free(addr, random_offset / PageSize);
                }

                /* Advance our block by the random offset. */
                addr += random_offset;

                /* Free memory after our allocated block. */
                if (random_offset != leftover_size) {
                    this->Free(addr + needed_size, (leftover_size - random_offset) / PageSize);
                }
            }

            /* Return the block we allocated. */
            return addr;
        }

        return Null<KPhysicalAddress>;
    }

    void KPageHeap::FreeBlock(KPhysicalAddress block, s32 index) {
        do {
            block = m_blocks[index++].PushBlock(block);
        } while (block != Null<KPhysicalAddress>);
    }

    void KPageHeap::Free(KPhysicalAddress addr, size_t num_pages) {
        /* Freeing no pages is a no-op. */
        if (num_pages == 0) {
            return;
        }

        /* Find the largest block size that we can free, and free as many as possible. */
        s32 big_index = static_cast<s32>(m_num_blocks) - 1;
        const KPhysicalAddress start  = addr;
        const KPhysicalAddress end    = addr + num_pages * PageSize;
        KPhysicalAddress before_start = start;
        KPhysicalAddress before_end   = start;
        KPhysicalAddress after_start  = end;
        KPhysicalAddress after_end    = end;
        while (big_index >= 0) {
            const size_t block_size = m_blocks[big_index].GetSize();
            const KPhysicalAddress big_start = util::AlignUp(GetInteger(start), block_size);
            const KPhysicalAddress big_end   = util::AlignDown(GetInteger(end), block_size);
            if (big_start < big_end) {
                /* Free as many big blocks as we can. */
                for (auto block = big_start; block < big_end; block += block_size) {
                    this->FreeBlock(block, big_index);
                }
                before_end  = big_start;
                after_start = big_end;
                break;
            }
            big_index--;
        }
        MESOSPHERE_ASSERT(big_index >= 0);

        /* Free space before the big blocks. */
        for (s32 i = big_index - 1; i >= 0; i--) {
            const size_t block_size = m_blocks[i].GetSize();
            while (before_start + block_size <= before_end) {
                before_end -= block_size;
                this->FreeBlock(before_end, i);
            }
        }

        /* Free space after the big blocks. */
        for (s32 i = big_index - 1; i >= 0; i--) {
            const size_t block_size = m_blocks[i].GetSize();
            while (after_start + block_size <= after_end) {
                this->FreeBlock(after_start, i);
                after_start += block_size;
            }
        }
    }

    size_t KPageHeap::CalculateManagementOverheadSize(size_t region_size, const size_t *block_shifts, size_t num_block_shifts) {
        size_t overhead_size = 0;
        for (size_t i = 0; i < num_block_shifts; i++) {
            const size_t cur_block_shift  = block_shifts[i];
            const size_t next_block_shift = (i != num_block_shifts - 1) ? block_shifts[i + 1] : 0;
            overhead_size += KPageHeap::Block::CalculateManagementOverheadSize(region_size, cur_block_shift, next_block_shift);
        }
        return util::AlignUp(overhead_size, PageSize);
    }

    void KPageHeap::DumpFreeList() const {
        MESOSPHERE_RELEASE_LOG("KPageHeap::DumpFreeList %p\n", this);

        for (size_t i = 0; i < m_num_blocks; ++i) {
            const size_t block_size = m_blocks[i].GetSize();
            const char *suffix;
            size_t size;
            if (block_size >= 1_GB) {
                suffix = "GiB";
                size   = block_size / 1_GB;
            } else if (block_size >= 1_MB) {
                suffix = "MiB";
                size   = block_size / 1_MB;
            } else if (block_size >= 1_KB) {
                suffix = "KiB";
                size   = block_size / 1_KB;
            } else {
                suffix = "B";
                size   = block_size;
            }

            MESOSPHERE_RELEASE_LOG("    %4zu %s block x %zu\n", size, suffix, m_blocks[i].GetNumFreeBlocks());
        }
    }

}
