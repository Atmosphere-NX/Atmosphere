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

        constinit KAddressSpaceInfo AddressSpaceInfos[] = {
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

        constexpr u8 FlagsToAddressSpaceWidthTable[4] = {
            32, 36, 32, 39
        };

        constexpr size_t GetAddressSpaceWidth(ams::svc::CreateProcessFlag flags) {
            /* Convert the input flags to an array index. */
            const size_t idx = (flags & ams::svc::CreateProcessFlag_AddressSpaceMask) >> ams::svc::CreateProcessFlag_AddressSpaceShift;
            MESOSPHERE_ABORT_UNLESS(idx < sizeof(FlagsToAddressSpaceWidthTable));

            /* Return the width. */
            return FlagsToAddressSpaceWidthTable[idx];
        }

        static_assert(GetAddressSpaceWidth(ams::svc::CreateProcessFlag_AddressSpace32Bit) == 32);
        static_assert(GetAddressSpaceWidth(ams::svc::CreateProcessFlag_AddressSpace64BitDeprecated) == 36);
        static_assert(GetAddressSpaceWidth(ams::svc::CreateProcessFlag_AddressSpace32BitWithoutAlias) == 32);
        static_assert(GetAddressSpaceWidth(ams::svc::CreateProcessFlag_AddressSpace64Bit) == 39);

        KAddressSpaceInfo &GetAddressSpaceInfo(size_t width, KAddressSpaceInfo::Type type) {
            for (auto &info : AddressSpaceInfos) {
                if (info.GetWidth() == width && info.GetType() == type) {
                    return info;
                }
            }
            MESOSPHERE_PANIC("Could not find AddressSpaceInfo");
        }

    }

    uintptr_t KAddressSpaceInfo::GetAddressSpaceStart(ams::svc::CreateProcessFlag flags, KAddressSpaceInfo::Type type, size_t code_size) {
        MESOSPHERE_UNUSED(code_size);
        return GetAddressSpaceInfo(GetAddressSpaceWidth(flags), type).GetAddress();
    }

    size_t KAddressSpaceInfo::GetAddressSpaceSize(ams::svc::CreateProcessFlag flags, KAddressSpaceInfo::Type type) {
        /* Extract the address space from the create process flags. */
        const auto as_flags = (flags & ams::svc::CreateProcessFlag_AddressSpaceMask);

        /* Get the address space width. */
        const auto as_width = GetAddressSpaceWidth(flags);

        /* Get the size. */
        size_t as_size = GetAddressSpaceInfo(as_width, type).GetSize();

        /* If we're getting size for 32-bit without alias, adjust the sizes accordingly. */
        if (as_flags == ams::svc::CreateProcessFlag_AddressSpace32BitWithoutAlias) {
            switch (type) {
                /* The heap space receives space that would otherwise go to the alias space. */
                case KAddressSpaceInfo::Type_Heap:
                    as_size += GetAddressSpaceInfo(as_width, KAddressSpaceInfo::Type_Alias).GetSize();
                    break;
                /* The alias space doesn't exist. */
                case KAddressSpaceInfo::Type_Alias:
                    as_size = 0;
                    break;
                /* Nothing to do by default. */
                default:
                    break;
            }
        }

        return as_size;
    }

    void KAddressSpaceInfo::SetAddressSpaceSize(size_t width, Type type, size_t size) {
        GetAddressSpaceInfo(width, type).SetSize(size);
    }

}
