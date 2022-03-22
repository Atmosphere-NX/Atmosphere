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
            u64 m_offset;
            u64 m_total_send_size;
            u64 m_cur_max_data;
            u64 m_prev_max_data;
            util::optional<u64> m_share;
            os::Event m_state_change_event;
            ChannelState m_state;
        public:
            ChannelImpl(impl::ChannelInternalType channel, PacketFactory *pf, ctrl::HtcctrlStateMachine *sm, TaskManager *tm, os::Event *ev);

            void SetVersion(s16 version);

            ChannelState GetChannelState() { return m_state; }
            os::EventType *GetChannelStateEvent() { return m_state_change_event.GetBase(); }

            Result ProcessReceivePacket(const PacketHeader &header, const void *body, size_t body_size);

            bool QuerySendPacket(PacketHeader *header, PacketBody *body, int *out_body_size);

            void RemovePacket(const PacketHeader &header);

            void ShutdownForce();

            void UpdateState();
        public:
            Result DoConnectBegin(u32 *out_task_id);
            Result DoConnectEnd();

            Result DoFlush(u32 *out_task_id);

            Result DoReceiveBegin(u32 *out_task_id, size_t size);
            Result DoReceiveEnd(size_t *out, void *dst, size_t dst_size);

            Result DoSend(u32 *out_task_id, size_t *out, const void *src, size_t src_size);

            Result DoShutdown();

            void SetConfig(const ChannelConfig &config);

            void SetSendBuffer(void *buf, size_t buf_size, size_t max_packet_size);
            void SetReceiveBuffer(void *buf, size_t buf_size);
            void SetSendBufferWithData(const void *buf, size_t buf_size, size_t max_packet_size);
        private:
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
