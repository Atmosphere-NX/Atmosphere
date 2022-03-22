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
#include "lm_time_util.hpp"

namespace ams::lm::srv {

    namespace {

        constinit std::atomic_bool g_is_time_invalid = false;

        constinit time::PosixTime InvalidPosixTime = { .value = 0 };

        constexpr bool IsValidPosixTime(time::PosixTime time) {
            return time.value > 0;
        }

        void EnsureTimeInitialized() {
            static constinit os::SdkMutex g_time_initialized_mutex;
            static constinit bool g_time_initialized = false;

            if (AMS_UNLIKELY(!g_time_initialized)) {
                std::scoped_lock lk(g_time_initialized_mutex);

                if (AMS_LIKELY(!g_time_initialized)) {
                    R_ABORT_UNLESS(time::Initialize());
                    g_time_initialized = true;
                }
            }
        }

    }

    time::PosixTime GetCurrentTime() {
        /* Ensure that we can use time services. */
        EnsureTimeInitialized();

        /* Repeatedly try to get a valid time. */
        for (auto wait_seconds = 1; wait_seconds <= 8; wait_seconds *= 2) {
            /* Get the standard user system clock time. */
            time::PosixTime current_time{};
            if (R_FAILED(time::StandardUserSystemClock::GetCurrentTime(std::addressof(current_time)))) {
                return InvalidPosixTime;
            }

            /* If the time is valid, return it. */
            if (IsValidPosixTime(current_time)) {
                return current_time;
            }

            /* Check if we've failed to get a time in the past. */
            if (g_is_time_invalid) {
                return InvalidPosixTime;
            }

            /* Wait a bit before trying again. */
            os::SleepThread(TimeSpan::FromSeconds(wait_seconds));
        }

        /* We failed to get a valid time. */
        g_is_time_invalid = true;
        return InvalidPosixTime;
    }

}
