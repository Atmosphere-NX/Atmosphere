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
#pragma once
#include <stratosphere.hpp>

namespace ams::os::impl {

    Result CreateInterProcessEvent(InterProcessEventType *event, EventClearMode clear_mode);
    void DestroyInterProcessEvent(InterProcessEventType *event);

    void AttachInterProcessEvent(InterProcessEventType *event, Handle read_handle, bool read_handle_managed, Handle write_handle, bool write_handle_managed, EventClearMode clear_mode);

    Handle DetachReadableHandleOfInterProcessEvent(InterProcessEventType *event);
    Handle DetachWritableHandleOfInterProcessEvent(InterProcessEventType *event);

    void WaitInterProcessEvent(InterProcessEventType *event);
    bool TryWaitInterProcessEvent(InterProcessEventType *event);
    bool TimedWaitInterProcessEvent(InterProcessEventType *event, TimeSpan timeout);

    void SignalInterProcessEvent(InterProcessEventType *event);
    void ClearInterProcessEvent(InterProcessEventType *event);

    Handle GetReadableHandleOfInterProcessEvent(const InterProcessEventType *event);
    Handle GetWritableHandleOfInterProcessEvent(const InterProcessEventType *event);

    void InitializeWaitableHolder(WaitableHolderType *waitable_holder, InterProcessEventType *event);

}
