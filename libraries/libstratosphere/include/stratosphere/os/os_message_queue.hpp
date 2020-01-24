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

namespace ams::os {

    namespace impl {

        class WaitableObjectList;

        template<MessageQueueWaitKind WaitKind>
        class WaitableHolderOfMessageQueue;

    }

    class MessageQueue {
        template<MessageQueueWaitKind WaitKind>
        friend class impl::WaitableHolderOfMessageQueue;
        NON_COPYABLE(MessageQueue);
        NON_MOVEABLE(MessageQueue);
        private:
            util::TypedStorage<impl::WaitableObjectList, sizeof(util::IntrusiveListNode), alignof(util::IntrusiveListNode)> waitlist_not_empty;
            util::TypedStorage<impl::WaitableObjectList, sizeof(util::IntrusiveListNode), alignof(util::IntrusiveListNode)> waitlist_not_full;
            Mutex queue_lock;
            ConditionVariable cv_not_full;
            ConditionVariable cv_not_empty;
            std::unique_ptr<uintptr_t[]> buffer;
            size_t capacity;

            size_t count;
            size_t offset;
        private:
            constexpr inline bool IsFull() const {
                return this->count >= this->capacity;
            }

            constexpr inline bool IsEmpty() const {
                return this->count == 0;
            }

            void SendInternal(uintptr_t data);
            void SendNextInternal(uintptr_t data);
            uintptr_t ReceiveInternal();
            uintptr_t PeekInternal();
        public:
            MessageQueue(std::unique_ptr<uintptr_t[]> buf, size_t c);
            ~MessageQueue();

            /* For convenience. */
            MessageQueue(size_t c) : MessageQueue(std::make_unique<uintptr_t[]>(c), c) { /* ... */ }

            /* Sending (FIFO functionality) */
            void Send(uintptr_t data);
            bool TrySend(uintptr_t data);
            bool TimedSend(uintptr_t data, u64 timeout);

            /* Sending (LIFO functionality) */
            void SendNext(uintptr_t data);
            bool TrySendNext(uintptr_t data);
            bool TimedSendNext(uintptr_t data, u64 timeout);

            /* Receive functionality */
            void Receive(uintptr_t *out);
            bool TryReceive(uintptr_t *out);
            bool TimedReceive(uintptr_t *out, u64 timeout);

            /* Peek functionality */
            void Peek(uintptr_t *out);
            bool TryPeek(uintptr_t *out);
            bool TimedPeek(uintptr_t *out, u64 timeout);
    };

}
