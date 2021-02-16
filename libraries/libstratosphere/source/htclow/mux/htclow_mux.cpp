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
#include "../ctrl/htclow_ctrl_state_machine.hpp"

namespace ams::htclow::mux {

    Mux::Mux(PacketFactory *pf, ctrl::HtcctrlStateMachine *sm)
        : m_packet_factory(pf), m_state_machine(sm), m_task_manager(), m_wake_event(os::EventClearMode_ManualClear),
          m_channel_impl_map(pf, sm, std::addressof(m_task_manager), std::addressof(m_wake_event)), m_global_send_buffer(pf),
          m_mutex(), m_is_sleeping(false), m_version(ProtocolVersion)
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
            m_is_sleeping = true;
        } else {
            m_is_sleeping = false;
            m_wake_event.Signal();
        }
    }

    Result Mux::CheckChannelExist(impl::ChannelInternalType channel) {
        R_UNLESS(m_channel_impl_map.Exists(channel), htclow::ResultChannelNotExist());
        return ResultSuccess();
    }

    Result Mux::SendErrorPacket(impl::ChannelInternalType channel) {
        /* TODO */
        AMS_ABORT("Mux::SendErrorPacket");
    }

}
