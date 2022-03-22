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
#include <stratosphere/os/os_message_queue_common.hpp>

namespace ams::os {

    struct MessageQueueType;
    struct MultiWaitHolderType;

    void InitializeMessageQueue(MessageQueueType *mq, uintptr_t *buffer, size_t count);
    void FinalizeMessageQueue(MessageQueueType *mq);

    /* Sending (FIFO functionality) */
    void SendMessageQueue(MessageQueueType *mq, uintptr_t data);
    bool TrySendMessageQueue(MessageQueueType *mq, uintptr_t data);
    bool TimedSendMessageQueue(MessageQueueType *mq, uintptr_t data, TimeSpan timeout);

    /* Jamming (LIFO functionality) */
    void JamMessageQueue(MessageQueueType *mq, uintptr_t data);
    bool TryJamMessageQueue(MessageQueueType *mq, uintptr_t data);
    bool TimedJamMessageQueue(MessageQueueType *mq, uintptr_t data, TimeSpan timeout);

    /* Receive functionality */
    void ReceiveMessageQueue(uintptr_t *out, MessageQueueType *mq);
    bool TryReceiveMessageQueue(uintptr_t *out, MessageQueueType *mq);
    bool TimedReceiveMessageQueue(uintptr_t *out, MessageQueueType *mq, TimeSpan timeout);

    /* Peek functionality */
    void PeekMessageQueue(uintptr_t *out, const MessageQueueType *mq);
    bool TryPeekMessageQueue(uintptr_t *out, const MessageQueueType *mq);
    bool TimedPeekMessageQueue(uintptr_t *out, const MessageQueueType *mq, TimeSpan timeout);

    void InitializeMultiWaitHolder(MultiWaitHolderType *multi_wait_holder, MessageQueueType *event, MessageQueueWaitType wait_type);

}
