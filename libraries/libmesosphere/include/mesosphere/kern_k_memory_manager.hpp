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
                Pool_Secure = Pool_System,
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
                public:
                    static size_t CalculateManagementOverheadSize(size_t region_size);

                    static constexpr size_t CalculateOptimizedProcessOverheadSize(size_t region_size) {
                        return (util::AlignUp((region_size / PageSize), BITSIZEOF(u64)) / BITSIZEOF(u64)) * sizeof(u64);
                    }
                private:
                    KPageHeap m_heap;
                    RefCount *m_page_reference_counts;
                    KVirtualAddress m_management_region;
                    Pool m_pool;
                    Impl *m_next;
                    Impl *m_prev;
                public:
                    Impl() : m_heap(), m_page_reference_counts(), m_management_region(), m_pool(), m_next(), m_prev() { /* ... */ }

                    size_t Initialize(uintptr_t address, size_t size, KVirtualAddress management, KVirtualAddress management_end, Pool p);

                    KVirtualAddress AllocateBlock(s32 index, bool random) { return m_heap.AllocateBlock(index, random); }
                    void Free(KVirtualAddress addr, size_t num_pages) { m_heap.Free(addr, num_pages); }

                    void UpdateUsedHeapSize() { m_heap.UpdateUsedSize(); }

                    void InitializeOptimizedMemory() { std::memset(GetVoidPointer(m_management_region), 0, CalculateOptimizedProcessOverheadSize(m_heap.GetSize())); }

                    void TrackUnoptimizedAllocation(KVirtualAddress block, size_t num_pages);
                    void TrackOptimizedAllocation(KVirtualAddress block, size_t num_pages);

                    bool ProcessOptimizedAllocation(KVirtualAddress block, size_t num_pages, u8 fill_pattern);

                    constexpr Pool GetPool() const { return m_pool; }
                    constexpr size_t GetSize() const { return m_heap.GetSize(); }
                    constexpr KVirtualAddress GetEndAddress() const { return m_heap.GetEndAddress(); }

                    size_t GetFreeSize() const { return m_heap.GetFreeSize(); }

                    void DumpFreeList() const { return m_heap.DumpFreeList(); }

                    constexpr size_t GetPageOffset(KVirtualAddress address)      const { return m_heap.GetPageOffset(address); }
                    constexpr size_t GetPageOffsetToEnd(KVirtualAddress address) const { return m_heap.GetPageOffsetToEnd(address); }

                    constexpr void SetNext(Impl *n) { m_next = n; }
                    constexpr void SetPrev(Impl *n) { m_prev = n; }
                    constexpr Impl *GetNext() const { return m_next; }
                    constexpr Impl *GetPrev() const { return m_prev; }

                    void OpenFirst(KVirtualAddress address, size_t num_pages) {
                        size_t index = this->GetPageOffset(address);
                        const size_t end = index + num_pages;
                        while (index < end) {
                            const RefCount ref_count = (++m_page_reference_counts[index]);
                            MESOSPHERE_ABORT_UNLESS(ref_count == 1);

                            index++;
                        }
                    }

                    void Open(KVirtualAddress address, size_t num_pages) {
                        size_t index = this->GetPageOffset(address);
                        const size_t end = index + num_pages;
                        while (index < end) {
                            const RefCount ref_count = (++m_page_reference_counts[index]);
                            MESOSPHERE_ABORT_UNLESS(ref_count > 1);

                            index++;
                        }
                    }

                    void Close(KVirtualAddress address, size_t num_pages) {
                        size_t index = this->GetPageOffset(address);
                        const size_t end = index + num_pages;

                        size_t free_start = 0;
                        size_t free_count = 0;
                        while (index < end) {
                            MESOSPHERE_ABORT_UNLESS(m_page_reference_counts[index] > 0);
                            const RefCount ref_count = (--m_page_reference_counts[index]);

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
                                    this->Free(m_heap.GetAddress() + free_start * PageSize, free_count);
                                    free_count = 0;
                                }
                            }

                            index++;
                        }

                        if (free_count > 0) {
                            this->Free(m_heap.GetAddress() + free_start * PageSize, free_count);
                        }
                    }
            };
        private:
            KLightLock m_pool_locks[Pool_Count];
            Impl *m_pool_managers_head[Pool_Count];
            Impl *m_pool_managers_tail[Pool_Count];
            Impl m_managers[MaxManagerCount];
            size_t m_num_managers;
            u64 m_optimized_process_ids[Pool_Count];
            bool m_has_optimized_process[Pool_Count];
        private:
            Impl &GetManager(KVirtualAddress address) {
                return m_managers[KMemoryLayout::GetVirtualLinearRegion(address).GetAttributes()];
            }

            constexpr Impl *GetFirstManager(Pool pool, Direction dir) {
                return dir == Direction_FromBack ? m_pool_managers_tail[pool] : m_pool_managers_head[pool];
            }

            constexpr Impl *GetNextManager(Impl *cur, Direction dir) {
                if (dir == Direction_FromBack) {
                    return cur->GetPrev();
                } else {
                    return cur->GetNext();
                }
            }

            Result AllocatePageGroupImpl(KPageGroup *out, size_t num_pages, Pool pool, Direction dir, bool unoptimized, bool random);
        public:
            KMemoryManager()
                : m_pool_locks(), m_pool_managers_head(), m_pool_managers_tail(), m_managers(), m_num_managers(), m_optimized_process_ids(), m_has_optimized_process()
            {
                /* ... */
            }

            NOINLINE void Initialize(KVirtualAddress management_region, size_t management_region_size);

            NOINLINE Result InitializeOptimizedMemory(u64 process_id, Pool pool);
            NOINLINE void FinalizeOptimizedMemory(u64 process_id, Pool pool);

            NOINLINE KVirtualAddress AllocateAndOpenContinuous(size_t num_pages, size_t align_pages, u32 option);
            NOINLINE Result AllocateAndOpen(KPageGroup *out, size_t num_pages, u32 option);
            NOINLINE Result AllocateAndOpenForProcess(KPageGroup *out, size_t num_pages, u32 option, u64 process_id, u8 fill_pattern);

            void Open(KVirtualAddress address, size_t num_pages) {
                /* Repeatedly open references until we've done so for all pages. */
                while (num_pages) {
                    auto &manager = this->GetManager(address);
                    const size_t cur_pages = std::min(num_pages, manager.GetPageOffsetToEnd(address));

                    {
                        KScopedLightLock lk(m_pool_locks[manager.GetPool()]);
                        manager.Open(address, cur_pages);
                    }

                    num_pages -= cur_pages;
                    address += cur_pages * PageSize;
                }
            }

            void Close(KVirtualAddress address, size_t num_pages) {
                /* Repeatedly close references until we've done so for all pages. */
                while (num_pages) {
                    auto &manager = this->GetManager(address);
                    const size_t cur_pages = std::min(num_pages, manager.GetPageOffsetToEnd(address));

                    {
                        KScopedLightLock lk(m_pool_locks[manager.GetPool()]);
                        manager.Close(address, cur_pages);
                    }

                    num_pages -= cur_pages;
                    address += cur_pages * PageSize;
                }
            }

            size_t GetSize() {
                size_t total = 0;
                for (size_t i = 0; i < m_num_managers; i++) {
                    total += m_managers[i].GetSize();
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

            size_t GetFreeSize() {
                size_t total = 0;
                for (size_t i = 0; i < m_num_managers; i++) {
                    KScopedLightLock lk(m_pool_locks[m_managers[i].GetPool()]);
                    total += m_managers[i].GetFreeSize();
                }
                return total;
            }

            size_t GetFreeSize(Pool pool) {
                KScopedLightLock lk(m_pool_locks[pool]);

                constexpr Direction GetSizeDirection = Direction_FromFront;
                size_t total = 0;
                for (auto *manager = this->GetFirstManager(pool, GetSizeDirection); manager != nullptr; manager = this->GetNextManager(manager, GetSizeDirection)) {
                    total += manager->GetFreeSize();
                }
                return total;
            }

            void DumpFreeList(Pool pool) {
                KScopedLightLock lk(m_pool_locks[pool]);

                constexpr Direction DumpDirection = Direction_FromFront;
                for (auto *manager = this->GetFirstManager(pool, DumpDirection); manager != nullptr; manager = this->GetNextManager(manager, DumpDirection)) {
                    manager->DumpFreeList();
                }
            }
        public:
            static size_t CalculateManagementOverheadSize(size_t region_size) {
                return Impl::CalculateManagementOverheadSize(region_size);
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
