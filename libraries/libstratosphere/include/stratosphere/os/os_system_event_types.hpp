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
#include <stratosphere/os/os_event_types.hpp>
#include <stratosphere/os/os_native_handle.hpp>

namespace ams::os {

    namespace impl {

        struct InterProcessEventType {
            enum State {
                State_NotInitialized = 0,
                State_Initialized    = 1,
            };

            util::TypedStorage<impl::MultiWaitObjectList, sizeof(util::IntrusiveListNode), alignof(util::IntrusiveListNode)> multi_wait_object_list_storage;

            bool auto_clear;
            u8   state;
            bool is_readable_handle_managed;
            bool is_writable_handle_managed;
            NativeHandle readable_handle;
            NativeHandle writable_handle;
        };
        static_assert(std::is_trivial<InterProcessEventType>::value);

    }

    struct SystemEventType {
        enum State {
            State_NotInitialized                 = 0,
            State_InitializedAsEvent             = 1,
            State_InitializedAsInterProcessEvent = 2,
        };

        union {
            EventType event;
            impl::InterProcessEventType inter_process_event;
        };

        u8 state;
    };
    static_assert(std::is_trivial<SystemEventType>::value);

}
