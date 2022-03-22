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
#include <stratosphere.hpp>

namespace ams::time {

    bool CalendarTime::IsValid() const {
        return time::IsValidDate(this->year, this->month, this->day);
    }

    CalendarTime &CalendarTime::operator+=(const TimeSpan &ts) {
        *this = ToCalendarTimeInUtc(ToPosixTimeFromUtc(*this) + ts);
        return *this;
    }

    CalendarTime &CalendarTime::operator-=(const TimeSpan &ts) {
        *this = ToCalendarTimeInUtc(ToPosixTimeFromUtc(*this) - ts);
        return *this;
    }

    CalendarTime operator+(const CalendarTime &lhs, const TimeSpan &rhs) {
        return ToCalendarTimeInUtc(ToPosixTimeFromUtc(lhs) + rhs);
    }

    CalendarTime operator-(const CalendarTime &lhs, const TimeSpan &rhs) {
        return ToCalendarTimeInUtc(ToPosixTimeFromUtc(lhs) - rhs);
    }

    TimeSpan operator-(const CalendarTime &lhs, const CalendarTime &rhs) {
        return ToPosixTimeFromUtc(lhs) - ToPosixTimeFromUtc(rhs);
    }

}
