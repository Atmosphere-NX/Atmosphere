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

namespace ams::kern {

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
                    constexpr Impl() : heap(), page_reference_counts(), metadata_region(), pool(), next(), prev() { /* ... */ }

                    size_t Initialize(const KMemoryRegion *region, Pool pool, KVirtualAddress metadata_region, KVirtualAddress metadata_region_end);

                    constexpr ALWAYS_INLINE void SetNext(Impl *n) { this->next = n; }
                    constexpr ALWAYS_INLINE void SetPrev(Impl *n) { this->prev = n; }
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
        public:
            constexpr KMemoryManager()
                : pool_locks(), pool_managers_head(), pool_managers_tail(), managers(), num_managers(), optimized_process_ids(), has_optimized_process()
            {
                /* ... */
            }

            void Initialize(KVirtualAddress metadata_region, size_t metadata_region_size);
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
