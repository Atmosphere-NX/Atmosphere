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
#include "os_address_space_allocator_types.hpp"
#include "os_rng_manager.hpp"

namespace ams::os::impl {

    AddressSpaceAllocator::AddressSpaceAllocator(uintptr_t sa, uintptr_t ea, size_t gsz) : cs() {
        /* Check preconditions. */
        AMS_ASSERT(sa >= gsz);
        AMS_ASSERT(ea + gsz >= ea);

        this->guard_page_count = util::DivideUp(gsz, MemoryPageSize);
        this->start_page       = sa / MemoryPageSize;
        this->end_page         = (ea / MemoryPageSize) + this->guard_page_count;
    }

    bool AddressSpaceAllocator::GetNextNonOverlappedNodeUnsafe(uintptr_t page, size_t page_num) {
        AMS_ASSERT(page < (page + page_num));

        return CheckFreeSpace(page * MemoryPageSize, page_num * MemoryPageSize);
    }

    void *AddressSpaceAllocator::AllocateSpace(size_t size, size_t align) {
        /* Check pre-conditions. */
        AMS_ASSERT(align > 0);

        /* Determine the page count. */
        const size_t page_count = util::DivideUp(size, MemoryPageSize);

        /* Determine the alignment pages. */
        AMS_ASSERT(util::IsAligned(align, MemoryPageSize));
        const size_t align_page_count = align / MemoryPageSize;

        /* Check the page count. */
        if (page_count > this->end_page - this->guard_page_count) {
            return nullptr;
        }

        /* Determine the range to look in. */
        const uintptr_t rand_start = util::DivideUp(this->start_page + this->guard_page_count, align_page_count);
        const uintptr_t rand_end   = (this->end_page - page_count - this->guard_page_count) / align_page_count;

        /* Check that we can find a space. */
        if (rand_start > rand_end) {
            return nullptr;
        }

        /* Try to find space up to 512 times. */
        for (size_t i = 0; i < 512; ++i) {
            /* Acquire exclusive access before doing calculations. */
            std::scoped_lock lk(this->cs);

            /* Determine a random page. */
            const uintptr_t target = ((GetRngManager().GenerateRandomU64() % (rand_end - rand_start + 1)) + rand_start) * align_page_count;
            AMS_ASSERT(this->start_page <= target - this->guard_page_count && target + page_count + this->guard_page_count <= this->end_page);

            /* If the page is valid, use it. */
            if (GetNextNonOverlappedNodeUnsafe(target - this->guard_page_count, page_count + 2 * this->guard_page_count)) {
                return reinterpret_cast<void *>(target * MemoryPageSize);
            }
        }

        /* We failed to find space. */
        return nullptr;
    }

    bool AddressSpaceAllocator::CheckGuardSpace(uintptr_t address, size_t size, size_t guard_size) {
        return CheckFreeSpace(address - guard_size, guard_size) && CheckFreeSpace(address + size, guard_size);
    }

}
