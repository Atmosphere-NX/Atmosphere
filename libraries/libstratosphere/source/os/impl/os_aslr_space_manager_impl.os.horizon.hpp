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

    class AslrSpaceManagerHorizonImpl {
        NON_COPYABLE(AslrSpaceManagerHorizonImpl);
        NON_MOVEABLE(AslrSpaceManagerHorizonImpl);
        private:
            static constexpr u64 AslrBase32Bit           = 0x0000200000ul;
            static constexpr u64 AslrSize32Bit           = 0x003FE00000ul;
            static constexpr u64 AslrBase64BitDeprecated = 0x0008000000ul;
            static constexpr u64 AslrSize64BitDeprecated = 0x0078000000ul;
            static constexpr u64 AslrBase64Bit           = 0x0008000000ul;
            static constexpr u64 AslrSize64Bit           = 0x7FF8000000ul;

            static constexpr size_t ForbiddenRegionCount = 2;
        private:
            static u64 GetAslrInfo(svc::InfoType type) {
                u64 value;
                R_ABORT_UNLESS(svc::GetInfo(std::addressof(value), type, svc::PseudoHandle::CurrentProcess, 0));

                static_assert(std::same_as<size_t, uintptr_t>);
                AMS_ASSERT(value <= std::numeric_limits<size_t>::max());

                return static_cast<u64>(value & std::numeric_limits<size_t>::max());
            }
        private:
            AddressSpaceAllocatorForbiddenRegion m_forbidden_regions[ForbiddenRegionCount];
        public:
            AslrSpaceManagerHorizonImpl() {
                m_forbidden_regions[0] = { .address = GetHeapSpaceBeginAddress(),  .size = GetHeapSpaceSize()  };
                m_forbidden_regions[1] = { .address = GetAliasSpaceBeginAddress(), .size = GetAliasSpaceSize() };
            }

            const AddressSpaceAllocatorForbiddenRegion *GetForbiddenRegions() const {
                return m_forbidden_regions;
            }

            static size_t GetForbiddenRegionCount() {
                return ForbiddenRegionCount;
            }

            static u64 GetHeapSpaceBeginAddress() {
                return GetAslrInfo(svc::InfoType_HeapRegionAddress);
            }

            static u64 GetHeapSpaceSize() {
                return GetAslrInfo(svc::InfoType_HeapRegionSize);
            }

            static u64 GetAliasSpaceBeginAddress() {
                return GetAslrInfo(svc::InfoType_AliasRegionAddress);
            }

            static u64 GetAliasSpaceSize() {
                return GetAslrInfo(svc::InfoType_AliasRegionSize);
            }

            static u64 GetAslrSpaceBeginAddress() {
                return GetAslrInfo(svc::InfoType_AslrRegionAddress);
            }

            static u64 GetAslrSpaceEndAddress() {
                return GetAslrInfo(svc::InfoType_AslrRegionAddress) + GetAslrInfo(svc::InfoType_AslrRegionSize);
            }
    };

    using AslrSpaceManagerImpl = AslrSpaceManagerHorizonImpl;

}
