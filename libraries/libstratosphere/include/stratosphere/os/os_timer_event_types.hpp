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
#include <vapours.hpp>
#include <stratosphere/os/os_event_common.hpp>
#include <stratosphere/os/impl/os_internal_critical_section.hpp>
#include <stratosphere/os/impl/os_internal_condition_variable.hpp>

namespace ams::os {

    namespace impl {

        class WaitableObjectList;

    }

    struct TimerEventType {
        using TimeSpanStorage = TYPED_STORAGE(TimeSpan);

        enum State {
            State_NotInitialized = 0,
            State_Initialized    = 1,
        };

        enum TimerState {
            TimerState_Stop     = 0,
            TimerState_OneShot  = 1,
            TimerState_Periodic = 2,
        };

        util::TypedStorage<impl::WaitableObjectList, sizeof(util::IntrusiveListNode), alignof(util::IntrusiveListNode)> waitable_object_list_storage;

        u8 state;
        u8 clear_mode;
        bool signaled;
        u8 timer_state;
        u32 broadcast_counter_low;
        u32 broadcast_counter_high;

        TimeSpanStorage next_time_to_wakeup;
        TimeSpanStorage first;
        TimeSpanStorage interval;

        impl::InternalCriticalSectionStorage cs_timer_event;
        impl::InternalConditionVariableStorage cv_signaled;
    };
    static_assert(std::is_trivial<TimerEventType>::value);

}
