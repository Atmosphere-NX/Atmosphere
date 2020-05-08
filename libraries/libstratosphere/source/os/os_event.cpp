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
#include "impl/os_timeout_helper.hpp"
#include "impl/os_waitable_object_list.hpp"
#include "impl/os_waitable_holder_impl.hpp"

namespace ams::os {

    namespace {

        ALWAYS_INLINE u64 GetBroadcastCounterUnsafe(EventType *event) {
            const u64 upper = event->broadcast_counter_high;
            return (upper << BITSIZEOF(event->broadcast_counter_low)) | event->broadcast_counter_low;
        }

        ALWAYS_INLINE void IncrementBroadcastCounterUnsafe(EventType *event) {
            if ((++event->broadcast_counter_low) == 0) {
                ++event->broadcast_counter_high;
            }
        }

    }

    void InitializeEvent(EventType *event, bool signaled, EventClearMode clear_mode) {
        /* Initialize internal variables. */
        new (GetPointer(event->cs_event))    impl::InternalCriticalSection;
        new (GetPointer(event->cv_signaled)) impl::InternalConditionVariable;

        /* Initialize the waitable object list. */
        new (GetPointer(event->waitable_object_list_storage)) impl::WaitableObjectList();

        /* Initialize member variables. */
        event->signaled               = signaled;
        event->initially_signaled     = signaled;
        event->clear_mode             = static_cast<u8>(clear_mode);
        event->broadcast_counter_low  = 0;
        event->broadcast_counter_high = 0;

        /* Mark initialized. */
        event->state = EventType::State_Initialized;
    }

    void FinalizeEvent(EventType *event) {
        AMS_ASSERT(event->state == EventType::State_Initialized);

        /* Mark uninitialized. */
        event->state = EventType::State_NotInitialized;

        /* Destroy objects. */
        GetReference(event->waitable_object_list_storage).~WaitableObjectList();
        GetReference(event->cv_signaled).~InternalConditionVariable();
        GetReference(event->cs_event).~InternalCriticalSection();
    }

    void SignalEvent(EventType *event) {
        AMS_ASSERT(event->state == EventType::State_Initialized);

        std::scoped_lock lk(GetReference(event->cs_event));

        /* If we're already signaled, nothing more to do. */
        if (event->signaled) {
            return;
        }

        event->signaled = true;

        /* Signal! */
        if (event->clear_mode == EventClearMode_ManualClear) {
            /* If we're manual clear, increment counter and wake all. */
            IncrementBroadcastCounterUnsafe(event);
            GetReference(event->cv_signaled).Broadcast();
        } else {
            /* If we're auto clear, signal one thread, which will clear. */
            GetReference(event->cv_signaled).Signal();
        }

        /* Wake up whatever manager, if any. */
        GetReference(event->waitable_object_list_storage).SignalAllThreads();
    }

    void WaitEvent(EventType *event) {
        AMS_ASSERT(event->state == EventType::State_Initialized);

        std::scoped_lock lk(GetReference(event->cs_event));

        const auto cur_counter = GetBroadcastCounterUnsafe(event);
        while (!event->signaled) {
            if (cur_counter != GetBroadcastCounterUnsafe(event)) {
                break;
            }
            GetReference(event->cv_signaled).Wait(GetPointer(event->cs_event));
        }

        if (event->clear_mode == EventClearMode_AutoClear) {
            event->signaled = false;
        }
    }

    bool TryWaitEvent(EventType *event) {
        AMS_ASSERT(event->state == EventType::State_Initialized);

        std::scoped_lock lk(GetReference(event->cs_event));

        const bool signaled = event->signaled;
        if (event->clear_mode == EventClearMode_AutoClear) {
            event->signaled = false;
        }

        return signaled;
    }

    bool TimedWaitEvent(EventType *event, TimeSpan timeout) {
        AMS_ASSERT(event->state == EventType::State_Initialized);
        AMS_ASSERT(timeout.GetNanoSeconds() >= 0);

        {
            impl::TimeoutHelper timeout_helper(timeout);
            std::scoped_lock lk(GetReference(event->cs_event));

            const auto cur_counter = GetBroadcastCounterUnsafe(event);
            while (!event->signaled) {
                if (cur_counter != GetBroadcastCounterUnsafe(event)) {
                    break;
                }

                auto wait_res = GetReference(event->cv_signaled).TimedWait(GetPointer(event->cs_event), timeout_helper);
                if (wait_res == ConditionVariableStatus::TimedOut) {
                    return false;
                }
            }

            if (event->clear_mode == EventClearMode_AutoClear) {
                event->signaled = false;
            }
        }

        return true;
    }

    void ClearEvent(EventType *event) {
        AMS_ASSERT(event->state == EventType::State_Initialized);

        std::scoped_lock lk(GetReference(event->cs_event));

        /* Clear the signaled state. */
        event->signaled = false;
    }

    void InitializeWaitableHolder(WaitableHolderType *waitable_holder, EventType *event) {
        AMS_ASSERT(event->state == EventType::State_Initialized);

        new (GetPointer(waitable_holder->impl_storage)) impl::WaitableHolderOfEvent(event);

        waitable_holder->user_data = 0;
    }

}
