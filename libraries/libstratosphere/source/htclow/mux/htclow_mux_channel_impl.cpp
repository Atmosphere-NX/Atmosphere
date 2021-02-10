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
#include <stratosphere.hpp>
#include "htclow_mux_channel_impl.hpp"
#include "../ctrl/htclow_ctrl_state_machine.hpp"
#include "../htclow_default_channel_config.hpp"

namespace ams::htclow::mux {

    ChannelImpl::ChannelImpl(impl::ChannelInternalType channel, PacketFactory *pf, ctrl::HtcctrlStateMachine *sm, TaskManager *tm, os::Event *ev)
        : m_channel(channel), m_packet_factory(pf), m_state_machine(sm), m_task_manager(tm), m_event(ev),
          m_send_buffer(m_channel, pf), m_receive_buffer(), m_version(ProtocolVersion), m_config(DefaultChannelConfig),
          m_offset(0), m_total_send_size(0), m_next_max_data(0), m_cur_max_data(0), m_share(),
          m_state_change_event(os::EventClearMode_ManualClear), m_state(ChannelState_Unconnectable)

    {
        this->UpdateState();
    }

    void ChannelImpl::SetVersion(s16 version) {
        /* Sanity check the version. */
        AMS_ASSERT(version <= ProtocolVersion);

        /* Set version. */
        m_version = version;
        m_send_buffer.SetVersion(version);
    }

    Result ChannelImpl::CheckState(std::initializer_list<ChannelState> states) const {
        /* Determine if we have a matching state. */
        bool match = false;
        for (const auto &state : states) {
            match |= m_state == state;
        }

        /* If we do, we're good. */
        R_SUCCEED_IF(match);

        /* Otherwise, return appropriate failure error. */
        if (m_state == ChannelState_Disconnected) {
            return htclow::ResultInvalidChannelStateDisconnected();
        } else {
            return htclow::ResultInvalidChannelState();
        }
    }

    Result ChannelImpl::CheckPacketVersion(s16 version) const {
        R_UNLESS(version == m_version, htclow::ResultChannelVersionNotMatched());
        return ResultSuccess();
    }


    Result ChannelImpl::ProcessReceivePacket(const PacketHeader &header, const void *body, size_t body_size) {
        switch (header.packet_type) {
            case PacketType_Data:
                return this->ProcessReceiveDataPacket(header.version, header.share, header.offset, body, body_size);
            case PacketType_MaxData:
                return this->ProcessReceiveMaxDataPacket(header.version, header.share);
            case PacketType_Error:
                return this->ProcessReceiveErrorPacket();
            default:
                return htclow::ResultProtocolError();
        }
    }

    Result ChannelImpl::ProcessReceiveDataPacket(s16 version, u64 share, u32 offset, const void *body, size_t body_size) {
        /* Check our state. */
        R_TRY(this->CheckState({ChannelState_Connectable, ChannelState_Connected}));

        /* Check the packet version. */
        R_TRY(this->CheckPacketVersion(version));

        /* Check that offset matches. */
        R_UNLESS(offset == static_cast<u32>(m_offset), htclow::ResultProtocolError());

        /* Check for flow control, if we should. */
        if (m_config.flow_control_enabled) {
            /* Check that the share increases monotonically. */
            if (m_share.has_value()) {
                R_UNLESS(m_share.value() <= share, htclow::ResultProtocolError());
            }

            /* Update our share. */
            m_share = share;

            /* Signal our event. */
            this->SignalSendPacketEvent();
        }

        /* Update our offset. */
        m_offset += body_size;

        /* Write the packet body. */
        R_ABORT_UNLESS(m_receive_buffer.Write(body, body_size));

        /* Notify the data was received. */
        m_task_manager->NotifyReceiveData(m_channel, m_receive_buffer.GetDataSize());

        return ResultSuccess();
    }

    Result ChannelImpl::ProcessReceiveMaxDataPacket(s16 version, u64 share) {
        /* Check our state. */
        R_TRY(this->CheckState({ChannelState_Connectable, ChannelState_Connected}));

        /* Check the packet version. */
        R_TRY(this->CheckPacketVersion(version));

        /* Check for flow control, if we should. */
        if (m_config.flow_control_enabled) {
            /* Check that the share increases monotonically. */
            if (m_share.has_value()) {
                R_UNLESS(m_share.value() <= share, htclow::ResultProtocolError());
            }

            /* Update our share. */
            m_share = share;

            /* Signal our event. */
            this->SignalSendPacketEvent();
        }

        return ResultSuccess();
    }

    Result ChannelImpl::ProcessReceiveErrorPacket() {
        if (m_state == ChannelState_Connected || m_state == ChannelState_Disconnected) {
            this->ShutdownForce();
        }
        return ResultSuccess();
    }

    bool ChannelImpl::QuerySendPacket(PacketHeader *header, PacketBody *body, int *out_body_size) {
        /* Check our send buffer. */
        if (m_send_buffer.QueryNextPacket(header, body, out_body_size, m_next_max_data, m_total_send_size, m_share.has_value(), m_share.value_or(0))) {
            /* Update tracking variables. */
            if (header->packet_type == PacketType_Data) {
                m_cur_max_data = m_next_max_data;
            }

            return true;
        } else {
            return false;
        }
    }

    void ChannelImpl::RemovePacket(const PacketHeader &header) {
        /* Remove the packet. */
        m_send_buffer.RemovePacket(header);

        /* Check if the send buffer is now empty. */
        if (m_send_buffer.Empty()) {
            m_task_manager->NotifySendBufferEmpty(m_channel);
        }
    }

    void ChannelImpl::UpdateState() {
        /* Check if shutdown must be forced. */
        if (m_state_machine->IsUnsupportedServiceChannelToShutdown(m_channel)) {
            this->ShutdownForce();
        }

        /* Check if we're readied. */
        if (m_state_machine->IsReadied()) {
            m_task_manager->NotifyConnectReady();
        }

        /* Update our state transition. */
        if (m_state_machine->IsConnectable(m_channel)) {
            if (m_state == ChannelState_Unconnectable) {
                this->SetState(ChannelState_Connectable);
            }
        } else if (m_state_machine->IsUnconnectable()) {
            if (m_state == ChannelState_Connectable) {
                this->SetState(ChannelState_Unconnectable);
                m_state_machine->SetNotConnecting(m_channel);
            } else if (m_state == ChannelState_Connected) {
                this->ShutdownForce();
            }
        }
    }

    void ChannelImpl::ShutdownForce() {
        /* Clear our send buffer. */
        m_send_buffer.Clear();

        /* Set our state to shutdown. */
        this->SetState(ChannelState_Disconnected);
    }

    void ChannelImpl::SetState(ChannelState state) {
        /* Check that we can perform the transition. */
        AMS_ABORT_UNLESS(IsStateTransitionAllowed(m_state, state));

        /* Perform the transition. */
        this->SetStateWithoutCheck(state);
    }

    void ChannelImpl::SetStateWithoutCheck(ChannelState state) {
        /* Change our state. */
        if (m_state != state) {
            m_state = state;
            m_state_change_event.Signal();
        }

        /* If relevant, notify disconnect. */
        if (m_state == ChannelState_Disconnected) {
            m_task_manager->NotifyDisconnect(m_channel);
        }
    }

    void ChannelImpl::SignalSendPacketEvent() {
        if (m_event != nullptr) {
            m_event->Signal();
        }
    }

    void ChannelImpl::SetSendBuffer(void *buf, size_t buf_size, size_t max_packet_size) {
        /* Set buffer. */
        m_send_buffer.SetBuffer(buf, buf_size);

        /* Determine true max packet size. */
        if (m_config.flow_control_enabled) {
            max_packet_size = std::min(max_packet_size, m_config.max_packet_size);
        }

        /* Set max packet size. */
        m_send_buffer.SetMaxPacketSize(max_packet_size);
    }

    void ChannelImpl::SetReceiveBuffer(void *buf, size_t buf_size) {
        /* Set the buffer. */
        m_receive_buffer.Initialize(buf, buf_size);
    }

    void ChannelImpl::SetSendBufferWithData(const void *buf, size_t buf_size, size_t max_packet_size) {
        /* Set buffer. */
        m_send_buffer.SetReadOnlyBuffer(buf, buf_size);

        /* Determine true max packet size. */
        if (m_config.flow_control_enabled) {
            max_packet_size = std::min(max_packet_size, m_config.max_packet_size);
        }

        /* Set max packet size. */
        m_send_buffer.SetMaxPacketSize(max_packet_size);
    }

}
