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
            size_t m_size;
        public:
            KDynamicPageManager() : m_lock(), m_page_bitmap(), m_used(), m_peak(), m_count(), m_address(), m_size() { /* ... */ }

            Result Initialize(KVirtualAddress memory, size_t sz) {
                /* We need to have positive size. */
                R_UNLESS(sz > 0, svc::ResultOutOfMemory());

                /* Calculate management overhead. */
                const size_t management_size  = KPageBitmap::CalculateManagementOverheadSize(sz / sizeof(PageBuffer));
                const size_t allocatable_size = sz - management_size;

                /* Set tracking fields. */
                m_address = memory;
                m_size    = util::AlignDown(allocatable_size, sizeof(PageBuffer));
                m_count   = allocatable_size / sizeof(PageBuffer);
                R_UNLESS(m_count > 0, svc::ResultOutOfMemory());

                /* Clear the management region. */
                u64 *management_ptr = GetPointer<u64>(m_address + allocatable_size);
                std::memset(management_ptr, 0, management_size);

                /* Initialize the bitmap. */
                m_page_bitmap.Initialize(management_ptr, m_count);

                /* Free the pages to the bitmap. */
                std::memset(GetPointer<PageBuffer>(m_address), 0, m_count * sizeof(PageBuffer));
                for (size_t i = 0; i < m_count; i++) {
                    m_page_bitmap.SetBit(i);
                }

                return ResultSuccess();
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

                return GetPointer<PageBuffer>(m_address) + offset;
            }

            void Free(PageBuffer *pb) {
                /* Take the lock. */
                KScopedInterruptDisable di;
                KScopedSpinLock lk(m_lock);

                /* Set the bit for the free page. */
                size_t offset = (reinterpret_cast<uintptr_t>(pb) - GetInteger(m_address)) / sizeof(PageBuffer);
                m_page_bitmap.SetBit(offset);

                /* Decrement our used count. */
                --m_used;
            }
    };

}
