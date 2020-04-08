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
#include <stratosphere/os/os_message_queue_common.hpp>
#include <stratosphere/os/os_message_queue_types.hpp>
#include <stratosphere/os/os_message_queue_api.hpp>

namespace ams::os {

    class MessageQueue {
        NON_COPYABLE(MessageQueue);
        NON_MOVEABLE(MessageQueue);
        private:
            MessageQueueType mq;
        public:
            explicit MessageQueue(uintptr_t *buf, size_t count) {
                InitializeMessageQueue(std::addressof(this->mq), buf, count);
            }

            ~MessageQueue() { FinalizeMessageQueue(std::addressof(this->mq)); }

            /* Sending (FIFO functionality) */
            void Send(uintptr_t data) {
                return SendMessageQueue(std::addressof(this->mq), data);
            }

            bool TrySend(uintptr_t data) {
                return TrySendMessageQueue(std::addressof(this->mq), data);
            }

            bool TimedSend(uintptr_t data, TimeSpan timeout) {
                return TimedSendMessageQueue(std::addressof(this->mq), data, timeout);
            }

            /* Sending (LIFO functionality) */
            void SendNext(uintptr_t data) {
                return SendNextMessageQueue(std::addressof(this->mq), data);
            }

            bool TrySendNext(uintptr_t data) {
                return TrySendNextMessageQueue(std::addressof(this->mq), data);
            }

            bool TimedSendNext(uintptr_t data, TimeSpan timeout) {
                return TimedSendNextMessageQueue(std::addressof(this->mq), data, timeout);
            }

            /* Receive functionality */
            void Receive(uintptr_t *out) {
                return ReceiveMessageQueue(out, std::addressof(this->mq));
            }

            bool TryReceive(uintptr_t *out) {
                return TryReceiveMessageQueue(out, std::addressof(this->mq));
            }

            bool TimedReceive(uintptr_t *out, TimeSpan timeout) {
                return TimedReceiveMessageQueue(out, std::addressof(this->mq), timeout);
            }

            /* Peek functionality */
            void Peek(uintptr_t *out) const {
                return PeekMessageQueue(out, std::addressof(this->mq));
            }

            bool TryPeek(uintptr_t *out) const {
                return TryPeekMessageQueue(out, std::addressof(this->mq));
            }

            bool TimedPeek(uintptr_t *out, TimeSpan timeout) const {
                return TimedPeekMessageQueue(out, std::addressof(this->mq), timeout);
            }

            operator MessageQueueType &() {
                return this->mq;
            }

            operator const MessageQueueType &() const {
                return this->mq;
            }

            MessageQueueType *GetBase() {
                return std::addressof(this->mq);
            }
    };

}
