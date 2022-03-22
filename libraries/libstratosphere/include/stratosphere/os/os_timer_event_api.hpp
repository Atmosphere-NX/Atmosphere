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
#include <stratosphere/os/os_event_common.hpp>

namespace ams::os {

    struct TimerEventType;
    struct MultiWaitHolderType;

    void InitializeTimerEvent(TimerEventType *event, EventClearMode clear_mode);
    void FinalizeTimerEvent(TimerEventType *event);

    void StartOneShotTimerEvent(TimerEventType *event, TimeSpan first_time);
    void StartPeriodicTimerEvent(TimerEventType *event, TimeSpan first_time, TimeSpan interval);
    void StopTimerEvent(TimerEventType *event);

    void WaitTimerEvent(TimerEventType *event);
    bool TryWaitTimerEvent(TimerEventType *event);

    void SignalTimerEvent(TimerEventType *event);
    void ClearTimerEvent(TimerEventType *event);

    void InitializeMultiWaitHolder(MultiWaitHolderType *multi_wait_holder, TimerEventType *event);

}
