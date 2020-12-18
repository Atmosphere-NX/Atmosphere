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

    void KPageHeap::Initialize(KVirtualAddress address, size_t size, KVirtualAddress management_address, size_t management_size, const size_t *block_shifts, size_t num_block_shifts) {
        /* Check our assumptions. */
        MESOSPHERE_ASSERT(util::IsAligned(GetInteger(address), PageSize));
        MESOSPHERE_ASSERT(util::IsAligned(size, PageSize));
        MESOSPHERE_ASSERT(0 < num_block_shifts && num_block_shifts <= NumMemoryBlockPageShifts);
        const KVirtualAddress management_end = management_address + management_size;

        /* Set our members. */
        m_heap_address = address;
        m_heap_size = size;
        m_num_blocks = num_block_shifts;

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

    KVirtualAddress KPageHeap::AllocateBlock(s32 index, bool random) {
        const size_t needed_size = m_blocks[index].GetSize();

        for (s32 i = index; i < static_cast<s32>(m_num_blocks); i++) {
            if (const KVirtualAddress addr = m_blocks[i].PopBlock(random); addr != Null<KVirtualAddress>) {
                if (const size_t allocated_size = m_blocks[i].GetSize(); allocated_size > needed_size) {
                    this->Free(addr + needed_size, (allocated_size - needed_size) / PageSize);
                }
                return addr;
            }
        }

        return Null<KVirtualAddress>;
    }

    void KPageHeap::FreeBlock(KVirtualAddress block, s32 index) {
        do {
            block = m_blocks[index++].PushBlock(block);
        } while (block != Null<KVirtualAddress>);
    }

    void KPageHeap::Free(KVirtualAddress addr, size_t num_pages) {
        /* Freeing no pages is a no-op. */
        if (num_pages == 0) {
            return;
        }

        /* Find the largest block size that we can free, and free as many as possible. */
        s32 big_index = static_cast<s32>(m_num_blocks) - 1;
        const KVirtualAddress start  = addr;
        const KVirtualAddress end    = addr + num_pages * PageSize;
        KVirtualAddress before_start = start;
        KVirtualAddress before_end   = start;
        KVirtualAddress after_start  = end;
        KVirtualAddress after_end    = end;
        while (big_index >= 0) {
            const size_t block_size = m_blocks[big_index].GetSize();
            const KVirtualAddress big_start = util::AlignUp(GetInteger(start), block_size);
            const KVirtualAddress big_end   = util::AlignDown(GetInteger(end), block_size);
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
