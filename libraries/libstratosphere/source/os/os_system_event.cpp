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
#include "impl/os_multiple_wait_holder_impl.hpp"
#include "impl/os_inter_process_event.hpp"
#include "impl/os_timeout_helper.hpp"

namespace ams::os {

    Result CreateSystemEvent(SystemEventType *event, EventClearMode clear_mode, bool inter_process) {
        if (inter_process) {
            R_TRY(impl::CreateInterProcessEvent(std::addressof(event->inter_process_event), clear_mode));
            event->state = SystemEventType::State_InitializedAsInterProcessEvent;
        } else {
            InitializeEvent(std::addressof(event->event), false, clear_mode);
            event->state = SystemEventType::State_InitializedAsEvent;
        }
        return ResultSuccess();
    }

    void DestroySystemEvent(SystemEventType *event) {
        auto state = event->state;
        event->state = SystemEventType::State_NotInitialized;

        switch (state) {
            case SystemEventType::State_InitializedAsInterProcessEvent: impl::DestroyInterProcessEvent(std::addressof(event->inter_process_event)); break;
            case SystemEventType::State_InitializedAsEvent:             FinalizeEvent(std::addressof(event->event)); break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    void AttachSystemEvent(SystemEventType *event, NativeHandle read_handle, bool read_handle_managed, NativeHandle write_handle, bool write_handle_managed, EventClearMode clear_mode) {
        AMS_ASSERT(read_handle != svc::InvalidHandle || write_handle != svc::InvalidHandle);
        impl::AttachInterProcessEvent(std::addressof(event->inter_process_event), read_handle, read_handle_managed, write_handle, write_handle_managed, clear_mode);
        event->state = SystemEventType::State_InitializedAsInterProcessEvent;
    }

    void AttachReadableHandleToSystemEvent(SystemEventType *event, NativeHandle read_handle, bool manage_read_handle, EventClearMode clear_mode) {
        return AttachSystemEvent(event, read_handle, manage_read_handle, svc::InvalidHandle, false, clear_mode);
    }

    void AttachWritableHandleToSystemEvent(SystemEventType *event, NativeHandle write_handle, bool manage_write_handle, EventClearMode clear_mode) {
        return AttachSystemEvent(event, svc::InvalidHandle, false, write_handle, manage_write_handle, clear_mode);
    }

    NativeHandle DetachReadableHandleOfSystemEvent(SystemEventType *event) {
        AMS_ASSERT(event->state == SystemEventType::State_InitializedAsInterProcessEvent);
        return impl::DetachReadableHandleOfInterProcessEvent(std::addressof(event->inter_process_event));
    }

    NativeHandle DetachWritableHandleOfSystemEvent(SystemEventType *event) {
        AMS_ASSERT(event->state == SystemEventType::State_InitializedAsInterProcessEvent);
        return impl::DetachWritableHandleOfInterProcessEvent(std::addressof(event->inter_process_event));
    }

    NativeHandle GetReadableHandleOfSystemEvent(const SystemEventType *event) {
        AMS_ASSERT(event->state == SystemEventType::State_InitializedAsInterProcessEvent);
        return impl::GetReadableHandleOfInterProcessEvent(std::addressof(event->inter_process_event));
    }

    NativeHandle GetWritableHandleOfSystemEvent(const SystemEventType *event) {
        AMS_ASSERT(event->state == SystemEventType::State_InitializedAsInterProcessEvent);
        return impl::GetWritableHandleOfInterProcessEvent(std::addressof(event->inter_process_event));

    }

    void SignalSystemEvent(SystemEventType *event) {
        switch (event->state) {
            case SystemEventType::State_InitializedAsInterProcessEvent: return impl::SignalInterProcessEvent(std::addressof(event->inter_process_event));
            case SystemEventType::State_InitializedAsEvent:             return SignalEvent(std::addressof(event->event));
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    void WaitSystemEvent(SystemEventType *event) {
        switch (event->state) {
            case SystemEventType::State_InitializedAsInterProcessEvent: return impl::WaitInterProcessEvent(std::addressof(event->inter_process_event));
            case SystemEventType::State_InitializedAsEvent:             return WaitEvent(std::addressof(event->event));
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    bool TryWaitSystemEvent(SystemEventType *event) {
        switch (event->state) {
            case SystemEventType::State_InitializedAsInterProcessEvent: return impl::TryWaitInterProcessEvent(std::addressof(event->inter_process_event));
            case SystemEventType::State_InitializedAsEvent:             return TryWaitEvent(std::addressof(event->event));
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    bool TimedWaitSystemEvent(SystemEventType *event, TimeSpan timeout) {
        AMS_ASSERT(timeout.GetNanoSeconds() >= 0);

        switch (event->state) {
            case SystemEventType::State_InitializedAsInterProcessEvent: return impl::TimedWaitInterProcessEvent(std::addressof(event->inter_process_event), timeout);
            case SystemEventType::State_InitializedAsEvent:             return TimedWaitEvent(std::addressof(event->event), timeout);
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    void ClearSystemEvent(SystemEventType *event) {
        switch (event->state) {
            case SystemEventType::State_InitializedAsInterProcessEvent: return impl::ClearInterProcessEvent(std::addressof(event->inter_process_event));
            case SystemEventType::State_InitializedAsEvent:             return ClearEvent(std::addressof(event->event));
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    void InitializeMultiWaitHolder(MultiWaitHolderType *multi_wait_holder, SystemEventType *event) {
        switch (event->state) {
            case SystemEventType::State_InitializedAsInterProcessEvent:
                util::ConstructAt(GetReference(multi_wait_holder->impl_storage).holder_of_inter_process_event_storage, std::addressof(event->inter_process_event));
                break;
            case SystemEventType::State_InitializedAsEvent:
                util::ConstructAt(GetReference(multi_wait_holder->impl_storage).holder_of_event_storage, std::addressof(event->event));
                break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

}
