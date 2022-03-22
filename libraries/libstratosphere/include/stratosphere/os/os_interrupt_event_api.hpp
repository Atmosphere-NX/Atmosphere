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
#include <stratosphere/os/os_interrupt_event_common.hpp>

namespace ams::os {

    struct InterruptEventType;
    struct MultiWaitHolderType;

    void InitializeInterruptEvent(InterruptEventType *event, InterruptName name, EventClearMode clear_mode);
    void FinalizeInterruptEvent(InterruptEventType *event);

    void WaitInterruptEvent(InterruptEventType *event);
    bool TryWaitInterruptEvent(InterruptEventType *event);
    bool TimedWaitInterruptEvent(InterruptEventType *event, TimeSpan timeout);
    void ClearInterruptEvent(InterruptEventType *event);

    void InitializeMultiWaitHolder(MultiWaitHolderType *multi_wait_holder, InterruptEventType *event);

}
