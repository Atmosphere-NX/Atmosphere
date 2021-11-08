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
#include "util_test_framework.hpp"

namespace ams::test {

    static constexpr s32 NumCores                           =  4;
    static constexpr s32 DpcManagerNormalThreadPriority     = 59;
    static constexpr s32 DpcManagerPreemptionThreadPriority = 63;

    static constexpr s32 HighestTestPriority = 32;
    static constexpr s32 LowestTestPriority  = svc::LowestThreadPriority;
    static_assert(HighestTestPriority < LowestTestPriority);

    static constexpr TimeSpan PreemptionTimeSpan = TimeSpan::FromMilliSeconds(10);

    constexpr inline bool IsPreemptionPriority(s32 core, s32 priority) {
        return priority == ((core == (NumCores - 1)) ? DpcManagerPreemptionThreadPriority : DpcManagerNormalThreadPriority);
    }

}