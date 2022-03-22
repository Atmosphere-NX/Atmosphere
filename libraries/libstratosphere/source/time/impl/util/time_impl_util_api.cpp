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

namespace ams::time::impl::util {

    namespace {

        constexpr inline const int DaysPerMonth[12] = {
            31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
        };
        static_assert(std::accumulate(std::begin(DaysPerMonth), std::end(DaysPerMonth), 0) == 365);

        constexpr inline const std::array<int, 12> SumDaysPerMonth = [] {
            std::array<int, 12> days = {};
            for (size_t i = 1; i < days.size(); ++i) {
                days[i] = days[i - 1] + DaysPerMonth[i - 1];
            }
            return days;
        }();
        static_assert(SumDaysPerMonth[ 0] == 0);
        static_assert(SumDaysPerMonth[11] + DaysPerMonth[11] == 365);

        constexpr bool IsLeapYearImpl(int year) {
            if ((year % 400) == 0) {
                return true;
            } else if ((year % 100) == 0) {
                return false;
            } else if ((year % 4) == 0) {
                return true;
            } else {
                return false;
            }
        }

        constexpr int DateToDaysImpl(int year, int month, int day) {
            /* Lightly validate input. */
            AMS_ASSERT(year > 0);
            AMS_ASSERT(month > 0);
            AMS_ASSERT(day > 0);

            /* Adjust months within range. */
            year += month / 12;
            month %= 12;
            if (month == 0) {
                month = 12;
            }
            AMS_ASSERT(1 <= month && month <= 12);

            /* Calculate days. */
            int res = (year - 1) * 365;
            res += (year / 4) - (year / 100) + (year / 400);
            res += SumDaysPerMonth[month - 1] + day;

            /* Subtract leap day, if it hasn't happened yet. */
            if (month < 3 && IsLeapYearImpl(year)) {
                res -= 1;
            }

            /* Subtract the current day. */
            res -= 1;

            return res;
        }

        constexpr void DaysToDateImpl(int *out_year, int *out_month, int *out_day, int days) {
            /* Lightly validate input. */
            AMS_ASSERT(days > 0);

            /* Declare unit conversion factors. */
            constexpr int DaysPerYear          = 365;
            constexpr int DaysPerFourYears     = DaysPerYear * 4 + 1;
            constexpr int DaysPerCentury       = DaysPerFourYears * 25 - 1;
            constexpr int DaysPerFourCenturies = DaysPerCentury * 4 + 1;

            /* Adjust date. */
            days -= 59;
            days += 365;

            /* Determine various units. */
            int four_centuries     = days / DaysPerFourCenturies;
            int four_centuries_rem = days % DaysPerFourCenturies;
            if (four_centuries_rem < 0) {
                four_centuries_rem += DaysPerFourCenturies;
                --four_centuries;
            }

            int centuries     = four_centuries_rem / DaysPerCentury;
            int centuries_rem = four_centuries_rem % DaysPerCentury;

            int four_years     = centuries_rem / DaysPerFourYears;
            int four_years_rem = centuries_rem % DaysPerFourYears;

            int years     = four_years_rem / DaysPerYear;
            int years_rem = four_years_rem % DaysPerYear;

            /* Adjust into range. */
            int year  = 400 * four_centuries + 100 * centuries + 4 * four_years + years;
            int month = (5 * years_rem + 2) / 153;
            int day   = years_rem - (153 * month + 2) / 5 + 1;

            /* Adjust in case we fell into a pathological case. */
            if (years == 4 || centuries == 4) {
                month = 11;
                day   = 29;
                year -= 1;
            }

            /* Adjust month. */
            if (month <= 9) {
                month += 3;
            } else {
                month -= 9;
                year  += 1;
            }

            /* Set output. */
            if (out_year) {
                *out_year = year;
            }
            if (out_month) {
                *out_month = month;
            }
            if (out_day) {
                *out_day = day;
            }
        }

        constexpr inline int EpochDays = DateToDaysImpl(1970, 1, 1);

        static_assert([]() -> bool {
            int year{}, month{}, day{};

            DaysToDateImpl(std::addressof(year), std::addressof(month), std::addressof(day), EpochDays);

            return year == 1970 && month == 1 && day == 1;
        }());

    }

    Result GetSpanBetween(s64 *out, const SteadyClockTimePoint &from, const SteadyClockTimePoint &to) {
        AMS_ASSERT(out != nullptr);

        R_UNLESS(out != nullptr,                 time::ResultInvalidPointer());
        R_UNLESS(from.source_id == to.source_id, time::ResultNotComparable());

        const bool no_overflow = (from.value >= 0 ? (to.value >= std::numeric_limits<s64>::min() + from.value)
                                                  : (to.value <= std::numeric_limits<s64>::max() + from.value));
        R_UNLESS(no_overflow, time::ResultOverflowed());

        *out = to.value - from.value;
        return ResultSuccess();
    }

    bool IsValidDate(int year, int month, int day) {
        return 1 <= year && 1 <= month && month <= 12 && 1 <= day && day <= GetDaysInMonth(year, month);
    }

    bool IsLeapYear(int year) {
        AMS_ASSERT(year > 0);

        return IsLeapYearImpl(year);
    }

    int GetDaysInMonth(int year, int month) {
        /* Check pre-conditions. */
        AMS_ASSERT(year > 0);
        AMS_ASSERT(1 <= month && month <= 12);

        if (month == 2 && IsLeapYear(year)) {
            return DaysPerMonth[month - 1] + 1;
        } else {
            return DaysPerMonth[month - 1];
        }
    }

    int DateToDays(int year, int month, int day) {
        return DateToDaysImpl(year, month, day);
    }

    void DaysToDate(int *out_year, int *out_month, int *out_day, int days) {
        DaysToDateImpl(out_year, out_month, out_day, days);
    }

    CalendarTime ToCalendarTimeInUtc(const PosixTime &posix_time) {
        constexpr s64 SecondsPerDay    = TimeSpan::FromDays(1).GetSeconds();
        constexpr s64 SecondsPerHour   = TimeSpan::FromHours(1).GetSeconds();
        constexpr s64 SecondsPerMinute = TimeSpan::FromMinutes(1).GetSeconds();

        /* Get year/month/day. */
        int year, month, day;
        DaysToDate(std::addressof(year), std::addressof(month), std::addressof(day), static_cast<int>(posix_time.value / SecondsPerDay) + EpochDays);

        /* Handle negative posix times. */
        s64 posix_abs = posix_time.value >= 0 ? posix_time.value : -1 * posix_time.value;
        s64 posix_rem = posix_abs % SecondsPerDay;
        if (posix_time.value < 0) {
            posix_rem *= -1;
        }

        /* Adjust remainder if negative. */
        if (posix_rem < 0) {
            if ((--day) <= 0) {
                if ((--month) <= 0) {
                    --year;
                    month = 12;
                }
                day = time::impl::util::GetDaysInMonth(year, month);
            }

            posix_rem += SecondsPerDay;
        }

        const int hour = posix_rem / SecondsPerHour;
        posix_rem %= SecondsPerHour;

        const int minute = posix_rem / SecondsPerMinute;
        posix_rem %= SecondsPerMinute;

        const int second = posix_rem;

        return CalendarTime {
            .year   = static_cast<s16>(year),
            .month  = static_cast<s8>(month),
            .day    = static_cast<s8>(day),
            .hour   = static_cast<s8>(hour),
            .minute = static_cast<s8>(minute),
            .second = static_cast<s8>(second),
        };
    }

    PosixTime ToPosixTimeFromUtc(const CalendarTime &calendar_time) {
        /* Validate pre-conditions. */
        AMS_ASSERT(IsValidDate(calendar_time.year, calendar_time.month, calendar_time.day));
        AMS_ASSERT(0 <= calendar_time.hour && calendar_time.hour <= 23);
        AMS_ASSERT(0 <= calendar_time.minute && calendar_time.minute <= 59);
        AMS_ASSERT(0 <= calendar_time.second && calendar_time.second <= 59);

        /* Extract/convert fields. */
        const s64 days = static_cast<s64>(time::impl::util::DateToDays(calendar_time.year, calendar_time.month, calendar_time.day)) - EpochDays;
        const s64 hours = calendar_time.hour;
        const s64 minutes = calendar_time.minute;
        const s64 seconds = calendar_time.second;

        return PosixTime { .value = ((((days * 24) + hours) * 60) + minutes) * 60 + seconds };
    }

}
