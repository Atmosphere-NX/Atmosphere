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
            m_channel_impl_map[pair.first].SetVersion(m_version);
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
        if (m_channel_impl_map.Exists(header.channel)) {
            R_TRY(this->CheckChannelExist(header.channel));

            return m_channel_impl_map[header.channel].ProcessReceivePacket(header, body, body_size);
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

        /* Get our map. */
        auto &map = m_channel_impl_map.GetMap();

        /* Iterate the map, checking for valid packet each time. */
        for (auto &pair : map) {
            /* Get the current channel impl. */
            auto &channel_impl = m_channel_impl_map.GetChannelImpl(pair.second);

            /* Check for an error packet. */
            /* NOTE: it's unclear why Nintendo does this every iteration of the loop... */
            if (auto *error_packet = m_global_send_buffer.GetNextPacket(); error_packet != nullptr) {
                std::memcpy(header, error_packet->GetHeader(), sizeof(*header));
                *out_body_size = 0;
                return true;
            }

            /* See if the channel has something for us to send. */
            if (channel_impl.QuerySendPacket(header, body, out_body_size)) {
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
        } else if (m_channel_impl_map.Exists(header.channel)) {
            m_channel_impl_map[header.channel].RemovePacket(header);
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
            m_channel_impl_map[pair.first].UpdateState();
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
        switch (m_state) {
            case MuxState::Normal:
                return true;
            case MuxState::Sleep:
                return false;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

}
