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

#include "os_address_space_allocator_types.hpp"

#ifdef ATMOSPHERE_OS_HORIZON
    #include "os_aslr_space_manager_impl.os.horizon.hpp"
#else
    #error "Unknown OS for AslrSpaceManagerImpl"
#endif

namespace ams::os::impl {

    constexpr inline size_t AslrSpaceLargeAlign = 2_MB;
    constexpr inline size_t AslrSpaceGuardSize  = 4 * MemoryPageSize;

    class AslrSpaceManager {
        NON_COPYABLE(AslrSpaceManager);
        NON_MOVEABLE(AslrSpaceManager);
        private:
            AddressSpaceAllocator allocator;
            AslrSpaceManagerImpl impl;
        public:
            AslrSpaceManager() : allocator(impl.GetAslrSpaceBeginAddress(), impl.GetAslrSpaceEndAddress(), AslrSpaceGuardSize) { /* ... */ }

            void *AllocateSpace(size_t size) {
                void *address = this->allocator.AllocateSpace(size, AslrSpaceLargeAlign);
                if (address == nullptr) {
                    address = this->allocator.AllocateSpace(size, MemoryPageSize);
                }
                return address;
            }

            bool CheckGuardSpace(uintptr_t address, size_t size) {
                return this->allocator.CheckGuardSpace(address, size, AslrSpaceGuardSize);
            }
    };

}
