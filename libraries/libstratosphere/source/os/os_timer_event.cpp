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
#include "impl/os_timer_event_helper.hpp"
#include "impl/os_tick_manager.hpp"
#include "impl/os_timeout_helper.hpp"
#include "impl/os_multiple_wait_object_list.hpp"
#include "impl/os_multiple_wait_holder_impl.hpp"

namespace ams::os {

    namespace {

        ALWAYS_INLINE u64 GetBroadcastCounterUnsafe(TimerEventType *event) {
            const u64 upper = event->broadcast_counter_high;
            return (upper << BITSIZEOF(event->broadcast_counter_low)) | event->broadcast_counter_low;
        }

        ALWAYS_INLINE void IncrementBroadcastCounterUnsafe(TimerEventType *event) {
            if ((++event->broadcast_counter_low) == 0) {
                ++event->broadcast_counter_high;
            }
        }

        ALWAYS_INLINE void SignalTimerEventImplUnsafe(TimerEventType *event) {
            /* Set signaled. */
            event->signaled = true;

            /* Signal! */
            if (event->clear_mode == EventClearMode_ManualClear) {
                /* If we're manual clear, increment counter and wake all. */
                IncrementBroadcastCounterUnsafe(event);
            } else {
                /* If we're auto clear, signal one thread, which will clear. */
                GetReference(event->cv_signaled).Signal();
            }

            /* Wake up whatever manager, if any. */
            GetReference(event->multi_wait_object_list_storage).SignalAllThreads();
        }

    }

    void InitializeTimerEvent(TimerEventType *event, EventClearMode clear_mode) {
        /* Initialize internal variables. */
        util::ConstructAt(event->cs_timer_event);
        util::ConstructAt(event->cv_signaled);

        /* Initialize the multi wait object list. */
        util::ConstructAt(event->multi_wait_object_list_storage);

        /* Initialize member variables. */
        event->clear_mode             = static_cast<u8>(clear_mode);
        event->signaled               = false;
        event->timer_state            = TimerEventType::TimerState_Stop;
        event->broadcast_counter_low  = 0;
        event->broadcast_counter_high = 0;

        GetReference(event->next_time_to_wakeup) = 0;
        GetReference(event->first)               = 0;
        GetReference(event->interval)            = 0;

        /* Mark initialized. */
        event->state = TimerEventType::State_Initialized;
    }

    void FinalizeTimerEvent(TimerEventType *event) {

        /* Mark uninitialized. */
        event->state = TimerEventType::State_NotInitialized;

        /* Destroy objects. */
        util::DestroyAt(event->multi_wait_object_list_storage);
        util::DestroyAt(event->cv_signaled);
        util::DestroyAt(event->cs_timer_event);
    }

    void StartOneShotTimerEvent(TimerEventType *event, TimeSpan first_time) {
        AMS_ASSERT(event->state == TimerEventType::State_Initialized);
        AMS_ASSERT(first_time >= 0);

        {
            std::scoped_lock lk(GetReference(event->cs_timer_event));

            /* Get the current time. */
            TimeSpan cur_time = impl::GetCurrentTick().ToTimeSpan();

            /* Set tracking timespans. */
            GetReference(event->next_time_to_wakeup) = impl::SaturatedAdd(cur_time, first_time);
            GetReference(event->first)               = first_time;
            GetReference(event->interval)            = 0;

            /* Set state to OneShot. */
            event->timer_state = TimerEventType::TimerState_OneShot;

            /* Signal. */
            GetReference(event->cv_signaled).Broadcast();

            /* Wake up whatever manager, if any. */
            GetReference(event->multi_wait_object_list_storage).SignalAllThreads();
        }
    }

    void StartPeriodicTimerEvent(TimerEventType *event, TimeSpan first_time, TimeSpan interval) {
        AMS_ASSERT(event->state == TimerEventType::State_Initialized);
        AMS_ASSERT(first_time >= 0);
        AMS_ASSERT(interval > 0);

        {
            std::scoped_lock lk(GetReference(event->cs_timer_event));

            /* Get the current time. */
            TimeSpan cur_time = impl::GetCurrentTick().ToTimeSpan();

            /* Set tracking timespans. */
            GetReference(event->next_time_to_wakeup) = impl::SaturatedAdd(cur_time, first_time);
            GetReference(event->first)               = first_time;
            GetReference(event->interval)            = interval;

            /* Set state to Periodic. */
            event->timer_state = TimerEventType::TimerState_Periodic;

            /* Signal. */
            GetReference(event->cv_signaled).Broadcast();

            /* Wake up whatever manager, if any. */
            GetReference(event->multi_wait_object_list_storage).SignalAllThreads();
        }
    }

    void StopTimerEvent(TimerEventType *event) {
        AMS_ASSERT(event->state == TimerEventType::State_Initialized);

        {
            std::scoped_lock lk(GetReference(event->cs_timer_event));

            /* Stop the event. */
            impl::StopTimerUnsafe(event);

            /* Signal. */
            GetReference(event->cv_signaled).Broadcast();

            /* Wake up whatever manager, if any. */
            GetReference(event->multi_wait_object_list_storage).SignalAllThreads();
        }
    }

    void WaitTimerEvent(TimerEventType *event) {
        AMS_ASSERT(event->state == TimerEventType::State_Initialized);

        {
            std::scoped_lock lk(GetReference(event->cs_timer_event));

            const auto cur_counter = GetBroadcastCounterUnsafe(event);
            while (!event->signaled) {
                if (cur_counter != GetBroadcastCounterUnsafe(event)) {
                    break;
                }

                /* Update. */
                auto cur_time = impl::GetCurrentTick().ToTimeSpan();
                if (impl::UpdateSignalStateAndRecalculateNextTimeToWakeupUnsafe(event, cur_time)) {
                    SignalTimerEventImplUnsafe(event);
                    break;
                }

                /* Check state. */
                if (event->timer_state == TimerEventType::TimerState_Stop) {
                    GetReference(event->cv_signaled).Wait(GetPointer(event->cs_timer_event));
                } else {
                    TimeSpan next_time = GetReference(event->next_time_to_wakeup);
                    impl::TimeoutHelper helper(next_time - cur_time);
                    GetReference(event->cv_signaled).TimedWait(GetPointer(event->cs_timer_event), helper);
                }
            }

            if (event->clear_mode == EventClearMode_AutoClear) {
                event->signaled = false;
            }
        }
    }

    bool TryWaitTimerEvent(TimerEventType *event) {
        AMS_ASSERT(event->state == TimerEventType::State_Initialized);

        {
            std::scoped_lock lk(GetReference(event->cs_timer_event));

            /* Update. */
            auto cur_time = impl::GetCurrentTick().ToTimeSpan();
            if (impl::UpdateSignalStateAndRecalculateNextTimeToWakeupUnsafe(event, cur_time)) {
                SignalTimerEventImplUnsafe(event);
            }

            bool prev = event->signaled;

            if (event->clear_mode == EventClearMode_AutoClear) {
                event->signaled = false;
            }

            return prev;
        }
    }

    void SignalTimerEvent(TimerEventType *event) {
        AMS_ASSERT(event->state == TimerEventType::State_Initialized);

        {
            std::scoped_lock lk(GetReference(event->cs_timer_event));

            /* If we're signaled, nothing to do. */
            if (event->signaled) {
                return;
            }

            /* Update. */
            auto cur_time = impl::GetCurrentTick().ToTimeSpan();
            impl::UpdateSignalStateAndRecalculateNextTimeToWakeupUnsafe(event, cur_time);

            /* Signal. */
            SignalTimerEventImplUnsafe(event);
        }
    }

    void ClearTimerEvent(TimerEventType *event) {
        AMS_ASSERT(event->state == TimerEventType::State_Initialized);

        {
            std::scoped_lock lk(GetReference(event->cs_timer_event));

            /* Update. */
            auto cur_time = impl::GetCurrentTick().ToTimeSpan();
            if (impl::UpdateSignalStateAndRecalculateNextTimeToWakeupUnsafe(event, cur_time)) {
                SignalTimerEventImplUnsafe(event);
            }

            /* Clear. */
            event->signaled = false;
        }
    }

    void InitializeMultiWaitHolder(MultiWaitHolderType *multi_wait_holder, TimerEventType *event) {
        AMS_ASSERT(event->state == EventType::State_Initialized);

        util::ConstructAt(GetReference(multi_wait_holder->impl_storage).holder_of_timer_event_storage, event);

        multi_wait_holder->user_data = 0;
    }

}
