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
#include <stratosphere.hpp>
#include "htclow_mux_task_manager.hpp"
#include "htclow_mux_channel_impl_map.hpp"
#include "htclow_mux_global_send_buffer.hpp"

namespace ams::htclow::mux {

    enum class MuxState {
        Normal,
        Sleep,
    };

    class Mux {
        private:
        private:
            PacketFactory *m_packet_factory;
            ctrl::HtcctrlStateMachine *m_state_machine;
            TaskManager m_task_manager;
            os::Event m_event;
            ChannelImplMap m_channel_impl_map;
            GlobalSendBuffer m_global_send_buffer;
            os::SdkMutex m_mutex;
            MuxState m_state;
            s16 m_version;
        public:
            Mux(PacketFactory *pf, ctrl::HtcctrlStateMachine *sm);

            void SetVersion(u16 version);

            os::EventType *GetSendPacketEvent() { return m_event.GetBase(); }

            Result CheckReceivedHeader(const PacketHeader &header) const;
            Result ProcessReceivePacket(const PacketHeader &header, const void *body, size_t body_size);

            bool QuerySendPacket(PacketHeader *header, PacketBody *body, int *out_body_size);
            void RemovePacket(const PacketHeader &header);

            void UpdateChannelState();
            void UpdateMuxState();
        private:
            Result CheckChannelExist(impl::ChannelInternalType channel);

            Result SendErrorPacket(impl::ChannelInternalType channel);

            bool IsSendable(PacketType packet_type) const;
    };

}
