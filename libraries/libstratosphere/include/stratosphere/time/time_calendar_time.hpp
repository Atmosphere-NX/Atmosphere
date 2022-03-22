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

namespace ams::time {

    struct CalendarTime {
        s16 year;
        s8 month;
        s8 day;
        s8 hour;
        s8 minute;
        s8 second;

        bool IsValid() const;

        CalendarTime &operator+=(const TimeSpan &ts);
        CalendarTime &operator-=(const TimeSpan &ts);

        friend CalendarTime operator+(const CalendarTime &lhs, const TimeSpan &rhs);
        friend CalendarTime operator-(const CalendarTime &lhs, const TimeSpan &rhs);

        friend TimeSpan operator-(const CalendarTime &lhs, const CalendarTime &rhs);

        constexpr friend bool operator==(const CalendarTime &lhs, const CalendarTime &rhs) { return lhs.year == rhs.year && lhs.month == rhs.month && lhs.day == rhs.day && lhs.hour == rhs.hour && lhs.minute == rhs.minute && lhs.second == rhs.second; }
        constexpr friend bool operator!=(const CalendarTime &lhs, const CalendarTime &rhs) { return !(lhs == rhs); }

        constexpr friend bool operator<=(const CalendarTime &lhs, const CalendarTime &rhs) { return !(rhs < lhs); }
        constexpr friend bool operator>=(const CalendarTime &lhs, const CalendarTime &rhs) { return !(lhs < rhs); }
        constexpr friend bool operator> (const CalendarTime &lhs, const CalendarTime &rhs) { return rhs < lhs; }

        constexpr friend bool operator< (const CalendarTime &lhs, const CalendarTime &rhs) {
            if (!std::is_constant_evaluated()) {
                AMS_ASSERT(lhs.IsValid());
                AMS_ASSERT(rhs.IsValid());
            }

            constexpr auto ToUint64 = [](const time::CalendarTime &time) ALWAYS_INLINE_LAMBDA {
                return (static_cast<u64>(time.year)   << 40) |
                       (static_cast<u64>(time.month)  << 32) |
                       (static_cast<u64>(time.day)    << 24) |
                       (static_cast<u64>(time.hour)   << 16) |
                       (static_cast<u64>(time.minute) <<  8) |
                       (static_cast<u64>(time.second) <<  0);
            };

            return ToUint64(lhs) < ToUint64(rhs);
        }
    };
    static_assert(sizeof(CalendarTime) == sizeof(u64));

}
