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
#include <stratosphere.hpp>
#include "os_address_space_allocator.hpp"
#include "os_rng_manager.hpp"

namespace ams::os::impl {

    template<std::unsigned_integral AddressType, std::unsigned_integral SizeType>
    AddressSpaceAllocatorBase<AddressType, SizeType>::AddressSpaceAllocatorBase(u64 start_address, u64 end_address, SizeType guard_size, const AddressSpaceAllocatorForbiddenRegion *forbidden_regions, size_t num_forbidden_regions) : m_critical_section(), m_forbidden_region_count(0) {
        /* Check pre-conditions. */
        AMS_ASSERT(start_address >= guard_size);
        AMS_ASSERT(end_address + guard_size >= end_address);

        /* Set member variables. */
        m_guard_page_count = util::DivideUp(guard_size, MemoryPageSize);
        m_start_page       = start_address / MemoryPageSize;
        m_end_page         = (end_address / MemoryPageSize) + m_guard_page_count;

        /* Check forbidden region count. */
        AMS_ASSERT(num_forbidden_regions <= MaxForbiddenRegions);

        /* Set forbidden regions. */
        for (size_t i = 0; i < num_forbidden_regions; ++i) {
            if (const auto &region = forbidden_regions[i]; region.size > 0) {
                /* Check region is valid. */
                AMS_ASSERT(util::IsAligned(region.address, MemoryPageSize));
                AMS_ASSERT(util::IsAligned(region.size, MemoryPageSize));
                AMS_ASSERT((region.address / MemoryPageSize) >= m_guard_page_count);
                AMS_ASSERT(region.address < region.address + region.size);

                /* Set region. */
                const auto idx = m_forbidden_region_count++;
                m_forbidden_region_start_pages[idx] = (region.address / MemoryPageSize) - m_guard_page_count;
                m_forbidden_region_end_pages[idx]   = ((region.address + region.size) / MemoryPageSize) + m_guard_page_count;
            }
        }
    }

    template<std::unsigned_integral AddressType, std::unsigned_integral SizeType>
    bool AddressSpaceAllocatorBase<AddressType, SizeType>::CheckGuardSpace(AddressType address, SizeType size, SizeType guard_size) {
        return this->CheckFreeSpace(address - guard_size, guard_size) && this->CheckFreeSpace(address + size, guard_size);
    }

    template<std::unsigned_integral AddressType, std::unsigned_integral SizeType>
    bool AddressSpaceAllocatorBase<AddressType, SizeType>::GetNextNonOverlappedNodeUnsafe(AddressType page, SizeType page_count) {
        /* Check pre-conditions. */
        AMS_ASSERT(page < page + page_count);

        return this->CheckFreeSpace(page * MemoryPageSize, page_count * MemoryPageSize);
    }

    template<std::unsigned_integral AddressType, std::unsigned_integral SizeType>
    AddressType AddressSpaceAllocatorBase<AddressType, SizeType>::AllocateSpace(SizeType size, SizeType align, SizeType align_offset) {
        /* Check pre-conditions. */
        AMS_ASSERT(align > 0);
        AMS_ASSERT((align_offset & ~(align - 1)) == 0);
        AMS_ASSERT(util::IsAligned(align_offset, MemoryPageSize));
        AMS_ASSERT(util::IsAligned(align, MemoryPageSize));

        /* Determine the page count. */
        const SizeType page_count = util::DivideUp(size, MemoryPageSize);

        /* Determine alignment page counts. */
        const SizeType align_offset_page_count = align_offset / MemoryPageSize;
        const SizeType align_page_count        = align / MemoryPageSize;

        /* Check page counts. */
        if (page_count + align_offset_page_count > m_end_page - m_guard_page_count) {
            return 0;
        }

        /* Determine the range to look in. */
        const AddressType rand_start = (align_offset_page_count <= m_start_page + m_guard_page_count) ? util::DivideUp(m_start_page + m_guard_page_count - align_offset_page_count, align_page_count) : 0;
        const AddressType rand_end   = (m_end_page - page_count - align_offset_page_count - m_guard_page_count) / align_page_count;

        /* Check that we can find a space. */
        if (rand_start > rand_end) {
            return 0;
        }

        /* Try to find a space up to 512 times. */
        for (size_t i = 0; i < 512; ++i) {
            /* Acquire exclusive access before doing calculations. */
            std::scoped_lock lk(m_critical_section);

            /* Determine a random page. */
            const AddressType target = (((GetRngManager().GenerateRandomU64() % (rand_end - rand_start + 1)) + rand_start) * align_page_count) + align_offset_page_count;
            AMS_ASSERT(m_start_page <= target - m_guard_page_count && target + page_count + m_guard_page_count <= m_end_page);

            /* Check that the page is not forbidden. */
            bool forbidden = false;
            for (size_t j = 0; j < m_forbidden_region_count; ++j) {
                if (m_forbidden_region_start_pages[j] < target + page_count && target < m_forbidden_region_end_pages[j]) {
                    forbidden = true;
                    break;
                }
            }

            /* If the page is valid, use it. */
            if (!forbidden && this->GetNextNonOverlappedNodeUnsafe(target - m_guard_page_count, page_count + 2 * m_guard_page_count)) {
                return target * MemoryPageSize;
            }
        }

        /* We failed to find space. */
        return 0;
    }

    /* Instantiate template. */
    /* TODO: instantiate <u64, u64> on 32-bit? */
    template class AddressSpaceAllocatorBase<uintptr_t, size_t>;

}
