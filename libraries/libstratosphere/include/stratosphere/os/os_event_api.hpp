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

    struct EventType;
    struct MultiWaitHolderType;

    void InitializeEvent(EventType *event, bool signaled, EventClearMode clear_mode);
    void FinalizeEvent(EventType *event);

    void SignalEvent(EventType *event);
    void WaitEvent(EventType *event);
    bool TryWaitEvent(EventType *event);
    bool TimedWaitEvent(EventType *event, TimeSpan timeout);
    void ClearEvent(EventType *event);

    void InitializeMultiWaitHolder(MultiWaitHolderType *multi_wait_holder, EventType *event);

}
