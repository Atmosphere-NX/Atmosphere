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

        ALWAYS_INLINE bool IsMessageQueueFull(const MessageQueueType *mq) {
            return mq->count >= mq->capacity;
        }

        ALWAYS_INLINE bool IsMessageQueueEmpty(const MessageQueueType *mq) {
            return mq->count == 0;
        }

        void SendUnsafe(MessageQueueType *mq, uintptr_t data) {
            /* Ensure our limits are correct. */
            auto count    = mq->count;
            auto capacity = mq->capacity;
            AMS_ASSERT(count < capacity);

            /* Determine where we're writing. */
            auto ind = mq->offset + count;
            if (ind >= capacity) {
                ind -= capacity;
            }
            AMS_ASSERT(0 <= ind && ind < capacity);

            /* Write the data. */
            mq->buffer[ind] = data;
            ++count;

            /* Update tracking. */
            mq->count = count;
        }

        void SendNextUnsafe(MessageQueueType *mq, uintptr_t data) {
            /* Ensure our limits are correct. */
            auto count    = mq->count;
            auto capacity = mq->capacity;
            AMS_ASSERT(count < capacity);

            /* Determine where we're writing. */
            auto offset = mq->offset - 1;
            if (offset < 0) {
                offset += capacity;
            }
            AMS_ASSERT(0 <= offset && offset < capacity);

            /* Write the data. */
            mq->buffer[offset] = data;
            ++count;

            /* Update tracking. */
            mq->offset = offset;
            mq->count  = count;
        }

        uintptr_t ReceiveUnsafe(MessageQueueType *mq) {
            /* Ensure our limits are correct. */
            auto count    = mq->count;
            auto offset   = mq->offset;
            auto capacity = mq->capacity;
            AMS_ASSERT(count > 0);
            AMS_ASSERT(offset >= 0 && offset < capacity);

            /* Get the data. */
            auto data = mq->buffer[offset];

            /* Calculate new tracking variables. */
            if ((++offset) >= capacity) {
                offset -= capacity;
            }
            --count;

            /* Update tracking. */
            mq->offset = offset;
            mq->count  = count;

            return data;
        }

        uintptr_t PeekUnsafe(const MessageQueueType *mq) {
            /* Ensure our limits are correct. */
            auto count    = mq->count;
            auto offset   = mq->offset;
            AMS_ASSERT(count > 0);

            return mq->buffer[offset];
        }

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

            while (IsMessageQueueFull(mq)) {
                GetReference(mq->cv_not_full).Wait(GetPointer(mq->cs_queue));
            }

            /* Send, signal. */
            SendUnsafe(mq, data);
            GetReference(mq->cv_not_empty).Broadcast();
            GetReference(mq->waitlist_not_empty).SignalAllThreads();
        }
    }

    bool TrySendMessageQueue(MessageQueueType *mq, uintptr_t data) {
        AMS_ASSERT(mq->state == MessageQueueType::State_Initialized);

        {
            /* Acquire mutex, check sendable. */
            std::scoped_lock lk(GetReference(mq->cs_queue));

            if (IsMessageQueueFull(mq)) {
                return false;
            }

            /* Send, signal. */
            SendUnsafe(mq, data);
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

            while (IsMessageQueueFull(mq)) {
                if (timeout_helper.TimedOut()) {
                    return false;
                }
                GetReference(mq->cv_not_full).TimedWait(GetPointer(mq->cs_queue), timeout_helper);
            }

            /* Send, signal. */
            SendUnsafe(mq, data);
            GetReference(mq->cv_not_empty).Broadcast();
            GetReference(mq->waitlist_not_empty).SignalAllThreads();
        }

        return true;
    }

    /* Sending (LIFO functionality) */
    void SendNextMessageQueue(MessageQueueType *mq, uintptr_t data) {
        AMS_ASSERT(mq->state == MessageQueueType::State_Initialized);

        {
            /* Acquire mutex, wait sendable. */
            std::scoped_lock lk(GetReference(mq->cs_queue));

            while (IsMessageQueueFull(mq)) {
                GetReference(mq->cv_not_full).Wait(GetPointer(mq->cs_queue));
            }

            /* Send, signal. */
            SendNextUnsafe(mq, data);
            GetReference(mq->cv_not_empty).Broadcast();
            GetReference(mq->waitlist_not_empty).SignalAllThreads();
        }
    }

    bool TrySendNextMessageQueue(MessageQueueType *mq, uintptr_t data) {
        AMS_ASSERT(mq->state == MessageQueueType::State_Initialized);

        {
            /* Acquire mutex, check sendable. */
            std::scoped_lock lk(GetReference(mq->cs_queue));

            if (IsMessageQueueFull(mq)) {
                return false;
            }

            /* Send, signal. */
            SendNextUnsafe(mq, data);
            GetReference(mq->cv_not_empty).Broadcast();
            GetReference(mq->waitlist_not_empty).SignalAllThreads();
        }

        return true;
    }

    bool TimedSendNextMessageQueue(MessageQueueType *mq, uintptr_t data, TimeSpan timeout) {
        AMS_ASSERT(mq->state == MessageQueueType::State_Initialized);
        AMS_ASSERT(timeout.GetNanoSeconds() >= 0);

        {
            /* Acquire mutex, wait sendable. */
            impl::TimeoutHelper timeout_helper(timeout);
            std::scoped_lock lk(GetReference(mq->cs_queue));

            while (IsMessageQueueFull(mq)) {
                if (timeout_helper.TimedOut()) {
                    return false;
                }
                GetReference(mq->cv_not_full).TimedWait(GetPointer(mq->cs_queue), timeout_helper);
            }

            /* Send, signal. */
            SendNextUnsafe(mq, data);
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

            while (IsMessageQueueEmpty(mq)) {
                GetReference(mq->cv_not_empty).Wait(GetPointer(mq->cs_queue));
            }

            /* Receive, signal. */
            *out = ReceiveUnsafe(mq);
            GetReference(mq->cv_not_full).Broadcast();
            GetReference(mq->waitlist_not_full).SignalAllThreads();
        }
    }

    bool TryReceiveMessageQueue(uintptr_t *out, MessageQueueType *mq) {
        AMS_ASSERT(mq->state == MessageQueueType::State_Initialized);

        {
            /* Acquire mutex, check receivable. */
            std::scoped_lock lk(GetReference(mq->cs_queue));

            if (IsMessageQueueEmpty(mq)) {
                return false;
            }

            /* Receive, signal. */
            *out = ReceiveUnsafe(mq);
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

            while (IsMessageQueueEmpty(mq)) {
                if (timeout_helper.TimedOut()) {
                    return false;
                }
                GetReference(mq->cv_not_empty).TimedWait(GetPointer(mq->cs_queue), timeout_helper);
            }

            /* Receive, signal. */
            *out = ReceiveUnsafe(mq);
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

            while (IsMessageQueueEmpty(mq)) {
                GetReference(mq->cv_not_empty).Wait(GetPointer(mq->cs_queue));
            }

            /* Peek. */
            *out = PeekUnsafe(mq);
        }
    }

    bool TryPeekMessageQueue(uintptr_t *out, const MessageQueueType *mq) {
        AMS_ASSERT(mq->state == MessageQueueType::State_Initialized);

        {
            /* Acquire mutex, check receivable. */
            std::scoped_lock lk(GetReference(mq->cs_queue));

            if (IsMessageQueueEmpty(mq)) {
                return false;
            }

            /* Peek. */
            *out = PeekUnsafe(mq);
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

            while (IsMessageQueueEmpty(mq)) {
                if (timeout_helper.TimedOut()) {
                    return false;
                }
                GetReference(mq->cv_not_empty).TimedWait(GetPointer(mq->cs_queue), timeout_helper);
            }

            /* Peek. */
            *out = PeekUnsafe(mq);
        }

        return true;
    }

    void InitializeWaitableHolder(WaitableHolderType *waitable_holder, MessageQueueType *mq, MessageQueueWaitType type) {
        AMS_ASSERT(mq->state == MessageQueueType::State_Initialized);

        switch (type) {
            case MessageQueueWaitType::ForNotFull:
                util::ConstructAt(GetReference(waitable_holder->impl_storage).holder_of_mq_for_not_full_storage, mq);
                break;
            case MessageQueueWaitType::ForNotEmpty:
                util::ConstructAt(GetReference(waitable_holder->impl_storage).holder_of_mq_for_not_empty_storage, mq);
                break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }

        waitable_holder->user_data = 0;
    }

}
