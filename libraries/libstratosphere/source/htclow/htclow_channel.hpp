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
#include <stratosphere.hpp>
#include "htclow_manager.hpp"

namespace ams::htclow {

    class Channel final {
        private:
            HtclowManager *m_manager;
            ChannelType m_channel;
        public:
            constexpr Channel(HtclowManager *manager) : m_manager(manager), m_channel() { /* ... */ }
            constexpr ~Channel() { m_channel = {}; }

            ChannelType *GetBase() { return std::addressof(m_channel); }
        public:
            Result Open(const Module *module, ChannelId id);
            void Close();

            ChannelState GetChannelState();
            os::EventType *GetChannelStateEvent();

            Result Connect();
            Result Flush();
            void Shutdown();

            Result Receive(s64 *out, void *dst, s64 size, ReceiveOption option);
            Result Send(s64 *out, const void *src, s64 size);

            void SetConfig(const ChannelConfig &config);
            void SetReceiveBuffer(void *buf, size_t size);
            void SetSendBuffer(void *buf, size_t size);
            void SetSendBufferWithData(const void *buf, size_t size);

            Result WaitReceive(s64 size);
            Result WaitReceive(s64 size, os::EventType *event);
        private:
            void WaitEvent(os::EventType *event, bool);
            Result ReceiveInternal(s64 *out, void *dst, s64 size, ReceiveOption option);
            Result WaitReceiveInternal(s64 size, os::EventType *event);
    };

}
