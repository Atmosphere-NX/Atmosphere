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
#include <mesosphere/kern_k_light_lock.hpp>
#include <mesosphere/kern_k_memory_layout.hpp>
#include <mesosphere/kern_k_page_heap.hpp>

namespace ams::kern {

    class KPageGroup;

    class KMemoryManager {
        public:
            enum Pool {
                Pool_Application     = 0,
                Pool_Applet          = 1,
                Pool_System          = 2,
                Pool_SystemNonSecure = 3,

                Pool_Count,

                Pool_Shift = 4,
                Pool_Mask  = (0xF << Pool_Shift),

                /* Aliases. */
                Pool_Unsafe = Pool_Application,
            };

            enum Direction {
                Direction_FromFront = 0,
                Direction_FromBack  = 1,

                Direction_Shift = 0,
                Direction_Mask  = (0xF << Direction_Shift),
            };

            static constexpr size_t MaxManagerCount = 10;
        private:
            class Impl {
                private:
                    using RefCount = u16;
                private:
                    KPageHeap heap;
                    RefCount *page_reference_counts;
                    KVirtualAddress metadata_region;
                    Pool pool;
                    Impl *next;
                    Impl *prev;
                public:
                    Impl() : heap(), page_reference_counts(), metadata_region(), pool(), next(), prev() { /* ... */ }

                    size_t Initialize(const KMemoryRegion *region, Pool pool, KVirtualAddress metadata_region, KVirtualAddress metadata_region_end);

                    KVirtualAddress AllocateBlock(s32 index, bool random) { return this->heap.AllocateBlock(index, random); }
                    void Free(KVirtualAddress addr, size_t num_pages) { this->heap.Free(addr, num_pages); }

                    void TrackAllocationForOptimizedProcess(KVirtualAddress block, size_t num_pages);

                    constexpr size_t GetSize() const { return this->heap.GetSize(); }
                    constexpr KVirtualAddress GetEndAddress() const { return this->heap.GetEndAddress(); }

                    constexpr void SetNext(Impl *n) { this->next = n; }
                    constexpr void SetPrev(Impl *n) { this->prev = n; }
                    constexpr Impl *GetNext() const { return this->next; }
                    constexpr Impl *GetPrev() const { return this->prev; }

                    void Open(KLightLock *pool_locks, KVirtualAddress address, size_t num_pages) {
                        KScopedLightLock lk(pool_locks[this->pool]);

                        size_t index = this->heap.GetPageOffset(address);
                        const size_t end = index + num_pages;
                        while (index < end) {
                            const RefCount ref_count = (++this->page_reference_counts[index]);
                            MESOSPHERE_ABORT_UNLESS(ref_count > 0);

                            index++;
                        }
                    }

                    void Close(KLightLock *pool_locks, KVirtualAddress address, size_t num_pages) {
                        KScopedLightLock lk(pool_locks[this->pool]);

                        size_t index = this->heap.GetPageOffset(address);
                        const size_t end = index + num_pages;

                        size_t free_start = 0;
                        size_t free_count = 0;
                        while (index < end) {
                            MESOSPHERE_ABORT_UNLESS(this->page_reference_counts[index] > 0);
                            const RefCount ref_count = (--this->page_reference_counts[index]);

                            /* Keep track of how many zero refcounts we see in a row, to minimize calls to free. */
                            if (ref_count == 0) {
                                if (free_count > 0) {
                                    free_count++;
                                } else {
                                    free_start = index;
                                    free_count = 1;
                                }
                            } else {
                                if (free_count > 0) {
                                    this->Free(this->heap.GetAddress() + free_start * PageSize, free_count);
                                    free_count = 0;
                                }
                            }

                            index++;
                        }

                        if (free_count > 0) {
                            this->Free(this->heap.GetAddress() + free_start * PageSize, free_count);
                        }
                    }
                public:
                    static size_t CalculateMetadataOverheadSize(size_t region_size);
            };
        private:
            KLightLock pool_locks[Pool_Count];
            Impl *pool_managers_head[Pool_Count];
            Impl *pool_managers_tail[Pool_Count];
            Impl managers[MaxManagerCount];
            size_t num_managers;
            u64 optimized_process_ids[Pool_Count];
            bool has_optimized_process[Pool_Count];
        private:
            Impl &GetManager(KVirtualAddress address) {
                return this->managers[KMemoryLayout::GetVirtualLinearRegion(address).GetAttributes()];
            }

            constexpr Impl *GetFirstManager(Pool pool, Direction dir) {
                return dir == Direction_FromBack ? this->pool_managers_tail[pool] : this->pool_managers_head[pool];
            }

            constexpr Impl *GetNextManager(Impl *cur, Direction dir) {
                if (dir == Direction_FromBack) {
                    return cur->GetPrev();
                } else {
                    return cur->GetNext();
                }
            }

            Result AllocatePageGroupImpl(KPageGroup *out, size_t num_pages, Pool pool, Direction dir, bool optimize, bool random);
        public:
            KMemoryManager()
                : pool_locks(), pool_managers_head(), pool_managers_tail(), managers(), num_managers(), optimized_process_ids(), has_optimized_process()
            {
                /* ... */
            }

            NOINLINE void Initialize(KVirtualAddress metadata_region, size_t metadata_region_size);

            NOINLINE KVirtualAddress AllocateContinuous(size_t num_pages, size_t align_pages, u32 option);
            NOINLINE Result Allocate(KPageGroup *out, size_t num_pages, u32 option);

            void Open(KVirtualAddress address, size_t num_pages) {
                /* Repeatedly open references until we've done so for all pages. */
                while (num_pages) {
                    auto &manager = this->GetManager(address);
                    const size_t cur_pages = std::min(num_pages, (manager.GetEndAddress() - address) / PageSize);
                    manager.Open(this->pool_locks, address, cur_pages);
                    num_pages -= cur_pages;
                    address += cur_pages * PageSize;
                }
            }

            void Close(KVirtualAddress address, size_t num_pages) {
                /* Repeatedly close references until we've done so for all pages. */
                while (num_pages) {
                    auto &manager = this->GetManager(address);
                    const size_t cur_pages = std::min(num_pages, (manager.GetEndAddress() - address) / PageSize);
                    manager.Close(this->pool_locks, address, cur_pages);
                    num_pages -= cur_pages;
                    address += cur_pages * PageSize;
                }
            }

            size_t GetSize() {
                size_t total = 0;
                for (size_t i = 0; i < this->num_managers; i++) {
                    total += this->managers[i].GetSize();
                }
                return total;
            }

            size_t GetSize(Pool pool) {
                constexpr Direction GetSizeDirection = Direction_FromFront;
                size_t total = 0;
                for (auto *manager = this->GetFirstManager(pool, GetSizeDirection); manager != nullptr; manager = this->GetNextManager(manager, GetSizeDirection)) {
                    total += manager->GetSize();
                }
                return total;
            }
        public:
            static size_t CalculateMetadataOverheadSize(size_t region_size) {
                return Impl::CalculateMetadataOverheadSize(region_size);
            }

            static constexpr ALWAYS_INLINE u32 EncodeOption(Pool pool, Direction dir) {
                return (pool << Pool_Shift) | (dir << Direction_Shift);
            }

            static constexpr ALWAYS_INLINE Pool GetPool(u32 option) {
                return static_cast<Pool>((option & Pool_Mask) >> Pool_Shift);
            }

            static constexpr ALWAYS_INLINE Direction GetDirection(u32 option) {
                return static_cast<Direction>((option & Direction_Mask) >> Direction_Shift);
            }

            static constexpr ALWAYS_INLINE std::tuple<Pool, Direction> DecodeOption(u32 option) {
                return std::make_tuple(GetPool(option), GetDirection(option));
            }
    };

}
