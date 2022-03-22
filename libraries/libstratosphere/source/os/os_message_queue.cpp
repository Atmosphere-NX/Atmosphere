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
#include "impl/os_timeout_helper.hpp"
#include "impl/os_multiple_wait_object_list.hpp"
#include "impl/os_multiple_wait_holder_impl.hpp"
#include "impl/os_message_queue_helper.hpp"

namespace ams::os {

    namespace {

        using MessageQueueHelper = impl::MessageQueueHelper<MessageQueueType>;

    }

    void InitializeMessageQueue(MessageQueueType *mq, uintptr_t *buffer, size_t count) {
        AMS_ASSERT(buffer != nullptr);
        AMS_ASSERT(count >= 1);

        /* Setup objects. */
        util::ConstructAt(mq->cs_queue);
        util::ConstructAt(mq->cv_not_full);
        util::ConstructAt(mq->cv_not_empty);

        /* Setup wait lists. */
        util::ConstructAt(mq->waitlist_not_empty);
        util::ConstructAt(mq->waitlist_not_full);

        /* Set member variables. */
        mq->buffer   = buffer;
        mq->capacity = static_cast<s32>(count);
        mq->count    = 0;
        mq->offset   = 0;

        /* Mark initialized. */
        mq->state = MessageQueueType::State_Initialized;
    }

    void FinalizeMessageQueue(MessageQueueType *mq) {
        AMS_ASSERT(mq->state = MessageQueueType::State_Initialized);

        AMS_ASSERT(GetReference(mq->waitlist_not_empty).IsEmpty());
        AMS_ASSERT(GetReference(mq->waitlist_not_full).IsEmpty());

        /* Mark uninitialized. */
        mq->state = MessageQueueType::State_NotInitialized;

        /* Destroy wait lists. */
        util::DestroyAt(mq->waitlist_not_empty);
        util::DestroyAt(mq->waitlist_not_full);

        /* Destroy objects. */
        util::DestroyAt(mq->cv_not_empty);
        util::DestroyAt(mq->cv_not_full);
        util::DestroyAt(mq->cs_queue);
    }

    /* Sending (FIFO functionality) */
    void SendMessageQueue(MessageQueueType *mq, uintptr_t data) {
        AMS_ASSERT(mq->state == MessageQueueType::State_Initialized);

        {
            /* Acquire mutex, wait sendable. */
            std::scoped_lock lk(GetReference(mq->cs_queue));

            while (MessageQueueHelper::IsMessageQueueFull(mq)) {
                GetReference(mq->cv_not_full).Wait(GetPointer(mq->cs_queue));
            }

            /* Send, signal. */
            MessageQueueHelper::EnqueueUnsafe(mq, data);
            GetReference(mq->cv_not_empty).Broadcast();
            GetReference(mq->waitlist_not_empty).SignalAllThreads();
        }
    }

    bool TrySendMessageQueue(MessageQueueType *mq, uintptr_t data) {
        AMS_ASSERT(mq->state == MessageQueueType::State_Initialized);

        {
            /* Acquire mutex, check sendable. */
            std::scoped_lock lk(GetReference(mq->cs_queue));

            if (MessageQueueHelper::IsMessageQueueFull(mq)) {
                return false;
            }

            /* Send, signal. */
            MessageQueueHelper::EnqueueUnsafe(mq, data);
            GetReference(mq->cv_not_empty).Broadcast();
            GetReference(mq->waitlist_not_empty).SignalAllThreads();
        }

        return true;
    }

    bool TimedSendMessageQueue(MessageQueueType *mq, uintptr_t data, TimeSpan timeout) {
        AMS_ASSERT(mq->state == MessageQueueType::State_Initialized);
        AMS_ASSERT(timeout.GetNanoSeconds() >= 0);

        {
            /* Acquire mutex, wait sendable. */
            impl::TimeoutHelper timeout_helper(timeout);
            std::scoped_lock lk(GetReference(mq->cs_queue));

            while (MessageQueueHelper::IsMessageQueueFull(mq)) {
                if (timeout_helper.TimedOut()) {
                    return false;
                }
                GetReference(mq->cv_not_full).TimedWait(GetPointer(mq->cs_queue), timeout_helper);
            }

            /* Send, signal. */
            MessageQueueHelper::EnqueueUnsafe(mq, data);
            GetReference(mq->cv_not_empty).Broadcast();
            GetReference(mq->waitlist_not_empty).SignalAllThreads();
        }

        return true;
    }

    /* Jamming (LIFO functionality) */
    void JamMessageQueue(MessageQueueType *mq, uintptr_t data) {
        AMS_ASSERT(mq->state == MessageQueueType::State_Initialized);

        {
            /* Acquire mutex, wait sendable. */
            std::scoped_lock lk(GetReference(mq->cs_queue));

            while (MessageQueueHelper::IsMessageQueueFull(mq)) {
                GetReference(mq->cv_not_full).Wait(GetPointer(mq->cs_queue));
            }

            /* Send, signal. */
            MessageQueueHelper::JamUnsafe(mq, data);
            GetReference(mq->cv_not_empty).Broadcast();
            GetReference(mq->waitlist_not_empty).SignalAllThreads();
        }
    }

    bool TryJamMessageQueue(MessageQueueType *mq, uintptr_t data) {
        AMS_ASSERT(mq->state == MessageQueueType::State_Initialized);

        {
            /* Acquire mutex, check sendable. */
            std::scoped_lock lk(GetReference(mq->cs_queue));

            if (MessageQueueHelper::IsMessageQueueFull(mq)) {
                return false;
            }

            /* Send, signal. */
            MessageQueueHelper::JamUnsafe(mq, data);
            GetReference(mq->cv_not_empty).Broadcast();
            GetReference(mq->waitlist_not_empty).SignalAllThreads();
        }

        return true;
    }

    bool TimedJamMessageQueue(MessageQueueType *mq, uintptr_t data, TimeSpan timeout) {
        AMS_ASSERT(mq->state == MessageQueueType::State_Initialized);
        AMS_ASSERT(timeout.GetNanoSeconds() >= 0);

        {
            /* Acquire mutex, wait sendable. */
            impl::TimeoutHelper timeout_helper(timeout);
            std::scoped_lock lk(GetReference(mq->cs_queue));

            while (MessageQueueHelper::IsMessageQueueFull(mq)) {
                if (timeout_helper.TimedOut()) {
                    return false;
                }
                GetReference(mq->cv_not_full).TimedWait(GetPointer(mq->cs_queue), timeout_helper);
            }

            /* Send, signal. */
            MessageQueueHelper::JamUnsafe(mq, data);
            GetReference(mq->cv_not_empty).Broadcast();
            GetReference(mq->waitlist_not_empty).SignalAllThreads();
        }

        return true;
    }

    /* Receive functionality */
    void ReceiveMessageQueue(uintptr_t *out, MessageQueueType *mq) {
        AMS_ASSERT(mq->state == MessageQueueType::State_Initialized);

        {
            /* Acquire mutex, wait receivable. */
            std::scoped_lock lk(GetReference(mq->cs_queue));

            while (MessageQueueHelper::IsMessageQueueEmpty(mq)) {
                GetReference(mq->cv_not_empty).Wait(GetPointer(mq->cs_queue));
            }

            /* Receive, signal. */
            *out = MessageQueueHelper::DequeueUnsafe(mq);
            GetReference(mq->cv_not_full).Broadcast();
            GetReference(mq->waitlist_not_full).SignalAllThreads();
        }
    }

    bool TryReceiveMessageQueue(uintptr_t *out, MessageQueueType *mq) {
        AMS_ASSERT(mq->state == MessageQueueType::State_Initialized);

        {
            /* Acquire mutex, check receivable. */
            std::scoped_lock lk(GetReference(mq->cs_queue));

            if (MessageQueueHelper::IsMessageQueueEmpty(mq)) {
                return false;
            }

            /* Receive, signal. */
            *out = MessageQueueHelper::DequeueUnsafe(mq);
            GetReference(mq->cv_not_full).Broadcast();
            GetReference(mq->waitlist_not_full).SignalAllThreads();
        }

        return true;
    }

    bool TimedReceiveMessageQueue(uintptr_t *out, MessageQueueType *mq, TimeSpan timeout) {
        AMS_ASSERT(mq->state == MessageQueueType::State_Initialized);
        AMS_ASSERT(timeout.GetNanoSeconds() >= 0);

        {
            /* Acquire mutex, wait receivable. */
            impl::TimeoutHelper timeout_helper(timeout);
            std::scoped_lock lk(GetReference(mq->cs_queue));

            while (MessageQueueHelper::IsMessageQueueEmpty(mq)) {
                if (timeout_helper.TimedOut()) {
                    return false;
                }
                GetReference(mq->cv_not_empty).TimedWait(GetPointer(mq->cs_queue), timeout_helper);
            }

            /* Receive, signal. */
            *out = MessageQueueHelper::DequeueUnsafe(mq);
            GetReference(mq->cv_not_full).Broadcast();
            GetReference(mq->waitlist_not_full).SignalAllThreads();
        }

        return true;
    }

    /* Peek functionality */
    void PeekMessageQueue(uintptr_t *out, const MessageQueueType *mq) {
        AMS_ASSERT(mq->state == MessageQueueType::State_Initialized);

        {
            /* Acquire mutex, wait receivable. */
            std::scoped_lock lk(GetReference(mq->cs_queue));

            while (MessageQueueHelper::IsMessageQueueEmpty(mq)) {
                GetReference(mq->cv_not_empty).Wait(GetPointer(mq->cs_queue));
            }

            /* Peek. */
            *out = MessageQueueHelper::PeekUnsafe(mq);
        }
    }

    bool TryPeekMessageQueue(uintptr_t *out, const MessageQueueType *mq) {
        AMS_ASSERT(mq->state == MessageQueueType::State_Initialized);

        {
            /* Acquire mutex, check receivable. */
            std::scoped_lock lk(GetReference(mq->cs_queue));

            if (MessageQueueHelper::IsMessageQueueEmpty(mq)) {
                return false;
            }

            /* Peek. */
            *out = MessageQueueHelper::PeekUnsafe(mq);
        }

        return true;
    }

    bool TimedPeekMessageQueue(uintptr_t *out, const MessageQueueType *mq, TimeSpan timeout) {
        AMS_ASSERT(mq->state == MessageQueueType::State_Initialized);
        AMS_ASSERT(timeout.GetNanoSeconds() >= 0);

        {
            /* Acquire mutex, wait receivable. */
            impl::TimeoutHelper timeout_helper(timeout);
            std::scoped_lock lk(GetReference(mq->cs_queue));

            while (MessageQueueHelper::IsMessageQueueEmpty(mq)) {
                if (timeout_helper.TimedOut()) {
                    return false;
                }
                GetReference(mq->cv_not_empty).TimedWait(GetPointer(mq->cs_queue), timeout_helper);
            }

            /* Peek. */
            *out = MessageQueueHelper::PeekUnsafe(mq);
        }

        return true;
    }

    void InitializeMultiWaitHolder(MultiWaitHolderType *multi_wait_holder, MessageQueueType *mq, MessageQueueWaitType type) {
        AMS_ASSERT(mq->state == MessageQueueType::State_Initialized);

        switch (type) {
            case MessageQueueWaitType::ForNotFull:
                util::ConstructAt(GetReference(multi_wait_holder->impl_storage).holder_of_mq_for_not_full_storage, mq);
                break;
            case MessageQueueWaitType::ForNotEmpty:
                util::ConstructAt(GetReference(multi_wait_holder->impl_storage).holder_of_mq_for_not_empty_storage, mq);
                break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }

        multi_wait_holder->user_data = 0;
    }

}
