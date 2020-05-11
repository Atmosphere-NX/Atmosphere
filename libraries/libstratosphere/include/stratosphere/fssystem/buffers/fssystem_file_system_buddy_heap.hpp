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
#include <stratosphere/fs/impl/fs_newable.hpp>

namespace ams::fssystem {

    class FileSystemBuddyHeap {
        NON_COPYABLE(FileSystemBuddyHeap);
        NON_MOVEABLE(FileSystemBuddyHeap);
        public:
            static constexpr size_t BufferAlignment = sizeof(void *);
            static constexpr size_t BlockSizeMin    = 2 * sizeof(void *);
            static constexpr s32 OrderUpperLimit    = BITSIZEOF(s32) - 1;
        private:
            class PageList;

            struct PageEntry { PageEntry *next; };
            static_assert(util::is_pod<PageEntry>::value);
            static_assert(sizeof(PageEntry) <= BlockSizeMin);

            class PageList : public ::ams::fs::impl::Newable {
                NON_COPYABLE(PageList);
                NON_MOVEABLE(PageList);
                private:
                    PageEntry *first_page_entry;
                    PageEntry *last_page_entry;
                    s32 entry_count;
                public:
                    constexpr  PageList() : first_page_entry(), last_page_entry(), entry_count() { /* ... */ }

                    constexpr bool IsEmpty() const { return this->entry_count == 0; }
                    constexpr s32  GetSize() const { return this->entry_count; }

                    constexpr const PageEntry *GetFront() const { return this->first_page_entry; }
                public:
                    PageEntry *PopFront();
                    void PushBack(PageEntry *page_entry);
                    bool Remove(PageEntry *page_entry);
            };
        private:
            size_t block_size;
            s32    order_max;
            uintptr_t heap_start;
            size_t    heap_size;

            PageList *free_lists;
            size_t total_free_size;
            PageList *external_free_lists;
            std::unique_ptr<PageList[]> internal_free_lists;
        public:
            static constexpr s32 GetBlockCountFromOrder(s32 order) {
                AMS_ASSERT(0 <= order);
                AMS_ASSERT(order < OrderUpperLimit);
                return (1 << order);
            }

            static constexpr size_t QueryWorkBufferSize(s32 order_max) {
                AMS_ASSERT(0 < order_max && order_max < OrderUpperLimit);
                return util::AlignUp(sizeof(PageList) * (order_max + 1) + alignof(PageList), alignof(u64));
            }

            static constexpr s32 QueryOrderMax(size_t size, size_t block_size) {
                AMS_ASSERT(size >= block_size);
                AMS_ASSERT(block_size >= BlockSizeMin);
                AMS_ASSERT(util::IsPowerOfTwo(block_size));
                const auto block_count = static_cast<s32>(util::AlignUp(size, block_size) / block_size);
                for (auto order = 1; true; order++) {
                    if (block_count <= GetBlockCountFromOrder(order)) {
                        return order;
                    }
                }
            }

        public:
            constexpr FileSystemBuddyHeap() : block_size(), order_max(), heap_start(), heap_size(), free_lists(), total_free_size(), external_free_lists(), internal_free_lists() { /* ... */ }

            Result Initialize(uintptr_t address, size_t size, size_t block_size, s32 order_max);

            Result Initialize(uintptr_t address, size_t size, size_t block_size) {
                return this->Initialize(address, size, block_size, QueryOrderMax(size, block_size));
            }

            Result Initialize(uintptr_t address, size_t size, size_t block_size, s32 order_max, void *work, size_t work_size) {
                AMS_ASSERT(work_size >= QueryWorkBufferSize(order_max));
                const auto aligned_work = util::AlignUp(reinterpret_cast<uintptr_t>(work), alignof(PageList));
                this->external_free_lists = reinterpret_cast<PageList *>(aligned_work);
                return this->Initialize(address, size, block_size, order_max);
            }

            Result Initialize(uintptr_t address, size_t size, size_t block_size, void *work, size_t work_size) {
                return this->Initialize(address, size, block_size, QueryOrderMax(size, block_size), work, work_size);
            }

            void Finalize();

            void *AllocateByOrder(s32 order);
            void Free(void *ptr, s32 order);

            size_t GetTotalFreeSize() const;
            size_t GetAllocatableSizeMax() const;
            void Dump() const;

            s32 GetOrderFromBytes(size_t size) const {
                AMS_ASSERT(this->free_lists != nullptr);
                return this->GetOrderFromBlockCount(this->GetBlockCountFromSize(size));
            }

            size_t GetBytesFromOrder(s32 order) const {
                AMS_ASSERT(this->free_lists != nullptr);
                AMS_ASSERT(0 <= order);
                AMS_ASSERT(order < this->GetOrderMax());
                return (this->GetBlockSize() << order);
            }

            s32 GetOrderMax() const {
                AMS_ASSERT(this->free_lists != nullptr);
                return this->order_max;
            }

            size_t GetBlockSize() const {
                AMS_ASSERT(this->free_lists != nullptr);
                return this->block_size;
            }

            s32 GetPageBlockCountMax() const {
                AMS_ASSERT(this->free_lists != nullptr);
                return 1 << this->GetOrderMax();
            }

            size_t GetPageSizeMax() const {
                AMS_ASSERT(this->free_lists != nullptr);
                return this->GetPageBlockCountMax() * this->GetBlockSize();
            }
        private:
            void DivideBuddies(PageEntry *page_entry, s32 required_order, s32 chosen_order);
            void JoinBuddies(PageEntry *page_entry, s32 order);
            PageEntry *GetBuddy(PageEntry *page_entry, s32 order);
            PageEntry *GetFreePageEntry(s32 order);

            s32 GetOrderFromBlockCount(s32 block_count) const;

            s32 GetBlockCountFromSize(size_t size) const {
                const size_t bsize = this->GetBlockSize();
                return static_cast<s32>(util::AlignUp(size, bsize) / bsize);
            }

            uintptr_t GetAddressFromPageEntry(const PageEntry &page_entry) const {
                const uintptr_t address = reinterpret_cast<uintptr_t>(std::addressof(page_entry));
                AMS_ASSERT(this->heap_start <= address);
                AMS_ASSERT(address < this->heap_start + this->heap_size);
                AMS_ASSERT(util::IsAligned(address - this->heap_start, this->GetBlockSize()));
                return address;
            }

            PageEntry *GetPageEntryFromAddress(uintptr_t address) const {
                AMS_ASSERT(this->heap_start <= address);
                AMS_ASSERT(address < this->heap_start + this->heap_size);
                return reinterpret_cast<PageEntry *>(this->heap_start + util::AlignDown(address - this->heap_start, this->GetBlockSize()));
            }

            s32 GetIndexFromPageEntry(const PageEntry &page_entry) const {
                const uintptr_t address = reinterpret_cast<uintptr_t>(std::addressof(page_entry));
                AMS_ASSERT(this->heap_start <= address);
                AMS_ASSERT(address < this->heap_start + this->heap_size);
                AMS_ASSERT(util::IsAligned(address - this->heap_start, this->GetBlockSize()));
                return static_cast<s32>((address - this->heap_start) / this->GetBlockSize());
            }

            bool IsAlignedToOrder(const PageEntry *page_entry, s32 order) const {
                return util::IsAligned(GetIndexFromPageEntry(*page_entry), GetBlockCountFromOrder(order));
            }
    };

}
