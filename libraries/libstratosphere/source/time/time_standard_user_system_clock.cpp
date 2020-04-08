/*
 * Copyright (c) 2019-2020 Atmosphère-NX
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

    Result StandardUserSystemClock::GetCurrentTime(PosixTime *out) {
        static_assert(sizeof(*out) == sizeof(u64));
        return ::timeGetCurrentTime(::TimeType_UserSystemClock, reinterpret_cast<u64 *>(out));
    }

    StandardUserSystemClock::time_point StandardUserSystemClock::now() {
        PosixTime posix_time = {};
        if (R_FAILED(GetCurrentTime(std::addressof(posix_time)))) {
            posix_time.value = 0;
        }

        return time_point(duration(posix_time.value));
    }

    std::time_t StandardUserSystemClock::to_time_t(const StandardUserSystemClock::time_point &t) {
        return static_cast<std::time_t>(std::chrono::duration_cast<std::chrono::seconds>(t.time_since_epoch()).count());
    }

    StandardUserSystemClock::time_point StandardUserSystemClock::from_time_t(std::time_t t) {
        return time_point(duration(t));
    }

    /* TODO: Result StandardUserSystemClock::GetSystemClockContext(SystemClockContext *out); */

}
