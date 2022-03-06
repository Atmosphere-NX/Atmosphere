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
#include "os_tick_manager.hpp"

namespace ams::os::impl {

    using TargetTimeSpan = DWORD;

    class TimeoutHelperImpl {
        public:
            static TargetTimeSpan ConvertToImplTime(Tick tick) {
                constexpr s64 MaxTime = std::numeric_limits<s64>::max() - TimeSpan::FromMilliSeconds(1).GetNanoSeconds();
                constexpr auto Ratio  = TimeSpan::FromMilliSeconds(1);
                constexpr auto NanoS  = TimeSpan::FromNanoSeconds(1);

                const auto time = TimeSpan::FromNanoSeconds(std::min<s64>(MaxTime, impl::GetTickManager().ConvertToTimeSpan(tick).GetNanoSeconds()));

                return static_cast<TargetTimeSpan>((time + Ratio - NanoS).GetMilliSeconds());
            }

            static void Sleep(TimeSpan tm);
    };

}
