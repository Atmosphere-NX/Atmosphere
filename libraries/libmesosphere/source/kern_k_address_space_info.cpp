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
#include <mesosphere.hpp>

namespace ams::kern {

    namespace {

        constexpr uintptr_t Invalid = std::numeric_limits<uintptr_t>::max();

        constexpr KAddressSpaceInfo AddressSpaceInfos[] = {
            { 32, 2_MB,    1_GB   - 2_MB,   KAddressSpaceInfo::Type_MapSmall, },
            { 32, 1_GB,    4_GB   - 1_GB,   KAddressSpaceInfo::Type_MapLarge, },
            { 32, Invalid, 1_GB,            KAddressSpaceInfo::Type_Heap,     },
            { 32, Invalid, 1_GB,            KAddressSpaceInfo::Type_Alias,    },
            { 36, 128_MB,  2_GB   - 128_MB, KAddressSpaceInfo::Type_MapSmall, },
            { 36, 2_GB,    64_GB  - 2_GB,   KAddressSpaceInfo::Type_MapLarge, },
            { 36, Invalid, 6_GB,            KAddressSpaceInfo::Type_Heap,     },
            { 36, Invalid, 6_GB,            KAddressSpaceInfo::Type_Alias,    },
            { 39, 128_MB,  512_GB - 128_MB, KAddressSpaceInfo::Type_Map39Bit, },
            { 39, Invalid, 64_GB,           KAddressSpaceInfo::Type_MapSmall, },
            { 39, Invalid, 6_GB,            KAddressSpaceInfo::Type_Heap,     },
            { 39, Invalid, 64_GB,           KAddressSpaceInfo::Type_Alias,    },
            { 39, Invalid, 2_GB,            KAddressSpaceInfo::Type_Stack,    },
        };

        constexpr bool IsAllowedIndexForAddress(size_t index) {
            return index < util::size(AddressSpaceInfos) && AddressSpaceInfos[index].GetAddress() != Invalid;
        }

        constexpr size_t AddressSpaceIndices32Bit[KAddressSpaceInfo::Type_Count] = {
            0, 1, 0, 2, 0, 3,
        };

        constexpr size_t AddressSpaceIndices36Bit[KAddressSpaceInfo::Type_Count] = {
            4, 5, 4, 6, 4, 7,
        };

        constexpr size_t AddressSpaceIndices39Bit[KAddressSpaceInfo::Type_Count] = {
            9, 8, 8, 10, 12, 11,
        };

        constexpr bool IsAllowed32BitType(KAddressSpaceInfo::Type type) {
            return type < KAddressSpaceInfo::Type_Count && type != KAddressSpaceInfo::Type_Map39Bit && type != KAddressSpaceInfo::Type_Stack;
        }

        constexpr bool IsAllowed36BitType(KAddressSpaceInfo::Type type) {
            return type < KAddressSpaceInfo::Type_Count && type != KAddressSpaceInfo::Type_Map39Bit && type != KAddressSpaceInfo::Type_Stack;
        }

        constexpr bool IsAllowed39BitType(KAddressSpaceInfo::Type type) {
            return type < KAddressSpaceInfo::Type_Count && type != KAddressSpaceInfo::Type_MapLarge;
        }

    }

    uintptr_t KAddressSpaceInfo::GetAddressSpaceStart(size_t width, KAddressSpaceInfo::Type type) {
        switch (width) {
            case 32:
                MESOSPHERE_ABORT_UNLESS(IsAllowed32BitType(type));
                MESOSPHERE_ABORT_UNLESS(IsAllowedIndexForAddress(AddressSpaceIndices32Bit[type]));
                return AddressSpaceInfos[AddressSpaceIndices32Bit[type]].GetAddress();
            case 36:
                MESOSPHERE_ABORT_UNLESS(IsAllowed36BitType(type));
                MESOSPHERE_ABORT_UNLESS(IsAllowedIndexForAddress(AddressSpaceIndices36Bit[type]));
                return AddressSpaceInfos[AddressSpaceIndices36Bit[type]].GetAddress();
            case 39:
                MESOSPHERE_ABORT_UNLESS(IsAllowed39BitType(type));
                MESOSPHERE_ABORT_UNLESS(IsAllowedIndexForAddress(AddressSpaceIndices39Bit[type]));
                return AddressSpaceInfos[AddressSpaceIndices39Bit[type]].GetAddress();
            MESOSPHERE_UNREACHABLE_DEFAULT_CASE();
        }
    }

    size_t KAddressSpaceInfo::GetAddressSpaceSize(size_t width, KAddressSpaceInfo::Type type) {
        switch (width) {
            case 32:
                MESOSPHERE_ABORT_UNLESS(IsAllowed32BitType(type));
                return AddressSpaceInfos[AddressSpaceIndices32Bit[type]].GetSize();
            case 36:
                MESOSPHERE_ABORT_UNLESS(IsAllowed36BitType(type));
                return AddressSpaceInfos[AddressSpaceIndices36Bit[type]].GetSize();
            case 39:
                MESOSPHERE_ABORT_UNLESS(IsAllowed39BitType(type));
                return AddressSpaceInfos[AddressSpaceIndices39Bit[type]].GetSize();
            MESOSPHERE_UNREACHABLE_DEFAULT_CASE();
        }
    }

}
