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
#include "impl/os_message_queue_helper.hpp"

namespace ams::os {

    namespace {

        using MessageQueueHelper = impl::MessageQueueHelper<LightMessageQueueType>;

        template<bool ClearEvent, auto EnqueueFunction>
        bool TryEnqueueLightMessageQueueImpl(LightMessageQueueType *mq, uintptr_t data) {
            /* Perform the enqueue. */
            {
                /* Acquire exclusive access to the queue. */
                std::scoped_lock lk(util::GetReference(mq->mutex_queue));

                /* Check that we can enqueue. */
                if (MessageQueueHelper::IsMessageQueueFull(mq)) {
                    /* If we should, clear the event. */
                    if constexpr (ClearEvent) {
                        util::GetReference(mq->ev_not_full).Clear();
                    }

                    /* We can't enqueue because we're full. */
                    return false;
                }

                /* Enqueue the data. */
                EnqueueFunction(mq, data);
            }

            /* Signal that we're not empty. */
            util::GetReference(mq->ev_not_empty).SignalWithManualClear();

            return true;
        }

        template<bool ClearEvent>
        bool TrySendLightMessageQueueImpl(LightMessageQueueType *mq, uintptr_t data) {
            return TryEnqueueLightMessageQueueImpl<ClearEvent, MessageQueueHelper::EnqueueUnsafe>(mq, data);
        }

        template<bool ClearEvent>
        bool TryJamLightMessageQueueImpl(LightMessageQueueType *mq, uintptr_t data) {
            return TryEnqueueLightMessageQueueImpl<ClearEvent, MessageQueueHelper::JamUnsafe>(mq, data);
        }

        template<bool ClearEvent>
        bool TryReceiveLightMessageQueueImpl(uintptr_t *out, LightMessageQueueType *mq) {
            /* Perform the receive. */
            {
                /* Acquire exclusive access to the queue. */
                std::scoped_lock lk(util::GetReference(mq->mutex_queue));

                /* Check that we can receive. */
                if (MessageQueueHelper::IsMessageQueueEmpty(mq)) {
                    /* If we should, clear the event. */
                    if constexpr (ClearEvent) {
                        util::GetReference(mq->ev_not_empty).Clear();
                    }

                    /* We can't enqueue because we're full. */
                    return false;
                }

                /* Receive the data. */
                *out = MessageQueueHelper::DequeueUnsafe(mq);
            }

            /* Signal that we're not full. */
            util::GetReference(mq->ev_not_full).SignalWithManualClear();

            return true;
        }

        template<bool ClearEvent>
        bool TryPeekLightMessageQueueImpl(uintptr_t *out, const LightMessageQueueType *mq) {
            /* Perform the peek. */
            {
                /* Acquire exclusive access to the queue. */
                std::scoped_lock lk(util::GetReference(mq->mutex_queue));

                /* Check that we can peek. */
                if (MessageQueueHelper::IsMessageQueueEmpty(mq)) {
                    /* If we should, clear the event. */
                    if constexpr (ClearEvent) {
                        util::GetReference(mq->ev_not_empty).Clear();
                    }

                    /* We can't enqueue because we're full. */
                    return false;
                }

                /* Peek the data. */
                *out = MessageQueueHelper::PeekUnsafe(mq);
            }

            return true;
        }

    }

    void InitializeLightMessageQueue(LightMessageQueueType *mq, uintptr_t *buffer, size_t count) {
        /* Check pre-conditions. */
        AMS_ASSERT(buffer != nullptr);
        AMS_ASSERT(count >= 1);

        /* Construct objects. */
        util::ConstructAt(mq->mutex_queue);
        util::ConstructAt(mq->ev_not_empty, false);
        util::ConstructAt(mq->ev_not_full, true);

        /* Set member variables. */
        mq->buffer   = buffer;
        mq->capacity = static_cast<s32>(count);
        mq->count    = 0;
        mq->offset   = 0;

        /* Mark initialized. */
        mq->state = LightMessageQueueType::State_Initialized;
    }

    void FinalizeLightMessageQueue(LightMessageQueueType *mq) {
        /* Check pre-conditions. */
        AMS_ASSERT(mq->state == LightMessageQueueType::State_Initialized);

        /* Mark uninitialized. */
        mq->state = LightMessageQueueType::State_NotInitialized;

        /* Destroy objects. */
        util::DestroyAt(mq->ev_not_empty);
        util::DestroyAt(mq->ev_not_full);
        util::DestroyAt(mq->mutex_queue);
    }

    /* Sending (FIFO functionality) */
    void SendLightMessageQueue(LightMessageQueueType *mq, uintptr_t data) {
        /* Check pre-conditions. */
        AMS_ASSERT(mq->state == LightMessageQueueType::State_Initialized);

        /* Repeatedly try to send. */
        while (!TrySendLightMessageQueueImpl<true>(mq, data)) {
            /* Wait until we can try to send again. */
            util::GetReference(mq->ev_not_full).WaitWithManualClear();
        }
    }

    bool TrySendLightMessageQueue(LightMessageQueueType *mq, uintptr_t data) {
        /* Check pre-conditions. */
        AMS_ASSERT(mq->state == LightMessageQueueType::State_Initialized);

        /* Try to send. */
        return TrySendLightMessageQueueImpl<false>(mq, data);
    }

    bool TimedSendLightMessageQueue(LightMessageQueueType *mq, uintptr_t data, TimeSpan timeout) {
        /* Check pre-conditions. */
        AMS_ASSERT(mq->state == LightMessageQueueType::State_Initialized);
        AMS_ASSERT(timeout.GetNanoSeconds() >= 0);

        /* Create timeout helper. */
        impl::TimeoutHelper timeout_helper(timeout);

        /* Repeatedly try to send. */
        while (!TrySendLightMessageQueueImpl<true>(mq, data)) {
            /* Check if we're timed out. */
            if (timeout_helper.TimedOut()) {
                return false;
            }

            /* Wait until we can try to send again. */
            util::GetReference(mq->ev_not_full).TimedWaitWithManualClear(timeout_helper);
        }

        return true;
    }

    /* Jamming (LIFO functionality) */
    void JamLightMessageQueue(LightMessageQueueType *mq, uintptr_t data) {
        /* Check pre-conditions. */
        AMS_ASSERT(mq->state == LightMessageQueueType::State_Initialized);

        /* Repeatedly try to jam. */
        while (!TryJamLightMessageQueueImpl<true>(mq, data)) {
            /* Wait until we can try to jam again. */
            util::GetReference(mq->ev_not_full).WaitWithManualClear();
        }
    }

    bool TryJamLightMessageQueue(LightMessageQueueType *mq, uintptr_t data) {
        /* Check pre-conditions. */
        AMS_ASSERT(mq->state == LightMessageQueueType::State_Initialized);

        /* Try to jam. */
        return TryJamLightMessageQueueImpl<false>(mq, data);
    }

    bool TimedJamLightMessageQueue(LightMessageQueueType *mq, uintptr_t data, TimeSpan timeout) {
        /* Check pre-conditions. */
        AMS_ASSERT(mq->state == LightMessageQueueType::State_Initialized);
        AMS_ASSERT(timeout.GetNanoSeconds() >= 0);

        /* Create timeout helper. */
        impl::TimeoutHelper timeout_helper(timeout);

        /* Repeatedly try to jam. */
        while (!TryJamLightMessageQueueImpl<true>(mq, data)) {
            /* Check if we're timed out. */
            if (timeout_helper.TimedOut()) {
                return false;
            }

            /* Wait until we can try to jam again. */
            util::GetReference(mq->ev_not_full).TimedWaitWithManualClear(timeout_helper);
        }

        return true;
    }

    /* Receive functionality */
    void ReceiveLightMessageQueue(uintptr_t *out, LightMessageQueueType *mq) {
        /* Check pre-conditions. */
        AMS_ASSERT(mq->state == LightMessageQueueType::State_Initialized);

        /* Repeatedly try to receive. */
        while (!TryReceiveLightMessageQueueImpl<true>(out, mq)) {
            /* Wait until we can try to receive again. */
            util::GetReference(mq->ev_not_empty).WaitWithManualClear();
        }
    }

    bool TryReceiveLightMessageQueue(uintptr_t *out, LightMessageQueueType *mq) {
        /* Check pre-conditions. */
        AMS_ASSERT(mq->state == LightMessageQueueType::State_Initialized);

        /* Try to receive. */
        return TryReceiveLightMessageQueueImpl<false>(out, mq);
    }

    bool TimedReceiveLightMessageQueue(uintptr_t *out, LightMessageQueueType *mq, TimeSpan timeout) {
        /* Check pre-conditions. */
        AMS_ASSERT(mq->state == LightMessageQueueType::State_Initialized);
        AMS_ASSERT(timeout.GetNanoSeconds() >= 0);

        /* Create timeout helper. */
        impl::TimeoutHelper timeout_helper(timeout);

        /* Repeatedly try to receive. */
        while (!TryReceiveLightMessageQueueImpl<true>(out, mq)) {
            /* Check if we're timed out. */
            if (timeout_helper.TimedOut()) {
                return false;
            }

            /* Wait until we can try to receive again. */
            util::GetReference(mq->ev_not_empty).TimedWaitWithManualClear(timeout_helper);
        }

        return true;
    }

    /* Peek functionality */
    void PeekLightMessageQueue(uintptr_t *out, const LightMessageQueueType *mq) {
        /* Check pre-conditions. */
        AMS_ASSERT(mq->state == LightMessageQueueType::State_Initialized);

        /* Repeatedly try to peek. */
        while (!TryPeekLightMessageQueueImpl<true>(out, mq)) {
            /* Wait until we can try to peek again. */
            util::GetReference(mq->ev_not_empty).WaitWithManualClear();
        }
    }

    bool TryPeekLightMessageQueue(uintptr_t *out, const LightMessageQueueType *mq) {
        /* Check pre-conditions. */
        AMS_ASSERT(mq->state == LightMessageQueueType::State_Initialized);

        /* Try to peek. */
        return TryPeekLightMessageQueueImpl<false>(out, mq);
    }

    bool TimedPeekLightMessageQueue(uintptr_t *out, const LightMessageQueueType *mq, TimeSpan timeout) {
        /* Check pre-conditions. */
        AMS_ASSERT(mq->state == LightMessageQueueType::State_Initialized);
        AMS_ASSERT(timeout.GetNanoSeconds() >= 0);

        /* Create timeout helper. */
        impl::TimeoutHelper timeout_helper(timeout);

        /* Repeatedly try to peek. */
        while (!TryPeekLightMessageQueueImpl<true>(out, mq)) {
            /* Check if we're timed out. */
            if (timeout_helper.TimedOut()) {
                return false;
            }

            /* Wait until we can try to peek again. */
            util::GetReference(mq->ev_not_empty).TimedWaitWithManualClear(timeout_helper);
        }

        return true;
    }

}
