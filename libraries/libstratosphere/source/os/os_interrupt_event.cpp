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
#include "impl/os_interrupt_event_impl.hpp"
#include "impl/os_waitable_holder_impl.hpp"
#include "impl/os_waitable_object_list.hpp"

namespace ams::os {

    void InitializeInterruptEvent(InterruptEventType *event, InterruptName name, EventClearMode clear_mode) {
        /* Initialize member variables. */
        event->clear_mode = static_cast<u8>(clear_mode);

        /* Initialize implementation. */
        new (GetPointer(event->impl)) impl::InterruptEventImpl(name, clear_mode);

        /* Mark initialized. */
        event->state = InterruptEventType::State_Initialized;
    }

    void FinalizeInterruptEvent(InterruptEventType *event) {
        AMS_ASSERT(event->state == InterruptEventType::State_Initialized);

        /* Mark uninitialized. */
        event->state = InterruptEventType::State_NotInitialized;

        /* Destroy objects. */
        GetReference(event->impl).~InterruptEventImpl();
    }

    void WaitInterruptEvent(InterruptEventType *event) {
        AMS_ASSERT(event->state == InterruptEventType::State_Initialized);
        return GetReference(event->impl).Wait();
    }

    bool TryWaitInterruptEvent(InterruptEventType *event) {
        AMS_ASSERT(event->state == InterruptEventType::State_Initialized);
        return GetReference(event->impl).TryWait();
    }

    bool TimedWaitInterruptEvent(InterruptEventType *event, TimeSpan timeout) {
        AMS_ASSERT(event->state == InterruptEventType::State_Initialized);
        AMS_ASSERT(timeout.GetNanoSeconds() >= 0);
        return GetReference(event->impl).TimedWait(timeout);
    }

    void ClearInterruptEvent(InterruptEventType *event) {
        AMS_ASSERT(event->state == InterruptEventType::State_Initialized);
        return GetReference(event->impl).Clear();
    }

    void InitializeWaitableHolder(WaitableHolderType *waitable_holder, InterruptEventType *event) {
        AMS_ASSERT(event->state == InterruptEventType::State_Initialized);

        new (GetPointer(waitable_holder->impl_storage)) impl::WaitableHolderOfInterruptEvent(event);

        waitable_holder->user_data = 0;
    }

}
