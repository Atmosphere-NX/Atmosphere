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
#include <stratosphere/os/os_native_handle.hpp>

namespace ams::os {

    struct SystemEventType;
    struct MultiWaitHolderType;

    Result CreateSystemEvent(SystemEventType *event, EventClearMode clear_mode, bool inter_process);
    void DestroySystemEvent(SystemEventType *event);

    void AttachSystemEvent(SystemEventType *event, NativeHandle read_handle, bool read_handle_managed, NativeHandle write_handle, bool write_handle_managed, EventClearMode clear_mode);
    void AttachReadableHandleToSystemEvent(SystemEventType *event, NativeHandle read_handle, bool manage_read_handle, EventClearMode clear_mode);
    void AttachWritableHandleToSystemEvent(SystemEventType *event, NativeHandle write_handle, bool manage_write_handle, EventClearMode clear_mode);

    NativeHandle DetachReadableHandleOfSystemEvent(SystemEventType *event);
    NativeHandle DetachWritableHandleOfSystemEvent(SystemEventType *event);

    NativeHandle GetReadableHandleOfSystemEvent(const SystemEventType *event);
    NativeHandle GetWritableHandleOfSystemEvent(const SystemEventType *event);

    void SignalSystemEvent(SystemEventType *event);
    void WaitSystemEvent(SystemEventType *event);
    bool TryWaitSystemEvent(SystemEventType *event);
    bool TimedWaitSystemEvent(SystemEventType *event, TimeSpan timeout);
    void ClearSystemEvent(SystemEventType *event);

    void InitializeMultiWaitHolder(MultiWaitHolderType *multi_wait_holder, SystemEventType *event);

}
