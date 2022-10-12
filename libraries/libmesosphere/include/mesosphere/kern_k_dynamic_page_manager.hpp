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
#pragma once
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_k_spin_lock.hpp>
#include <mesosphere/kern_k_slab_heap.hpp>
#include <mesosphere/kern_k_page_group.hpp>
#include <mesosphere/kern_k_memory_block.hpp>
#include <mesosphere/kern_k_page_bitmap.hpp>
#include <mesosphere/kern_select_interrupt_manager.hpp>

namespace ams::kern {

    class KDynamicPageManager {
        public:
            class PageBuffer {
                private:
                    u8 m_buffer[PageSize];
            };
            static_assert(sizeof(PageBuffer) == PageSize);
        private:
            KSpinLock m_lock;
            KPageBitmap m_page_bitmap;
            size_t m_used;
            size_t m_peak;
            size_t m_count;
            KVirtualAddress m_address;
            KVirtualAddress m_aligned_address;
            size_t m_size;
        public:
            KDynamicPageManager() : m_lock(), m_page_bitmap(), m_used(), m_peak(), m_count(), m_address(Null<KVirtualAddress>), m_aligned_address(Null<KVirtualAddress>), m_size() { /* ... */ }

            Result Initialize(KVirtualAddress memory, size_t size, size_t align) {
                /* We need to have positive size. */
                R_UNLESS(size > 0, svc::ResultOutOfMemory());

                /* Set addresses. */
                m_address         = memory;
                m_aligned_address = util::AlignDown(GetInteger(memory), align);

                /* Calculate extents. */
                const size_t managed_size  = m_address + size - m_aligned_address;
                const size_t overhead_size = util::AlignUp(KPageBitmap::CalculateManagementOverheadSize(managed_size / sizeof(PageBuffer)), sizeof(PageBuffer));
                R_UNLESS(overhead_size < size, svc::ResultOutOfMemory());

                /* Set tracking fields. */
                m_size  = util::AlignDown(size - overhead_size, sizeof(PageBuffer));
                m_count = m_size / sizeof(PageBuffer);

                /* Clear the management region. */
                u64 *management_ptr = GetPointer<u64>(m_address + size - overhead_size);
                std::memset(management_ptr, 0, overhead_size);

                /* Initialize the bitmap. */
                const size_t allocatable_region_size = (GetInteger(m_address) + size - overhead_size) - GetInteger(m_aligned_address);
                MESOSPHERE_ABORT_UNLESS(allocatable_region_size >= sizeof(PageBuffer));

                m_page_bitmap.Initialize(management_ptr, allocatable_region_size / sizeof(PageBuffer));

                /* Free the pages to the bitmap. */
                for (size_t i = 0; i < m_count; i++) {
                    /* Ensure the freed page is all-zero. */
                    cpu::ClearPageToZero(GetPointer<PageBuffer>(m_address) + i);

                    /* Set the bit for the free page. */
                    m_page_bitmap.SetBit((GetInteger(m_address) + (i * sizeof(PageBuffer)) - GetInteger(m_aligned_address)) / sizeof(PageBuffer));
                }

                R_SUCCEED();
            }

            constexpr KVirtualAddress GetAddress() const { return m_address; }
            constexpr size_t GetSize() const { return m_size; }
            constexpr size_t GetUsed() const { return m_used; }
            constexpr size_t GetPeak() const { return m_peak; }
            constexpr size_t GetCount() const { return m_count; }

            PageBuffer *Allocate() {
                /* Take the lock. */
                KScopedInterruptDisable di;
                KScopedSpinLock lk(m_lock);

                /* Find a random free block. */
                ssize_t soffset = m_page_bitmap.FindFreeBlock(true);
                if (AMS_UNLIKELY(soffset < 0)) {
                    return nullptr;
                }

                const size_t offset = static_cast<size_t>(soffset);

                /* Update our tracking. */
                m_page_bitmap.ClearBit(offset);
                m_peak = std::max(m_peak, (++m_used));

                return GetPointer<PageBuffer>(m_aligned_address) + offset;
            }

            PageBuffer *Allocate(size_t count) {
                /* Take the lock. */
                KScopedInterruptDisable di;
                KScopedSpinLock lk(m_lock);

                /* Find a random free block. */
                ssize_t soffset = m_page_bitmap.FindFreeRange(count);
                if (AMS_UNLIKELY(soffset < 0)) {
                    return nullptr;
                }

                const size_t offset = static_cast<size_t>(soffset);

                /* Update our tracking. */
                m_page_bitmap.ClearRange(offset, count);
                m_used += count;
                m_peak = std::max(m_peak, m_used);

                return GetPointer<PageBuffer>(m_aligned_address) + offset;
            }

            void Free(PageBuffer *pb) {
                /* Ensure all pages in the heap are zero. */
                cpu::ClearPageToZero(pb);

                /* Take the lock. */
                KScopedInterruptDisable di;
                KScopedSpinLock lk(m_lock);

                /* Set the bit for the free page. */
                size_t offset = (reinterpret_cast<uintptr_t>(pb) - GetInteger(m_aligned_address)) / sizeof(PageBuffer);
                m_page_bitmap.SetBit(offset);

                /* Decrement our used count. */
                --m_used;
            }
    };

}
