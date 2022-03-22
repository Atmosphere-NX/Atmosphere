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
#include <stratosphere/time/time_posix_time.hpp>
#include <stratosphere/time/time_steady_clock_time_point.hpp>

namespace ams::time::impl::util {

    Result GetSpanBetween(s64 *out, const SteadyClockTimePoint &from, const SteadyClockTimePoint &to);

    bool IsLeapYear(int year);
    bool IsValidDate(int year, int month, int day);

    int GetDaysInMonth(int year, int month);

    int DateToDays(int year, int month, int day);
    void DaysToDate(int *out_year, int *out_month, int *out_day, int days);

    CalendarTime ToCalendarTimeInUtc(const PosixTime &posix_time);
    PosixTime ToPosixTimeFromUtc(const CalendarTime &calendar_time);

}
