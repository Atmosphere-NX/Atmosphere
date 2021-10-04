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
#include "htclow_mux_global_send_buffer.hpp"
#include "../htclow_packet_factory.hpp"

namespace ams::htclow::mux {

    Packet *GlobalSendBuffer::GetNextPacket() {
        if (!m_packet_list.empty()) {
            return std::addressof(m_packet_list.front());
        } else {
            return nullptr;
        }
    }

    Result GlobalSendBuffer::AddPacket(std::unique_ptr<Packet, PacketDeleter> ptr) {
        /* Global send buffer only supports adding error packets. */
        R_UNLESS(ptr->GetHeader()->packet_type == PacketType_Error, htclow::ResultInvalidArgument());

        /* Check if we already have an error packet for the channel. */
        for (const auto &packet : m_packet_list) {
            R_SUCCEED_IF(packet.GetHeader()->channel == ptr->GetHeader()->channel);
        }

        /* We don't, so push back a new one. */
        m_packet_list.push_back(*(ptr.release()));

        return ResultSuccess();
    }

    void GlobalSendBuffer::RemovePacket() {
        auto *packet = std::addressof(m_packet_list.front());
        m_packet_list.pop_front();

        m_packet_factory->Delete(packet);
    }

}
