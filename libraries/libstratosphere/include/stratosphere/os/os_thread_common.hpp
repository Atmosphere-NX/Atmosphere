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

namespace ams::os {

    constexpr inline s32 ThreadSuspendCountMax = 127;

    constexpr inline s32 ThreadNameLengthMax = 0x20;

    constexpr inline s32 ThreadPriorityRangeSize = 32;
    constexpr inline s32 HighestThreadPriority   = 0;
    constexpr inline s32 DefaultThreadPriority   = ThreadPriorityRangeSize / 2;
    constexpr inline s32 LowestThreadPriority    = ThreadPriorityRangeSize - 1;

    constexpr inline s32 InvalidThreadPriority   = 127;

    constexpr inline s32 LowestSystemThreadPriority  = 35;
    constexpr inline s32 HighestSystemThreadPriority = -12;

    constexpr inline size_t StackGuardAlignment  = 4_KB;
    constexpr inline size_t ThreadStackAlignment = 4_KB;

    using ThreadFunction = void (*)(void *);

}