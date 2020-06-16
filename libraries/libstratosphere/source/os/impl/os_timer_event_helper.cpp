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
#include "os_timer_event_helper.hpp"

namespace ams::os::impl {

    TimeSpan SaturatedAdd(TimeSpan t1, TimeSpan t2) {
        AMS_ASSERT(t1 >= 0);
        AMS_ASSERT(t2 >= 0);

        const u64 u1 = t1.GetNanoSeconds();
        const u64 u2 = t2.GetNanoSeconds();

        const u64 sum = u1 + u2;
        return TimeSpan::FromNanoSeconds(std::min<u64>(std::numeric_limits<s64>::max(), sum));
    }

    void StopTimerUnsafe(TimerEventType *event) {
        GetReference(event->next_time_to_wakeup) = 0;
        event->timer_state = TimerEventType::TimerState_Stop;
    }

    bool UpdateSignalStateAndRecalculateNextTimeToWakeupUnsafe(TimerEventType *event, TimeSpan cur_time) {
        TimeSpan next_time = GetReference(event->next_time_to_wakeup);

        switch (event->timer_state) {
            case TimerEventType::TimerState_Stop:
                break;
            case TimerEventType::TimerState_OneShot:
                if (next_time < cur_time) {
                    event->signaled = true;
                    StopTimerUnsafe(event);
                    return true;
                }
                break;
            case TimerEventType::TimerState_Periodic:
                if (next_time < cur_time) {
                    event->signaled = true;

                    next_time = SaturatedAdd(next_time, GetReference(event->interval));
                    if (next_time < cur_time) {
                        const u64 elapsed_from_first = (cur_time - GetReference(event->first)).GetNanoSeconds();
                        const u64 interval           = GetReference(event->interval).GetNanoSeconds();

                        const u64 t = std::min<u64>(std::numeric_limits<s64>::max(), ((elapsed_from_first / interval) + 1) * interval);
                        next_time = SaturatedAdd(TimeSpan::FromNanoSeconds(t), GetReference(event->first));
                    }

                    GetReference(event->next_time_to_wakeup) = next_time;
                    return true;
                }
                break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }

        return false;
    }

}
