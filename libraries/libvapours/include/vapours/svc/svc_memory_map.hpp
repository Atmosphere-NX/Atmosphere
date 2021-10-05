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
#include <vapours/common.hpp>

namespace ams::svc {

    #if defined(ATMOSPHERE_ARCH_ARM64)

        constexpr inline size_t AddressMemoryRegionSmall32Size = 1_GB;
        constexpr inline size_t AddressMemoryRegionLarge32Size = 4_GB - AddressMemoryRegionSmall32Size;
        constexpr inline size_t AddressMemoryRegionHeap32Size  = 1_GB;
        constexpr inline size_t AddressMemoryRegionAlias32Size = 1_GB;

        constexpr inline size_t AddressMemoryRegionSmall36Size = 2_GB;
        constexpr inline size_t AddressMemoryRegionLarge36Size = 64_GB - AddressMemoryRegionSmall36Size;
        constexpr inline size_t AddressMemoryRegionHeap36Size  = 6_GB;
        constexpr inline size_t AddressMemoryRegionAlias36Size = 6_GB;

        constexpr inline size_t AddressMemoryRegionSmall39Size = 64_GB;
        constexpr inline size_t AddressMemoryRegionHeap39Size  = 6_GB;
        constexpr inline size_t AddressMemoryRegionAlias39Size = 64_GB;
        constexpr inline size_t AddressMemoryRegionStack39Size = 2_GB;

        constexpr inline size_t AddressMemoryRegion39Size = 512_GB;

    #elif defined(ATMOSPHERE_ARCH_ARM)

        constexpr inline size_t AddressMemoryRegionSmall32Size = 512_MB;
        constexpr inline size_t AddressMemoryRegionLarge32Size = 2_GB - AddressMemoryRegionSmall32Size;
        constexpr inline size_t AddressMemoryRegionHeap32Size  = 1_GB;
        constexpr inline size_t AddressMemoryRegionAlias32Size = 512_MB;

        constexpr inline size_t AddressMemoryRegionSmall36Size = 0;
        constexpr inline size_t AddressMemoryRegionLarge36Size = 0;
        constexpr inline size_t AddressMemoryRegionHeap36Size  = 0;
        constexpr inline size_t AddressMemoryRegionAlias36Size = 0;

        constexpr inline size_t AddressMemoryRegionSmall39Size = 0;
        constexpr inline size_t AddressMemoryRegionHeap39Size  = 0;
        constexpr inline size_t AddressMemoryRegionAlias39Size = 0;
        constexpr inline size_t AddressMemoryRegionStack39Size = 0;

        constexpr inline size_t AddressMemoryRegion39Size = 0;

    #else

        #error "Unknown architecture for svc::AddressMemoryRegion*Size"

    #endif

    constexpr inline size_t AddressNullGuard32Size = 2_MB;
    constexpr inline size_t AddressNullGuard64Size = 128_MB;

    constexpr inline uintptr_t AddressMemoryRegionSmall32Start = 0;
    constexpr inline uintptr_t AddressMemoryRegionSmall32End   = AddressMemoryRegionSmall32Start + AddressMemoryRegionSmall32Size;

    constexpr inline uintptr_t AddressMemoryRegionLarge32Start = AddressMemoryRegionSmall32End;
    constexpr inline uintptr_t AddressMemoryRegionLarge32End   = AddressMemoryRegionLarge32Start + AddressMemoryRegionLarge32Size;

    constexpr inline uintptr_t AddressSmallMap32Start = AddressMemoryRegionSmall32Start + AddressNullGuard32Size;
    constexpr inline uintptr_t AddressSmallMap32End   = AddressMemoryRegionSmall32End;
    constexpr inline size_t    AddressSmallMap32Size  = AddressSmallMap32End - AddressSmallMap32Start;

    constexpr inline uintptr_t AddressLargeMap32Start = AddressMemoryRegionLarge32Start;
    constexpr inline uintptr_t AddressLargeMap32End   = AddressMemoryRegionLarge32End;
    constexpr inline size_t    AddressLargeMap32Size  = AddressLargeMap32End - AddressLargeMap32Start;

    constexpr inline uintptr_t AddressMemoryRegionSmall36Start = 0;
    constexpr inline uintptr_t AddressMemoryRegionSmall36End   = AddressMemoryRegionSmall36Start + AddressMemoryRegionSmall36Size;

    constexpr inline uintptr_t AddressMemoryRegionLarge36Start = AddressMemoryRegionSmall36End;
    constexpr inline uintptr_t AddressMemoryRegionLarge36End   = AddressMemoryRegionLarge36Start + AddressMemoryRegionLarge36Size;

    constexpr inline uintptr_t AddressSmallMap36Start = AddressMemoryRegionSmall36Start + AddressNullGuard64Size;
    constexpr inline uintptr_t AddressSmallMap36End   = AddressMemoryRegionSmall36End;
    constexpr inline size_t    AddressSmallMap36Size  = AddressSmallMap36End - AddressSmallMap36Start;

    constexpr inline uintptr_t AddressLargeMap36Start = AddressMemoryRegionLarge36Start;
    constexpr inline uintptr_t AddressLargeMap36End   = AddressMemoryRegionLarge36End;
    constexpr inline size_t    AddressLargeMap36Size  = AddressLargeMap36End - AddressLargeMap36Start;

    constexpr inline uintptr_t AddressMap39Start = 0 + AddressNullGuard64Size;
    constexpr inline uintptr_t AddressMap39End   = AddressMemoryRegion39Size;
    constexpr inline size_t    AddressMap39Size  = AddressMap39End - AddressMap39Start;

}
