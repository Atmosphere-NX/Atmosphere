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
#include "os_inter_process_event.hpp"
#include "os_inter_process_event_impl.hpp"
#include "os_multiple_wait_object_list.hpp"

namespace ams::os::impl {

    namespace {

        inline void SetupInterProcessEventType(InterProcessEventType *event, NativeHandle read_handle, bool read_handle_managed, NativeHandle write_handle, bool write_handle_managed, EventClearMode clear_mode) {
            /* Set handles. */
            event->readable_handle            = read_handle;
            event->is_readable_handle_managed = read_handle_managed;
            event->writable_handle            = write_handle;
            event->is_writable_handle_managed = write_handle_managed;

            /* Set auto clear. */
            event->auto_clear = (clear_mode == EventClearMode_AutoClear);

            /* Create the waitlist node. */
            util::ConstructAt(event->multi_wait_object_list_storage);

            /* Set state. */
            event->state = InterProcessEventType::State_Initialized;
        }

    }

    Result CreateInterProcessEvent(InterProcessEventType *event, EventClearMode clear_mode) {
        NativeHandle rh, wh;
        R_TRY(impl::InterProcessEventImpl::Create(std::addressof(wh), std::addressof(rh)));

        SetupInterProcessEventType(event, rh, true, wh, true, clear_mode);
        return ResultSuccess();
    }

    void DestroyInterProcessEvent(InterProcessEventType *event) {
        AMS_ASSERT(event->state == InterProcessEventType::State_Initialized);

        /* Clear the state. */
        event->state = InterProcessEventType::State_NotInitialized;

        /* Close handles if required. */
        if (event->is_readable_handle_managed) {
            if (event->readable_handle != os::InvalidNativeHandle) {
                impl::InterProcessEventImpl::Close(event->readable_handle);
            }
            event->is_readable_handle_managed = false;
        }

        if (event->is_writable_handle_managed) {
            if (event->writable_handle != os::InvalidNativeHandle) {
                impl::InterProcessEventImpl::Close(event->writable_handle);
            }
            event->is_writable_handle_managed = false;
        }

        /* Destroy the waitlist. */
        util::DestroyAt(event->multi_wait_object_list_storage);
    }

    void AttachInterProcessEvent(InterProcessEventType *event, NativeHandle read_handle, bool read_handle_managed, NativeHandle write_handle, bool write_handle_managed, EventClearMode clear_mode) {
        AMS_ASSERT(read_handle != os::InvalidNativeHandle || write_handle != os::InvalidNativeHandle);

        return SetupInterProcessEventType(event, read_handle, read_handle_managed, write_handle, write_handle_managed, clear_mode);
    }

    NativeHandle DetachReadableHandleOfInterProcessEvent(InterProcessEventType *event) {
        AMS_ASSERT(event->state == InterProcessEventType::State_Initialized);

        const NativeHandle handle = event->readable_handle;

        event->readable_handle            = os::InvalidNativeHandle;
        event->is_readable_handle_managed = false;

        return handle;
    }

    NativeHandle DetachWritableHandleOfInterProcessEvent(InterProcessEventType *event) {
        AMS_ASSERT(event->state == InterProcessEventType::State_Initialized);

        const NativeHandle handle = event->writable_handle;

        event->writable_handle            = os::InvalidNativeHandle;
        event->is_writable_handle_managed = false;

        return handle;
    }

    void WaitInterProcessEvent(InterProcessEventType *event) {
        AMS_ASSERT(event->state == InterProcessEventType::State_Initialized);

        return impl::InterProcessEventImpl::Wait(event->readable_handle, event->auto_clear);
    }

    bool TryWaitInterProcessEvent(InterProcessEventType *event) {
        AMS_ASSERT(event->state == InterProcessEventType::State_Initialized);

        return impl::InterProcessEventImpl::TryWait(event->readable_handle, event->auto_clear);
    }

    bool TimedWaitInterProcessEvent(InterProcessEventType *event, TimeSpan timeout) {
        AMS_ASSERT(event->state == InterProcessEventType::State_Initialized);
        AMS_ASSERT(timeout.GetNanoSeconds() >= 0);

        return impl::InterProcessEventImpl::TimedWait(event->readable_handle, event->auto_clear, timeout);
    }

    void SignalInterProcessEvent(InterProcessEventType *event) {
        AMS_ASSERT(event->state != InterProcessEventType::State_NotInitialized);

        return impl::InterProcessEventImpl::Signal(event->writable_handle);
    }

    void ClearInterProcessEvent(InterProcessEventType *event) {
        AMS_ASSERT(event->state != InterProcessEventType::State_NotInitialized);

        auto handle = event->readable_handle;
        if (handle == os::InvalidNativeHandle) {
            handle = event->writable_handle;
        }
        return impl::InterProcessEventImpl::Clear(handle);
    }

    NativeHandle GetReadableHandleOfInterProcessEvent(const InterProcessEventType *event) {
        AMS_ASSERT(event->state != InterProcessEventType::State_NotInitialized);

        return event->readable_handle;
    }

    NativeHandle GetWritableHandleOfInterProcessEvent(const InterProcessEventType *event) {
        AMS_ASSERT(event->state != InterProcessEventType::State_NotInitialized);

        return event->writable_handle;
    }

}
