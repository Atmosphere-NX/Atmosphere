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
#include <stratosphere.hpp>
#include "os_address_space_allocator_forbidden_region.hpp"

namespace ams::os::impl {

    template<std::unsigned_integral AddressType, std::unsigned_integral SizeType>
    class AddressSpaceAllocatorBase {
        NON_COPYABLE(AddressSpaceAllocatorBase);
        NON_MOVEABLE(AddressSpaceAllocatorBase);
        private:
            static constexpr size_t MaxForbiddenRegions = 2;
        private:
            InternalCriticalSection m_critical_section;
            AddressType m_start_page;
            AddressType m_end_page;
            SizeType m_guard_page_count;
            AddressType m_forbidden_region_start_pages[MaxForbiddenRegions];
            AddressType m_forbidden_region_end_pages[MaxForbiddenRegions];
            size_t m_forbidden_region_count;
        public:
            AddressSpaceAllocatorBase(u64 start_address, u64 end_address, SizeType guard_size, const AddressSpaceAllocatorForbiddenRegion *forbidden_regions, size_t num_forbidden_regions);

            AddressType AllocateSpace(SizeType size, SizeType align, SizeType align_offset);

            bool CheckGuardSpace(AddressType address, SizeType size, SizeType guard_size);
        private:
            bool GetNextNonOverlappedNodeUnsafe(AddressType page, SizeType page_count);
        public:
            virtual bool CheckFreeSpace(AddressType address, SizeType size) = 0;
    };

}

#ifdef ATMOSPHERE_OS_HORIZON
    #include "os_address_space_allocator_impl.os.horizon.hpp"
#else
    #error "Unknown OS for AddressSpaceAllocator"
#endif
