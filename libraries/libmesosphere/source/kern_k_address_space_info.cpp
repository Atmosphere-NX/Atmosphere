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
            { 32, ams::svc::AddressSmallMap32Start, ams::svc::AddressSmallMap32Size,          KAddressSpaceInfo::Type_MapSmall, },
            { 32, ams::svc::AddressLargeMap32Start, ams::svc::AddressLargeMap32Size,          KAddressSpaceInfo::Type_MapLarge, },
            { 32, Invalid,                          ams::svc::AddressMemoryRegionHeap32Size,  KAddressSpaceInfo::Type_Heap,     },
            { 32, Invalid,                          ams::svc::AddressMemoryRegionAlias32Size, KAddressSpaceInfo::Type_Alias,    },
            { 36, ams::svc::AddressSmallMap36Start, ams::svc::AddressSmallMap36Size,          KAddressSpaceInfo::Type_MapSmall, },
            { 36, ams::svc::AddressLargeMap36Start, ams::svc::AddressLargeMap36Size,          KAddressSpaceInfo::Type_MapLarge, },
            { 36, Invalid,                          ams::svc::AddressMemoryRegionHeap36Size,  KAddressSpaceInfo::Type_Heap,     },
            { 36, Invalid,                          ams::svc::AddressMemoryRegionAlias36Size, KAddressSpaceInfo::Type_Alias,    },
            { 39, ams::svc::AddressMap39Start,      ams::svc::AddressMap39Size,               KAddressSpaceInfo::Type_Map39Bit, },
            { 39, Invalid,                          ams::svc::AddressMemoryRegionSmall39Size, KAddressSpaceInfo::Type_MapSmall, },
            { 39, Invalid,                          ams::svc::AddressMemoryRegionHeap39Size,  KAddressSpaceInfo::Type_Heap,     },
            { 39, Invalid,                          ams::svc::AddressMemoryRegionAlias39Size, KAddressSpaceInfo::Type_Alias,    },
            { 39, Invalid,                          ams::svc::AddressMemoryRegionStack39Size, KAddressSpaceInfo::Type_Stack,    },
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
