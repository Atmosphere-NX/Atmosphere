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

namespace ams::htc::server::driver {

    class IDriver {
        public:
            virtual void SetDisconnectionEmulationEnabled(bool en)                                                                                       = 0;
            virtual Result Open(htclow::ChannelId channel)                                                                                               = 0;
            virtual Result Open(htclow::ChannelId channel, void *receive_buffer, size_t receive_buffer_size, void *send_buffer, size_t send_buffer_size) = 0;
            virtual void Close(htclow::ChannelId channel)                                                                                                = 0;
            virtual Result Connect(htclow::ChannelId channel)                                                                                            = 0;
            virtual void Shutdown(htclow::ChannelId channel)                                                                                             = 0;
            virtual Result Send(s64 *out, const void *src, s64 src_size, htclow::ChannelId channel)                                                      = 0;
            virtual Result Receive(s64 *out, void *dst, s64 dst_size, htclow::ChannelId channel, htclow::ReceiveOption option)                           = 0;
            virtual htclow::ChannelState GetChannelState(htclow::ChannelId channel)                                                                      = 0;
            virtual os::EventType *GetChannelStateEvent(htclow::ChannelId channel)                                                                       = 0;
    };

}
