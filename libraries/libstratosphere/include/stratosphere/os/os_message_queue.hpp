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
#include <stratosphere/os/os_message_queue_common.hpp>
#include <stratosphere/os/os_message_queue_types.hpp>
#include <stratosphere/os/os_message_queue_api.hpp>

namespace ams::os {

    class MessageQueue {
        NON_COPYABLE(MessageQueue);
        NON_MOVEABLE(MessageQueue);
        private:
            MessageQueueType m_mq;
        public:
            explicit MessageQueue(uintptr_t *buf, size_t count) {
                InitializeMessageQueue(std::addressof(m_mq), buf, count);
            }

            ~MessageQueue() { FinalizeMessageQueue(std::addressof(m_mq)); }

            /* Sending (FIFO functionality) */
            void Send(uintptr_t data) {
                return SendMessageQueue(std::addressof(m_mq), data);
            }

            bool TrySend(uintptr_t data) {
                return TrySendMessageQueue(std::addressof(m_mq), data);
            }

            bool TimedSend(uintptr_t data, TimeSpan timeout) {
                return TimedSendMessageQueue(std::addressof(m_mq), data, timeout);
            }

            /* Jamming (LIFO functionality) */
            void Jam(uintptr_t data) {
                return JamMessageQueue(std::addressof(m_mq), data);
            }

            bool TryJam(uintptr_t data) {
                return TryJamMessageQueue(std::addressof(m_mq), data);
            }

            bool TimedJam(uintptr_t data, TimeSpan timeout) {
                return TimedJamMessageQueue(std::addressof(m_mq), data, timeout);
            }

            /* Receive functionality */
            void Receive(uintptr_t *out) {
                return ReceiveMessageQueue(out, std::addressof(m_mq));
            }

            bool TryReceive(uintptr_t *out) {
                return TryReceiveMessageQueue(out, std::addressof(m_mq));
            }

            bool TimedReceive(uintptr_t *out, TimeSpan timeout) {
                return TimedReceiveMessageQueue(out, std::addressof(m_mq), timeout);
            }

            /* Peek functionality */
            void Peek(uintptr_t *out) const {
                return PeekMessageQueue(out, std::addressof(m_mq));
            }

            bool TryPeek(uintptr_t *out) const {
                return TryPeekMessageQueue(out, std::addressof(m_mq));
            }

            bool TimedPeek(uintptr_t *out, TimeSpan timeout) const {
                return TimedPeekMessageQueue(out, std::addressof(m_mq), timeout);
            }

            operator MessageQueueType &() {
                return m_mq;
            }

            operator const MessageQueueType &() const {
                return m_mq;
            }

            MessageQueueType *GetBase() {
                return std::addressof(m_mq);
            }
    };

}
