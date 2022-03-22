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

namespace ams::os {

    struct LightMessageQueueType;

    void InitializeLightMessageQueue(LightMessageQueueType *mq, uintptr_t *buffer, size_t count);
    void FinalizeLightMessageQueue(LightMessageQueueType *mq);

    /* Sending (FIFO functionality) */
    void SendLightMessageQueue(LightMessageQueueType *mq, uintptr_t data);
    bool TrySendLightMessageQueue(LightMessageQueueType *mq, uintptr_t data);
    bool TimedSendLightMessageQueue(LightMessageQueueType *mq, uintptr_t data, TimeSpan timeout);

    /* Jamming (LIFO functionality) */
    void JamLightMessageQueue(LightMessageQueueType *mq, uintptr_t data);
    bool TryJamLightMessageQueue(LightMessageQueueType *mq, uintptr_t data);
    bool TimedJamLightMessageQueue(LightMessageQueueType *mq, uintptr_t data, TimeSpan timeout);

    /* Receive functionality */
    void ReceiveLightMessageQueue(uintptr_t *out, LightMessageQueueType *mq);
    bool TryReceiveLightMessageQueue(uintptr_t *out, LightMessageQueueType *mq);
    bool TimedReceiveLightMessageQueue(uintptr_t *out, LightMessageQueueType *mq, TimeSpan timeout);

    /* Peek functionality */
    void PeekLightMessageQueue(uintptr_t *out, const LightMessageQueueType *mq);
    bool TryPeekLightMessageQueue(uintptr_t *out, const LightMessageQueueType *mq);
    bool TimedPeekLightMessageQueue(uintptr_t *out, const LightMessageQueueType *mq, TimeSpan timeout);

}
