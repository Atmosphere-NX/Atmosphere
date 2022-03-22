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
#include <vapours.hpp>
#include <stratosphere/time/time_common.hpp>
#include <stratosphere/time/time_system_clock_common.hpp>
#include <stratosphere/time/time_posix_time.hpp>

namespace ams::time {

    class StandardNetworkSystemClock {
        public:
            using rep        = SystemClockTraits::rep;
            using period     = SystemClockTraits::period;
            using duration   = SystemClockTraits::duration;
            using time_point = SystemClockTraits::time_point;
            static constexpr bool is_steady = false;
        public:
            static time_point now();
            static std::time_t to_time_t(const StandardUserSystemClock::time_point &t);
            static time_point from_time_t(std::time_t t);
        public:
            static Result GetCurrentTime(PosixTime *out);
            /* TODO: static Result GetSystemClockContext(SystemClockContext *out); */
    };

}
