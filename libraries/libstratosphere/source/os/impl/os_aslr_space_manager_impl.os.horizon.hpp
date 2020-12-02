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
        private:
            static u64 GetAslrInfo(svc::InfoType type) {
                u64 value;
                R_ABORT_UNLESS(svc::GetInfo(std::addressof(value), type, svc::PseudoHandle::CurrentProcess, 0));
                static_assert(std::same_as<size_t, uintptr_t>);
                AMS_ASSERT(value <= std::numeric_limits<size_t>::max());
                return static_cast<u64>(value & std::numeric_limits<size_t>::max());
            }
        public:
            constexpr AslrSpaceManagerHorizonImpl() = default;

            static u64 GetHeapSpaceBeginAddress() {
                return GetAslrInfo(svc::InfoType_HeapRegionAddress);
            }

            static u64 GetHeapSpaceBeginSize() {
                return GetAslrInfo(svc::InfoType_HeapRegionSize);
            }

            static u64 GetAliasSpaceBeginAddress() {
                return GetAslrInfo(svc::InfoType_AliasRegionAddress);
            }

            static u64 GetAliasSpaceBeginSize() {
                return GetAslrInfo(svc::InfoType_AliasRegionSize);
            }

            static u64 GetAslrSpaceBeginAddress() {
                if (hos::GetVersion() >= hos::Version_2_0_0 || svc::IsKernelMesosphere()) {
                    return GetAslrInfo(svc::InfoType_AslrRegionAddress);
                } else {
                    if (GetHeapSpaceBeginAddress() < AslrBase64BitDeprecated || GetAliasSpaceBeginAddress() < AslrBase64BitDeprecated) {
                        return AslrBase32Bit;
                    } else {
                        return AslrBase64BitDeprecated;
                    }
                }
            }

            static u64 GetAslrSpaceEndAddress() {
                if (hos::GetVersion() >= hos::Version_2_0_0 || svc::IsKernelMesosphere()) {
                    return GetAslrInfo(svc::InfoType_AslrRegionAddress) + GetAslrInfo(svc::InfoType_AslrRegionSize);
                } else {
                    if (GetHeapSpaceBeginAddress() < AslrBase64BitDeprecated || GetAliasSpaceBeginAddress() < AslrBase64BitDeprecated) {
                        return AslrBase32Bit + AslrSize32Bit;
                    } else {
                        return AslrBase64BitDeprecated + AslrSize64BitDeprecated;
                    }
                }
            }
    };

    using AslrSpaceManagerImpl = AslrSpaceManagerHorizonImpl;

}
