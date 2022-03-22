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
#include <stratosphere.hpp>
#include "htclow_mux.hpp"
#include "../htclow_packet_factory.hpp"
#include "../ctrl/htclow_ctrl_state_machine.hpp"

namespace ams::htclow::mux {

    Mux::Mux(PacketFactory *pf, ctrl::HtcctrlStateMachine *sm)
        : m_packet_factory(pf), m_state_machine(sm), m_task_manager(), m_event(os::EventClearMode_ManualClear),
          m_channel_impl_map(pf, sm, std::addressof(m_task_manager), std::addressof(m_event)), m_global_send_buffer(pf),
          m_mutex(), m_state(MuxState::Normal), m_version(ProtocolVersion)
    {
        /* ... */
    }

    void Mux::SetVersion(u16 version) {
        /* Set our version. */
        m_version = version;

        /* Set all entries in our map. */
        /* NOTE: Nintendo does this highly inefficiently... */
        for (auto &pair : m_channel_impl_map.GetMap()) {
            m_channel_impl_map[pair.second].SetVersion(m_version);
        }
    }

    Result Mux::CheckReceivedHeader(const PacketHeader &header) const {
        /* Check the packet signature. */
        AMS_ASSERT(header.signature == HtcGen2Signature);

        /* Switch on the packet type. */
        switch (header.packet_type) {
            case PacketType_Data:
                R_UNLESS(header.version == m_version,            htclow::ResultProtocolError());
                R_UNLESS(header.body_size <= sizeof(PacketBody), htclow::ResultProtocolError());
                break;
            case PacketType_MaxData:
                R_UNLESS(header.version == m_version,            htclow::ResultProtocolError());
                R_UNLESS(header.body_size == 0,                  htclow::ResultProtocolError());
                break;
            case PacketType_Error:
                R_UNLESS(header.body_size == 0,                  htclow::ResultProtocolError());
                break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }

        return ResultSuccess();
    }

    Result Mux::ProcessReceivePacket(const PacketHeader &header, const void *body, size_t body_size) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Process for the channel. */
        if (auto it = m_channel_impl_map.GetMap().find(header.channel); it != m_channel_impl_map.GetMap().end()) {
            return m_channel_impl_map[it->second].ProcessReceivePacket(header, body, body_size);
        } else {
            if (header.packet_type == PacketType_Data || header.packet_type == PacketType_MaxData) {
                this->SendErrorPacket(header.channel);
            }
            return htclow::ResultChannelNotExist();
        }
    }

    bool Mux::QuerySendPacket(PacketHeader *header, PacketBody *body, int *out_body_size) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Check for an error packet. */
        /* NOTE: Nintendo checks this once per iteration of the below loop. */
        /* The extra checks are unnecessary, because we hold our mutex. */
        if (auto *error_packet = m_global_send_buffer.GetNextPacket(); error_packet != nullptr) {
            std::memcpy(header, error_packet->GetHeader(), sizeof(*header));
            *out_body_size = 0;
            return true;
        }

        /* Iterate the map, checking each channel for a valid valid packet. */
        for (auto &pair : m_channel_impl_map.GetMap()) {
            /* Get the current channel impl. */
            /* See if the channel has something for us to send. */
            if (m_channel_impl_map[pair.second].QuerySendPacket(header, body, out_body_size)) {
                return this->IsSendable(header->packet_type);
            }
        }

        return false;
    }

    void Mux::RemovePacket(const PacketHeader &header) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Remove the packet from the appropriate source. */
        if (header.packet_type == PacketType_Error) {
            m_global_send_buffer.RemovePacket();
        } else if (auto it = m_channel_impl_map.GetMap().find(header.channel); it != m_channel_impl_map.GetMap().end()) {
            m_channel_impl_map[it->second].RemovePacket(header);
        }

        /* Notify the task manager. */
        m_task_manager.NotifySendReady();
    }

    void Mux::UpdateChannelState() {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Update the state of all channels in our map. */
        /* NOTE: Nintendo does this highly inefficiently... */
        for (auto pair : m_channel_impl_map.GetMap()) {
            m_channel_impl_map[pair.second].UpdateState();
        }
    }

    void Mux::UpdateMuxState() {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Update whether we're sleeping. */
        if (m_state_machine->IsSleeping()) {
            m_state = MuxState::Sleep;
        } else {
            m_state = MuxState::Normal;
            m_event.Signal();
        }
    }

    Result Mux::CheckChannelExist(impl::ChannelInternalType channel) {
        R_UNLESS(m_channel_impl_map.Exists(channel), htclow::ResultChannelNotExist());
        return ResultSuccess();
    }

    Result Mux::SendErrorPacket(impl::ChannelInternalType channel) {
        /* Create and send the packet. */
        R_TRY(m_global_send_buffer.AddPacket(m_packet_factory->MakeErrorPacket(channel)));

        /* Signal our event. */
        m_event.Signal();

        return ResultSuccess();
    }

    bool Mux::IsSendable(PacketType packet_type) const {
        AMS_UNUSED(packet_type);

        switch (m_state) {
            case MuxState::Normal:
                return true;
            case MuxState::Sleep:
                return false;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    Result Mux::Open(impl::ChannelInternalType channel) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Check that the channel doesn't already exist. */
        R_UNLESS(!m_channel_impl_map.Exists(channel), htclow::ResultChannelAlreadyExist());

        /* Add the channel. */
        R_TRY(m_channel_impl_map.AddChannel(channel));

        /* Set the channel version. */
        m_channel_impl_map.GetChannelImpl(channel).SetVersion(m_version);

        return ResultSuccess();
    }

    Result Mux::Close(impl::ChannelInternalType channel) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* If we have the channel, close it. */
        if (auto it = m_channel_impl_map.GetMap().find(channel); it != m_channel_impl_map.GetMap().end()) {
            /* Shut down the channel. */
            m_channel_impl_map[it->second].ShutdownForce();

            /* Remove the channel. */
            R_ABORT_UNLESS(m_channel_impl_map.RemoveChannel(channel));
        }

        return ResultSuccess();
    }

    Result Mux::ConnectBegin(u32 *out_task_id, impl::ChannelInternalType channel) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Find the channel. */
        auto it = m_channel_impl_map.GetMap().find(channel);
        R_UNLESS(it != m_channel_impl_map.GetMap().end(), htclow::ResultChannelNotExist());

        /* Perform the connection. */
        return m_channel_impl_map[it->second].DoConnectBegin(out_task_id);
    }

    Result Mux::ConnectEnd(impl::ChannelInternalType channel, u32 task_id) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Get the trigger for the task. */
        const auto trigger = m_task_manager.GetTrigger(task_id);

        /* Free the task. */
        m_task_manager.FreeTask(task_id);

        /* Check that we didn't hit a disconnect. */
        R_UNLESS(trigger != EventTrigger_Disconnect, htclow::ResultInvalidChannelStateDisconnected());

        /* Find the channel. */
        auto it = m_channel_impl_map.GetMap().find(channel);
        R_UNLESS(it != m_channel_impl_map.GetMap().end(), htclow::ResultChannelNotExist());

        /* Perform the disconnection. */
        return m_channel_impl_map[it->second].DoConnectEnd();
    }

    ChannelState Mux::GetChannelState(impl::ChannelInternalType channel) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        return m_channel_impl_map.GetChannelImpl(channel).GetChannelState();
    }

    os::EventType *Mux::GetChannelStateEvent(impl::ChannelInternalType channel) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        return m_channel_impl_map.GetChannelImpl(channel).GetChannelStateEvent();
    }

    Result Mux::FlushBegin(u32 *out_task_id, impl::ChannelInternalType channel) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Find the channel. */
        auto it = m_channel_impl_map.GetMap().find(channel);
        R_UNLESS(it != m_channel_impl_map.GetMap().end(), htclow::ResultChannelNotExist());

        /* Perform the connection. */
        return m_channel_impl_map[it->second].DoFlush(out_task_id);
    }

    Result Mux::FlushEnd(u32 task_id) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Get the trigger for the task. */
        const auto trigger = m_task_manager.GetTrigger(task_id);

        /* Free the task. */
        m_task_manager.FreeTask(task_id);

        /* Check that we didn't hit a disconnect. */
        R_UNLESS(trigger != EventTrigger_Disconnect, htclow::ResultInvalidChannelStateDisconnected());

        return ResultSuccess();
    }

    os::EventType *Mux::GetTaskEvent(u32 task_id) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        return m_task_manager.GetTaskEvent(task_id);
    }

    Result Mux::ReceiveBegin(u32 *out_task_id, impl::ChannelInternalType channel, size_t size) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Find the channel. */
        auto it = m_channel_impl_map.GetMap().find(channel);
        R_UNLESS(it != m_channel_impl_map.GetMap().end(), htclow::ResultChannelNotExist());

        /* Perform the connection. */
        return m_channel_impl_map[it->second].DoReceiveBegin(out_task_id, size);
    }

    Result Mux::ReceiveEnd(size_t *out, void *dst, size_t dst_size, impl::ChannelInternalType channel, u32 task_id) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Free the task. */
        m_task_manager.FreeTask(task_id);

        /* If we have data, perform the receive. */
        if (dst_size > 0) {
            /* Find the channel. */
            auto it = m_channel_impl_map.GetMap().find(channel);
            R_UNLESS(it != m_channel_impl_map.GetMap().end(), htclow::ResultChannelNotExist());

            /* Perform the receive. */
            return m_channel_impl_map[it->second].DoReceiveEnd(out, dst, dst_size);
        } else {
            *out = 0;
            return ResultSuccess();
        }
    }

    Result Mux::SendBegin(u32 *out_task_id, size_t *out, const void *src, size_t src_size, impl::ChannelInternalType channel) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Find the channel. */
        auto it = m_channel_impl_map.GetMap().find(channel);
        R_UNLESS(it != m_channel_impl_map.GetMap().end(), htclow::ResultChannelNotExist());

        /* Perform the connection. */
        return m_channel_impl_map[it->second].DoSend(out_task_id, out, src, src_size);
    }

    Result Mux::SendEnd(u32 task_id) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Get the trigger for the task. */
        const auto trigger = m_task_manager.GetTrigger(task_id);

        /* Free the task. */
        m_task_manager.FreeTask(task_id);

        /* Check that we didn't hit a disconnect. */
        R_UNLESS(trigger != EventTrigger_Disconnect, htclow::ResultInvalidChannelStateDisconnected());

        return ResultSuccess();
    }

    Result Mux::WaitReceiveBegin(u32 *out_task_id, impl::ChannelInternalType channel, size_t size) {
        return this->ReceiveBegin(out_task_id, channel, size);
    }

    Result Mux::WaitReceiveEnd(u32 task_id) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Get the trigger for the task. */
        const auto trigger = m_task_manager.GetTrigger(task_id);

        /* Free the task. */
        m_task_manager.FreeTask(task_id);

        /* Check that we didn't hit a disconnect. */
        R_UNLESS(trigger != EventTrigger_Disconnect, htclow::ResultInvalidChannelStateDisconnected());

        return ResultSuccess();
    }

    void Mux::SetConfig(impl::ChannelInternalType channel, const ChannelConfig &config) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Find the channel. */
        auto it = m_channel_impl_map.GetMap().find(channel);
        AMS_ABORT_UNLESS(it != m_channel_impl_map.GetMap().end());

        /* Perform the connection. */
        return m_channel_impl_map[it->second].SetConfig(config);
    }

    void Mux::SetSendBuffer(impl::ChannelInternalType channel, void *buf, size_t buf_size, size_t max_packet_size) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Find the channel. */
        auto it = m_channel_impl_map.GetMap().find(channel);
        AMS_ABORT_UNLESS(it != m_channel_impl_map.GetMap().end());

        /* Set the send buffer. */
        m_channel_impl_map[it->second].SetSendBuffer(buf, buf_size, max_packet_size);
    }

    void Mux::SetSendBufferWithData(impl::ChannelInternalType channel, const void *buf, size_t buf_size, size_t max_packet_size) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Find the channel. */
        auto it = m_channel_impl_map.GetMap().find(channel);
        AMS_ABORT_UNLESS(it != m_channel_impl_map.GetMap().end());

        /* Set the send buffer. */
        m_channel_impl_map[it->second].SetSendBufferWithData(buf, buf_size, max_packet_size);
    }

    void Mux::SetReceiveBuffer(impl::ChannelInternalType channel, void *buf, size_t buf_size) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Find the channel. */
        auto it = m_channel_impl_map.GetMap().find(channel);
        AMS_ABORT_UNLESS(it != m_channel_impl_map.GetMap().end());

        /* Set the send buffer. */
        m_channel_impl_map[it->second].SetReceiveBuffer(buf, buf_size);
    }

    Result Mux::Shutdown(impl::ChannelInternalType channel) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Find the channel. */
        auto it = m_channel_impl_map.GetMap().find(channel);
        R_UNLESS(it != m_channel_impl_map.GetMap().end(), htclow::ResultChannelNotExist());

        /* Perform the shutdown. */
        return m_channel_impl_map[it->second].DoShutdown();
    }

}
