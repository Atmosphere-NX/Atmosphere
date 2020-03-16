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
#include <stratosphere.hpp>
#include "os_tick_manager.hpp"

namespace ams::os::impl {

    TimeSpan TickManager::ConvertToTimeSpan(Tick tick) const {
        /* Get the tick value. */
        const s64 tick_val  = tick.GetInt64Value();

        /* Get the tick frequency. */
        const s64 tick_freq = GetTickFrequency();
        AMS_AUDIT(tick_freq < MaxTickFrequency);

        /* Clamp tick to range. */
        if (tick_val > GetMaxTick()) {
            return TimeSpan::FromNanoSeconds(std::numeric_limits<s64>::max());
        } else if (tick_val < -GetMaxTick()) {
            return TimeSpan::FromNanoSeconds(std::numeric_limits<s64>::min());
        } else {
            /* Convert to timespan. */
            constexpr s64 NanoSecondsPerSecond = TimeSpan::FromSeconds(1).GetNanoSeconds();
            const s64 seconds = tick_val / tick_freq;
            const s64 frac    = tick_val % tick_freq;
            const TimeSpan ts = TimeSpan::FromSeconds(seconds) + TimeSpan::FromNanoSeconds(frac * NanoSecondsPerSecond / tick_freq);

            constexpr TimeSpan ZeroTS    = TimeSpan::FromNanoSeconds(0);
            AMS_ASSERT(!((tick_val > 0 && ts < ZeroTS) || (tick_val < 0 && ts > ZeroTS)));

            return ts;
        }
    }

    Tick TickManager::ConvertToTick(TimeSpan ts) const {
        /* Get the TimeSpan in nanoseconds. */
        const s64 ns = ts.GetNanoSeconds();

        /* Clamp ns to range. */
        if (ns > GetMaxTimeSpanNs()) {
            return Tick(std::numeric_limits<s64>::max());
        } else if (ns < -GetMaxTimeSpanNs()) {
            return Tick(std::numeric_limits<s64>::min());
        } else {
            /* Get the tick frequency. */
            const s64 tick_freq = GetTickFrequency();
            AMS_AUDIT(tick_freq < MaxTickFrequency);

            /* Convert to tick. */
            constexpr s64 NanoSecondsPerSecond = TimeSpan::FromSeconds(1).GetNanoSeconds();
            const bool negative = ns < 0;
            s64 seconds   = ns / NanoSecondsPerSecond;
            s64 frac      = ns % NanoSecondsPerSecond;

            /* If negative, negate seconds/frac. */
            if (negative) {
                seconds = -seconds;
                frac    = -frac;
            }

            /* Calculate the tick, and invert back to negative if needed. */
            s64 tick = (seconds * tick_freq) + ((frac * tick_freq + NanoSecondsPerSecond - 1) / NanoSecondsPerSecond);
            if (negative) {
                tick = -tick;
            }

            return Tick(tick);
        }
    }

}
