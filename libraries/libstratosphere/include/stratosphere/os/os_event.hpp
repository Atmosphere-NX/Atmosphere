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
#include "os_mutex.hpp"
#include "os_condvar.hpp"
#include "os_timeout_helper.hpp"

namespace ams::os {

    namespace impl {

        class WaitableObjectList;
        class WaitableHolderOfEvent;

    }

    class Event {
        friend class impl::WaitableHolderOfEvent;
        NON_COPYABLE(Event);
        NON_MOVEABLE(Event);
        private:
            util::TypedStorage<impl::WaitableObjectList, sizeof(util::IntrusiveListNode), alignof(util::IntrusiveListNode)> waitable_object_list_storage;
            Mutex lock;
            ConditionVariable cv;
            u64 counter = 0;
            bool auto_clear;
            bool signaled;
        public:
            Event(bool a = true, bool s = false);
            ~Event();

            void Signal();
            void Reset();
            void Wait();
            bool TryWait();
            bool TimedWait(u64 ns);
    };

}
