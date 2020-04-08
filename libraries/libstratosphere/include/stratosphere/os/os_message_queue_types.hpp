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
#include <stratosphere/os/impl/os_internal_critical_section.hpp>
#include <stratosphere/os/impl/os_internal_condition_variable.hpp>

namespace ams::os {

    namespace impl {

        class WaitableObjectList;

    }

    struct MessageQueueType {
        enum State {
            State_NotInitialized = 0,
            State_Initialized    = 1,
        };

        util::TypedStorage<impl::WaitableObjectList, sizeof(util::IntrusiveListNode), alignof(util::IntrusiveListNode)> waitlist_not_full;
        util::TypedStorage<impl::WaitableObjectList, sizeof(util::IntrusiveListNode), alignof(util::IntrusiveListNode)> waitlist_not_empty;
        uintptr_t *buffer;
        s32 capacity;
        s32 count;
        s32 offset;
        u8 state;

        mutable impl::InternalCriticalSectionStorage cs_queue;
        mutable impl::InternalConditionVariableStorage cv_not_full;
        mutable impl::InternalConditionVariableStorage cv_not_empty;
    };
    static_assert(std::is_trivial<MessageQueueType>::value);

}
