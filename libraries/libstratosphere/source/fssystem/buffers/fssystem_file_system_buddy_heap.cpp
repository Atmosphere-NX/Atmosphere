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
#include <stratosphere.hpp>

namespace ams::fssystem {

    FileSystemBuddyHeap::PageEntry *FileSystemBuddyHeap::PageList::PopFront() {
        AMS_ASSERT(this->entry_count > 0);

        /* Get the first entry. */
        auto page_entry = this->first_page_entry;

        /* Advance our list. */
        this->first_page_entry = page_entry->next;
        page_entry->next = nullptr;

        /* Decrement our count. */
        --this->entry_count;
        AMS_ASSERT(this->entry_count >= 0);

        /* If this was our last page, clear our last entry. */
        if (this->entry_count == 0) {
            this->last_page_entry = nullptr;
        }

        return page_entry;
    }

    void FileSystemBuddyHeap::PageList::PushBack(PageEntry *page_entry) {
        AMS_ASSERT(page_entry != nullptr);

        /* If we're empty, we want to set the first page entry. */
        if (this->IsEmpty()) {
            this->first_page_entry = page_entry;
        } else {
            /* We're not empty, so push the page to the back. */
            AMS_ASSERT(this->last_page_entry != page_entry);
            this->last_page_entry->next = page_entry;
        }

        /* Set our last page entry to be this one, and link it to the list. */
        this->last_page_entry = page_entry;
        this->last_page_entry->next = nullptr;

        /* Increment our entry count. */
        ++this->entry_count;
        AMS_ASSERT(this->entry_count > 0);
    }

    bool FileSystemBuddyHeap::PageList::Remove(PageEntry *page_entry) {
        AMS_ASSERT(page_entry != nullptr);

        /* If we're empty, we can't remove the page list. */
        if (this->IsEmpty()) {
            return false;
        }

        /* We're going to loop over all pages to find this one, then unlink it. */
        PageEntry *prev_entry = nullptr;
        PageEntry *cur_entry  = this->first_page_entry;

        while (true) {
            /* Check if we found the page. */
            if (cur_entry == page_entry) {
                if (cur_entry == this->first_page_entry) {
                    /* If it's the first page, we just set our first. */
                    this->first_page_entry = cur_entry->next;
                } else if (cur_entry == this->last_page_entry) {
                    /* If it's the last page, we set our last. */
                    this->last_page_entry = prev_entry;
                    this->last_page_entry->next = nullptr;
                } else {
                    /* If it's in the middle, we just unlink. */
                    prev_entry->next = cur_entry->next;
                }

                /* Unlink this entry's next. */
                cur_entry->next = nullptr;

                /* Update our entry count. */
                --this->entry_count;
                AMS_ASSERT(this->entry_count >= 0);

                return true;
            }

            /* If we have no next page, we can't remove. */
            if (cur_entry->next == nullptr) {
                return false;
            }

            /* Advance to the next item in the list. */
            prev_entry = cur_entry;
            cur_entry  = cur_entry->next;
        }
    }

    Result FileSystemBuddyHeap::Initialize(uintptr_t address, size_t size, size_t block_size, s32 order_max) {
        /* Ensure our preconditions. */
        AMS_ASSERT(this->free_lists == nullptr);
        AMS_ASSERT(address != 0);
        AMS_ASSERT(util::IsAligned(address, BufferAlignment));
        AMS_ASSERT(block_size >= BlockSizeMin);
        AMS_ASSERT(util::IsPowerOfTwo(block_size));
        AMS_ASSERT(size >= block_size);
        AMS_ASSERT(order_max > 0);
        AMS_ASSERT(order_max < OrderUpperLimit);

        /* Set up our basic member variables */
        this->block_size = block_size;
        this->order_max  = order_max;
        this->heap_start = address;
        this->heap_size  = (size / this->block_size) * this->block_size;

        this->total_free_size = 0;

        /* Determine page sizes. */
        const auto max_page_size  = this->block_size << this->order_max;
        const auto max_page_count = util::AlignUp(this->heap_size, max_page_size) / max_page_size;
        AMS_ASSERT(max_page_count > 0);

        /* Setup the free lists. */
        if (this->external_free_lists != nullptr) {
            AMS_ASSERT(this->internal_free_lists == nullptr);
            this->free_lists = this->external_free_lists;
        } else {
            this->internal_free_lists.reset(new PageList[this->order_max + 1]);
            this->free_lists = this->internal_free_lists.get();
            R_UNLESS(this->free_lists != nullptr, fs::ResultAllocationFailureInFileSystemBuddyHeapA());
        }

        /* All but the last page region should go to the max order. */
        for (size_t i = 0; i < max_page_count - 1; i++) {
            auto page_entry = this->GetPageEntryFromAddress(this->heap_start + i * max_page_size);
            this->free_lists[this->order_max].PushBack(page_entry);
        }
        this->total_free_size += this->free_lists[this->order_max].GetSize() * this->GetBytesFromOrder(this->order_max);

        /* Allocate remaining space to smaller orders as possible. */
        {
            auto remaining   = this->heap_size  - (max_page_count - 1) * max_page_size;
            auto cur_address = this->heap_start + (max_page_count - 1) * max_page_size;
            AMS_ASSERT(util::IsAligned(remaining, this->block_size));

            do {
                /* Determine what order we can use. */
                auto order = GetOrderFromBytes(remaining + 1);
                if (order < 0) {
                    AMS_ASSERT(GetOrderFromBytes(remaining) == this->order_max);
                    order = this->order_max + 1;
                }
                AMS_ASSERT(0 < order);
                AMS_ASSERT(order <= this->order_max + 1);

                /* Add to the correct free list. */
                this->free_lists[order - 1].PushBack(GetPageEntryFromAddress(cur_address));
                this->total_free_size += GetBytesFromOrder(order - 1);

                /* Move on to the next order. */
                const auto page_size = GetBytesFromOrder(order - 1);
                cur_address += page_size;
                remaining   -= page_size;
            } while (this->block_size <= remaining);
        }

        return ResultSuccess();
    }

    void FileSystemBuddyHeap::Finalize() {
        AMS_ASSERT(this->free_lists != nullptr);
        this->free_lists = nullptr;
        this->external_free_lists = nullptr;
        this->internal_free_lists.reset();
    }

    void *FileSystemBuddyHeap::AllocateByOrder(s32 order) {
        AMS_ASSERT(this->free_lists != nullptr);
        AMS_ASSERT(order >= 0);
        AMS_ASSERT(order <= this->GetOrderMax());

        /* Get the page entry. */
        if (const auto page_entry = this->GetFreePageEntry(order); page_entry != nullptr) {
            /* Ensure we're allocating an unlinked page. */
            AMS_ASSERT(page_entry->next == nullptr);

            /* Return the address for this entry. */
            return reinterpret_cast<void *>(this->GetAddressFromPageEntry(*page_entry));
        } else {
            return nullptr;
        }
    }

    void FileSystemBuddyHeap::Free(void *ptr, s32 order) {
        AMS_ASSERT(this->free_lists != nullptr);
        AMS_ASSERT(order >= 0);
        AMS_ASSERT(order <= this->GetOrderMax());

        /* Allow free(nullptr) */
        if (ptr == nullptr) {
            return;
        }

        /* Ensure the pointer is block aligned. */
        AMS_ASSERT(util::IsAligned(reinterpret_cast<uintptr_t>(ptr) - this->heap_start, this->GetBlockSize()));

        /* Get the page entry. */
        auto page_entry = this->GetPageEntryFromAddress(reinterpret_cast<uintptr_t>(ptr));
        AMS_ASSERT(this->IsAlignedToOrder(page_entry, order));

        /* Reinsert into the free lists. */
        this->JoinBuddies(page_entry, order);
    }

    size_t FileSystemBuddyHeap::GetTotalFreeSize() const {
        AMS_ASSERT(this->free_lists != nullptr);
        return this->total_free_size;
    }

    size_t FileSystemBuddyHeap::GetAllocatableSizeMax() const {
        AMS_ASSERT(this->free_lists != nullptr);

        /* The maximum allocatable size is a chunk from the biggest non-empty order. */
        for (s32 order = this->GetOrderMax(); order >= 0; --order) {
            if (!this->free_lists[order].IsEmpty()) {
                return this->GetBytesFromOrder(order);
            }
        }

        /* If all orders are empty, then we can't allocate anything. */
        return 0;
    }

    void FileSystemBuddyHeap::Dump() const {
        AMS_ASSERT(this->free_lists != nullptr);
        /* TODO: Support logging metrics. */
    }

    void FileSystemBuddyHeap::DivideBuddies(PageEntry *page_entry, s32 required_order, s32 chosen_order) {
        AMS_ASSERT(page_entry != nullptr);
        AMS_ASSERT(required_order >= 0);
        AMS_ASSERT(chosen_order >= required_order);
        AMS_ASSERT(chosen_order <= this->GetOrderMax());

        /* Start at the end of the entry. */
        auto address = this->GetAddressFromPageEntry(*page_entry) + this->GetBytesFromOrder(chosen_order);

        for (auto order = chosen_order; order > required_order; --order) {
            /* For each order, subtract that order's size from the address to get the start of a new block. */
            address -= this->GetBytesFromOrder(order - 1);
            auto divided_entry = this->GetPageEntryFromAddress(address);

            /* Push back to the list. */
            this->free_lists[order - 1].PushBack(divided_entry);
            this->total_free_size += this->GetBytesFromOrder(order - 1);
        }
    }

    void FileSystemBuddyHeap::JoinBuddies(PageEntry *page_entry, s32 order) {
        AMS_ASSERT(page_entry != nullptr);
        AMS_ASSERT(order >= 0);
        AMS_ASSERT(order <= this->GetOrderMax());

        auto cur_entry = page_entry;
        auto cur_order = order;

        while (cur_order < this->GetOrderMax()) {
            /* Get the buddy page. */
            const auto buddy_entry = this->GetBuddy(cur_entry, cur_order);

            /* Check whether the buddy is in the relevant free list. */
            if (buddy_entry != nullptr && this->free_lists[cur_order].Remove(buddy_entry)) {
                this->total_free_size -= GetBytesFromOrder(cur_order);

                /* Ensure we coalesce with the correct buddy when page is aligned */
                if (!this->IsAlignedToOrder(cur_entry, cur_order + 1)) {
                    cur_entry = buddy_entry;
                }

                ++cur_order;
            } else {
                /* Buddy isn't in the free list, so we can't coalesce. */
                break;
            }
        }

        /* Insert the coalesced entry into the free list. */
        this->free_lists[cur_order].PushBack(cur_entry);
        this->total_free_size += this->GetBytesFromOrder(cur_order);
    }

    FileSystemBuddyHeap::PageEntry *FileSystemBuddyHeap::GetBuddy(PageEntry *page_entry, s32 order) {
        AMS_ASSERT(page_entry != nullptr);
        AMS_ASSERT(order >= 0);
        AMS_ASSERT(order <= this->GetOrderMax());

        const auto address = this->GetAddressFromPageEntry(*page_entry);
        const auto offset  = this->GetBlockCountFromOrder(order) * this->GetBlockSize();

        if (this->IsAlignedToOrder(page_entry, order + 1)) {
            /* If the page entry is aligned to the next order, return the buddy block to the right of the current entry. */
            return (address + offset < this->heap_start + this->heap_size) ? GetPageEntryFromAddress(address + offset) : nullptr;
        } else {
            /* If the page entry isn't aligned, return the buddy block to the left of the current entry. */
            return (this->heap_start <= address - offset) ? GetPageEntryFromAddress(address - offset) : nullptr;
        }
    }

    FileSystemBuddyHeap::PageEntry *FileSystemBuddyHeap::GetFreePageEntry(s32 order) {
        AMS_ASSERT(order >= 0);
        AMS_ASSERT(order <= this->GetOrderMax());

        /* Try orders from low to high until we find a free page entry. */
        for (auto cur_order = order; cur_order <= this->GetOrderMax(); cur_order++) {
            if (auto &free_list = this->free_lists[cur_order]; !free_list.IsEmpty()) {
                /* The current list isn't empty, so grab an entry from it. */
                PageEntry *page_entry = free_list.PopFront();
                AMS_ASSERT(page_entry != nullptr);

                /* Update size bookkeeping. */
                this->total_free_size -= GetBytesFromOrder(cur_order);

                /* If we allocated more memory than needed, free the unneeded portion. */
                this->DivideBuddies(page_entry, order, cur_order);
                AMS_ASSERT(page_entry->next == nullptr);

                /* Return the newly-divided entry. */
                return page_entry;
            }
        }

        /* We failed to find a free page. */
        return nullptr;
    }

    s32 FileSystemBuddyHeap::GetOrderFromBlockCount(s32 block_count) const {
        AMS_ASSERT(block_count >= 0);

        /* Return the first order with a big enough block count. */
        for (s32 order = 0; order <= this->GetOrderMax(); ++order) {
            if (block_count <= this->GetBlockCountFromOrder(order)) {
                return order;
            }
        }
        return -1;
    }

}
