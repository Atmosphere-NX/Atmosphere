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
                    u8 buffer[PageSize];
            };
            static_assert(sizeof(PageBuffer) == PageSize);
        private:
            KSpinLock lock;
            KPageBitmap page_bitmap;
            size_t used;
            size_t peak;
            size_t count;
            KVirtualAddress address;
            size_t size;
        public:
            KDynamicPageManager() : lock(), page_bitmap(), used(), peak(), count(), address(), size() { /* ... */ }

            Result Initialize(KVirtualAddress memory, size_t sz) {
                /* We need to have positive size. */
                R_UNLESS(sz > 0, svc::ResultOutOfMemory());

                /* Calculate management overhead. */
                const size_t management_size  = KPageBitmap::CalculateManagementOverheadSize(sz / sizeof(PageBuffer));
                const size_t allocatable_size = sz - management_size;

                /* Set tracking fields. */
                this->address = memory;
                this->size    = util::AlignDown(allocatable_size, sizeof(PageBuffer));
                this->count   = allocatable_size / sizeof(PageBuffer);
                R_UNLESS(this->count > 0, svc::ResultOutOfMemory());

                /* Clear the management region. */
                u64 *management_ptr = GetPointer<u64>(this->address + allocatable_size);
                std::memset(management_ptr, 0, management_size);

                /* Initialize the bitmap. */
                this->page_bitmap.Initialize(management_ptr, this->count);

                /* Free the pages to the bitmap. */
                std::memset(GetPointer<PageBuffer>(this->address), 0, this->count * sizeof(PageBuffer));
                for (size_t i = 0; i < this->count; i++) {
                    this->page_bitmap.SetBit(i);
                }

                return ResultSuccess();
            }

            constexpr KVirtualAddress GetAddress() const { return this->address; }
            constexpr size_t GetSize() const { return this->size; }
            constexpr size_t GetUsed() const { return this->used; }
            constexpr size_t GetPeak() const { return this->peak; }
            constexpr size_t GetCount() const { return this->count; }

            PageBuffer *Allocate() {
                /* Take the lock. */
                KScopedInterruptDisable di;
                KScopedSpinLock lk(this->lock);

                /* Find a random free block. */
                ssize_t soffset = this->page_bitmap.FindFreeBlock(true);
                if (AMS_UNLIKELY(soffset < 0)) {
                    return nullptr;
                }

                const size_t offset = static_cast<size_t>(soffset);

                /* Update our tracking. */
                this->page_bitmap.ClearBit(offset);
                this->peak = std::max(this->peak, (++this->used));

                return GetPointer<PageBuffer>(this->address) + offset;
            }

            void Free(PageBuffer *pb) {
                /* Take the lock. */
                KScopedInterruptDisable di;
                KScopedSpinLock lk(this->lock);

                /* Set the bit for the free page. */
                size_t offset = (reinterpret_cast<uintptr_t>(pb) - GetInteger(this->address)) / sizeof(PageBuffer);
                this->page_bitmap.SetBit(offset);

                /* Decrement our used count. */
                --this->used;
            }
    };

}
