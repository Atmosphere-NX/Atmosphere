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
#include <stratosphere.hpp>

namespace ams::os::impl {

    Result CreateInterProcessEvent(InterProcessEventType *event, EventClearMode clear_mode);
    void DestroyInterProcessEvent(InterProcessEventType *event);

    void AttachInterProcessEvent(InterProcessEventType *event, NativeHandle read_handle, bool read_handle_managed, NativeHandle write_handle, bool write_handle_managed, EventClearMode clear_mode);

    NativeHandle DetachReadableHandleOfInterProcessEvent(InterProcessEventType *event);
    NativeHandle DetachWritableHandleOfInterProcessEvent(InterProcessEventType *event);

    void WaitInterProcessEvent(InterProcessEventType *event);
    bool TryWaitInterProcessEvent(InterProcessEventType *event);
    bool TimedWaitInterProcessEvent(InterProcessEventType *event, TimeSpan timeout);

    void SignalInterProcessEvent(InterProcessEventType *event);
    void ClearInterProcessEvent(InterProcessEventType *event);

    NativeHandle GetReadableHandleOfInterProcessEvent(const InterProcessEventType *event);
    NativeHandle GetWritableHandleOfInterProcessEvent(const InterProcessEventType *event);

    void InitializeMultiWaitHolder(MultiWaitHolderType *multi_wait_holder, InterProcessEventType *event);

}
