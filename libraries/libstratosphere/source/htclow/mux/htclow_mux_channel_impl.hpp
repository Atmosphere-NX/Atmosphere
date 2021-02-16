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
#include "htclow_mux_send_buffer.hpp"

namespace ams::htclow {

    class PacketFactory;

    namespace ctrl {

        class HtcctrlStateMachine;

    }

}

namespace ams::htclow::mux {

    class ChannelImpl {
        private:
            impl::ChannelInternalType m_channel;
            PacketFactory *m_packet_factory;
            ctrl::HtcctrlStateMachine *m_state_machine;
            TaskManager *m_task_manager;
            os::Event *m_event;
            SendBuffer m_send_buffer;
            RingBuffer m_receive_buffer;
            s16 m_version;
            ChannelConfig m_config;
            u64 m_total_send_size;
            u64 m_next_max_data;
            u64 m_cur_max_data;
            u64 m_offset;
            std::optional<u64> m_share;
            os::Event m_state_change_event;
            ChannelState m_state;
        public:
            ChannelImpl(impl::ChannelInternalType channel, PacketFactory *pf, ctrl::HtcctrlStateMachine *sm, TaskManager *tm, os::Event *ev);

            void SetVersion(s16 version);

            Result ProcessReceivePacket(const PacketHeader &header, const void *body, size_t body_size);

            bool QuerySendPacket(PacketHeader *header, PacketBody *body, int *out_body_size);

            void RemovePacket(const PacketHeader &header);

            void UpdateState();
        private:
            void ShutdownForce();
            void SetState(ChannelState state);
            void SetStateWithoutCheck(ChannelState state);

            void SignalSendPacketEvent();

            Result CheckState(std::initializer_list<ChannelState> states) const;
            Result CheckPacketVersion(s16 version) const;

            Result ProcessReceiveDataPacket(s16 version, u64 share, u32 offset, const void *body, size_t body_size);
            Result ProcessReceiveMaxDataPacket(s16 version, u64 share);
            Result ProcessReceiveErrorPacket();
    };

}
