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
#include <stratosphere.hpp>

#ifdef ATMOSPHERE_OS_HORIZON
    #include "os_address_space_allocator_impl.os.horizon.hpp"
#else
    #error "Unknown OS for AddressSpaceAllocatorImpl"
#endif

namespace ams::os::impl {

    class AddressSpaceAllocator {
        NON_COPYABLE(AddressSpaceAllocator);
        NON_MOVEABLE(AddressSpaceAllocator);
        private:
            InternalCriticalSection cs;
            uintptr_t start_page;
            uintptr_t end_page;
            size_t guard_page_count;
        private:
            bool GetNextNonOverlappedNodeUnsafe(uintptr_t page, size_t page_num);
        public:
            AddressSpaceAllocator(uintptr_t sa, uintptr_t ea, size_t gsz);

            void *AllocateSpace(size_t size, size_t align);
            bool CheckGuardSpace(uintptr_t address, size_t size, size_t guard_size);
    };

}
