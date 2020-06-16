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
#include <vapours.hpp>

namespace ams::mmu::arch::arm {

    /* NOTE: mmu is not supported by libexosphere-on-arm. */
    /* However, we want the memory layout to be parseable, so we will include some arm64 mmu definitions. */
    constexpr inline u64 L1EntryShift = 30;
    constexpr inline u64 L2EntryShift = 21;
    constexpr inline u64 L3EntryShift = 12;

    constexpr inline u64 L1EntrySize = 1_GB;
    constexpr inline u64 L2EntrySize = 2_MB;
    constexpr inline u64 L3EntrySize = 4_KB;

    constexpr inline u64 PageSize = L3EntrySize;

    constexpr inline u64 L1EntryMask = ((static_cast<u64>(1) << (48 - L1EntryShift)) - 1) << L1EntryShift;
    constexpr inline u64 L2EntryMask = ((static_cast<u64>(1) << (48 - L2EntryShift)) - 1) << L2EntryShift;
    constexpr inline u64 L3EntryMask = ((static_cast<u64>(1) << (48 - L3EntryShift)) - 1) << L3EntryShift;

    constexpr inline u64 TableEntryMask = L3EntryMask;

    static_assert(L1EntryMask == 0x0000FFFFC0000000ul);
    static_assert(L2EntryMask == 0x0000FFFFFFE00000ul);
    static_assert(L3EntryMask == 0x0000FFFFFFFFF000ul);

}
