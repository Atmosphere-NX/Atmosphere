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
#include "os_common_types.hpp"

namespace ams::os {

    class WaitableManager;

    class Event;
    class SystemEvent;
    class InterruptEvent;
    class Thread;
    class MessageQueue;
    class Semaphore;

    namespace impl {

        class WaitableHolderImpl;

    }

    class WaitableHolder {
        friend class WaitableManager;
        NON_COPYABLE(WaitableHolder);
        NON_MOVEABLE(WaitableHolder);
        private:
            util::TypedStorage<impl::WaitableHolderImpl, 2 * sizeof(util::IntrusiveListNode) + 3 * sizeof(void *), alignof(void *)> impl_storage;
            uintptr_t user_data;
        public:
            static constexpr size_t ImplStorageSize = sizeof(impl_storage);
        public:
            WaitableHolder(Handle handle);
            WaitableHolder(Event *event);
            WaitableHolder(SystemEvent *event);
            WaitableHolder(InterruptEvent *event);
            WaitableHolder(Thread *thread);
            WaitableHolder(Semaphore *semaphore);
            WaitableHolder(MessageQueue *message_queue, MessageQueueWaitKind wait_kind);

            ~WaitableHolder();

            void SetUserData(uintptr_t data) {
                this->user_data = data;
            }

            uintptr_t GetUserData() const {
                return this->user_data;
            }

            void UnlinkFromWaitableManager();
    };

}
