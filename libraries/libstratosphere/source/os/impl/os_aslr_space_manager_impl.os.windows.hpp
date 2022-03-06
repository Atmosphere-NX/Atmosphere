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

    class AslrSpaceManagerWindowsImpl {
        NON_COPYABLE(AslrSpaceManagerWindowsImpl);
        NON_MOVEABLE(AslrSpaceManagerWindowsImpl);
        public:
            constexpr AslrSpaceManagerWindowsImpl() = default;

            static constexpr ALWAYS_INLINE const AddressSpaceAllocatorForbiddenRegion *GetForbiddenRegions() {
                return nullptr;
            }

            static constexpr ALWAYS_INLINE size_t GetForbiddenRegionCount() {
                return 0;
            }

            static constexpr ALWAYS_INLINE u64 GetHeapSpaceBeginAddress() {
                return 8_GB;
            }

            static constexpr ALWAYS_INLINE u64 GetHeapSpaceSize() {
                return 4_GB;
            }

            static constexpr ALWAYS_INLINE u64 GetAliasSpaceBeginAddress() {
                return 60_GB;
            }

            static constexpr ALWAYS_INLINE u64 GetAliasSpaceSize() {
                return 4_GB;
            }

            static constexpr ALWAYS_INLINE u64 GetAslrSpaceBeginAddress() {
                return 2_MB;
            }

            static constexpr ALWAYS_INLINE u64 GetAslrSpaceEndAddress() {
                return 64_GB;
            }
    };

    using AslrSpaceManagerImpl = AslrSpaceManagerWindowsImpl;

}
