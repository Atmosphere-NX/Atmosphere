/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include "os_waitable_holder_base.hpp"
#include "os_waitable_object_list.hpp"

namespace sts::os::impl {

    template<MessageQueueWaitKind WaitKind>
    class WaitableHolderOfMessageQueue : public WaitableHolderOfUserObject {
        static_assert(WaitKind == MessageQueueWaitKind::ForNotEmpty || WaitKind == MessageQueueWaitKind::ForNotFull, "MessageQueueHolder WaitKind!");
        private:
            MessageQueue *message_queue;
        private:
            TriBool IsSignaledImpl() const {
                if constexpr (WaitKind == MessageQueueWaitKind::ForNotEmpty) {
                    /* ForNotEmpty. */
                    return this->message_queue->IsEmpty() ? TriBool::False : TriBool::True;
                } else /* if constexpr (WaitKind == MessageQueueWaitKind::ForNotFull) */ {
                    /* ForNotFull */
                    return this->message_queue->IsFull() ? TriBool::False : TriBool::True;
                }
            }
        public:
            explicit WaitableHolderOfMessageQueue(MessageQueue *mq) : message_queue(mq) { /* ... */ }

            /* IsSignaled, Link, Unlink implemented. */
            virtual TriBool IsSignaled() const override {
                std::scoped_lock lk(this->message_queue->queue_lock);
                return this->IsSignaledImpl();
            }

            virtual TriBool LinkToObjectList() override {
                std::scoped_lock lk(this->message_queue->queue_lock);

                GetReference(this->message_queue->waitable_object_list_storage).LinkWaitableHolder(*this);
                return this->IsSignaledImpl();
            }

            virtual void UnlinkFromObjectList() override {
                std::scoped_lock lk(this->message_queue->queue_lock);

                GetReference(this->message_queue->waitable_object_list_storage).UnlinkWaitableHolder(*this);
            }
    };

    using WaitableHolderOfMessageQueueForNotEmpty = WaitableHolderOfMessageQueue<MessageQueueWaitKind::ForNotEmpty>;
    using WaitableHolderOfMessageQueueForNotFull  = WaitableHolderOfMessageQueue<MessageQueueWaitKind::ForNotFull>;

}
