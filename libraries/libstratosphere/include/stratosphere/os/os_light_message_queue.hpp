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
#include <stratosphere/os/os_light_message_queue_types.hpp>
#include <stratosphere/os/os_light_message_queue_api.hpp>

namespace ams::os {

    class LightMessageQueue {
        NON_COPYABLE(LightMessageQueue);
        NON_MOVEABLE(LightMessageQueue);
        private:
            LightMessageQueueType m_mq;
        public:
            explicit LightMessageQueue(uintptr_t *buf, size_t count) {
                InitializeLightMessageQueue(std::addressof(m_mq), buf, count);
            }

            ~LightMessageQueue() { FinalizeLightMessageQueue(std::addressof(m_mq)); }

            /* Sending (FIFO functionality) */
            void Send(uintptr_t data) {
                return SendLightMessageQueue(std::addressof(m_mq), data);
            }

            bool TrySend(uintptr_t data) {
                return TrySendLightMessageQueue(std::addressof(m_mq), data);
            }

            bool TimedSend(uintptr_t data, TimeSpan timeout) {
                return TimedSendLightMessageQueue(std::addressof(m_mq), data, timeout);
            }

            /* Jamming (LIFO functionality) */
            void Jam(uintptr_t data) {
                return JamLightMessageQueue(std::addressof(m_mq), data);
            }

            bool TryJam(uintptr_t data) {
                return TryJamLightMessageQueue(std::addressof(m_mq), data);
            }

            bool TimedJam(uintptr_t data, TimeSpan timeout) {
                return TimedJamLightMessageQueue(std::addressof(m_mq), data, timeout);
            }

            /* Receive functionality */
            void Receive(uintptr_t *out) {
                return ReceiveLightMessageQueue(out, std::addressof(m_mq));
            }

            bool TryReceive(uintptr_t *out) {
                return TryReceiveLightMessageQueue(out, std::addressof(m_mq));
            }

            bool TimedReceive(uintptr_t *out, TimeSpan timeout) {
                return TimedReceiveLightMessageQueue(out, std::addressof(m_mq), timeout);
            }

            /* Peek functionality */
            void Peek(uintptr_t *out) const {
                return PeekLightMessageQueue(out, std::addressof(m_mq));
            }

            bool TryPeek(uintptr_t *out) const {
                return TryPeekLightMessageQueue(out, std::addressof(m_mq));
            }

            bool TimedPeek(uintptr_t *out, TimeSpan timeout) const {
                return TimedPeekLightMessageQueue(out, std::addressof(m_mq), timeout);
            }

            operator LightMessageQueueType &() {
                return m_mq;
            }

            operator const LightMessageQueueType &() const {
                return m_mq;
            }

            LightMessageQueueType *GetBase() {
                return std::addressof(m_mq);
            }
    };

}
