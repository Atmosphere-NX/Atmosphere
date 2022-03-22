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
#include "htclow_ctrl_send_buffer.hpp"
#include "htclow_ctrl_packet_factory.hpp"

namespace ams::htclow::ctrl {

    bool HtcctrlSendBuffer::IsPriorPacket(HtcctrlPacketType packet_type) const {
        return packet_type == HtcctrlPacketType_DisconnectFromTarget;
    }

    bool HtcctrlSendBuffer::IsPosteriorPacket(HtcctrlPacketType packet_type) const {
        switch (packet_type) {
            case HtcctrlPacketType_ConnectFromTarget:
            case HtcctrlPacketType_ReadyFromTarget:
            case HtcctrlPacketType_SuspendFromTarget:
            case HtcctrlPacketType_ResumeFromTarget:
            case HtcctrlPacketType_BeaconResponse:
            case HtcctrlPacketType_InformationFromTarget:
                return true;
            default:
                return false;
        }
    }

    void HtcctrlSendBuffer::AddPacket(std::unique_ptr<HtcctrlPacket, HtcctrlPacketDeleter> ptr) {
        /* Get the packet. */
        HtcctrlPacket *packet = ptr.release();

        /* Get the packet type. */
        const auto packet_type = packet->GetHeader()->packet_type;

        /* Add the packet to the appropriate list. */
        if (this->IsPriorPacket(packet_type)) {
            m_prior_packet_list.push_back(*packet);
        } else {
            AMS_ABORT_UNLESS(this->IsPosteriorPacket(packet_type));
            m_posterior_packet_list.push_back(*packet);
        }
    }

    void HtcctrlSendBuffer::RemovePacket(const HtcctrlPacketHeader &header) {
        /* Get the packet type. */
        const auto packet_type = header.packet_type;

        /* Remove the front from the appropriate list. */
        HtcctrlPacket *packet;
        if (this->IsPriorPacket(packet_type)) {
            packet = std::addressof(m_prior_packet_list.front());
            m_prior_packet_list.pop_front();
        } else {
            AMS_ABORT_UNLESS(this->IsPosteriorPacket(packet_type));
            packet = std::addressof(m_posterior_packet_list.front());
            m_posterior_packet_list.pop_front();
        }

        /* Delete the packet. */
        m_packet_factory->Delete(packet);
    }

    bool HtcctrlSendBuffer::QueryNextPacket(HtcctrlPacketHeader *header, HtcctrlPacketBody *body, int *out_body_size) {
        if (!m_prior_packet_list.empty()) {
            this->CopyPacket(header, body, out_body_size, m_prior_packet_list.front());
            return true;
        } else if (!m_posterior_packet_list.empty()) {
            this->CopyPacket(header, body, out_body_size, m_posterior_packet_list.front());
            return true;
        } else {
            return false;
        }
    }

    void HtcctrlSendBuffer::CopyPacket(HtcctrlPacketHeader *header, HtcctrlPacketBody *body, int *out_body_size, const HtcctrlPacket &packet) {
        /* Get the body size. */
        const int body_size = packet.GetBodySize();
        AMS_ASSERT(0 <= body_size && body_size <= static_cast<int>(sizeof(*body)));

        /* Copy the header. */
        std::memcpy(header, packet.GetHeader(), sizeof(*header));

        /* Copy the body. */
        std::memcpy(body, packet.GetBody(), body_size);

        /* Set the output body size. */
        *out_body_size = body_size;
    }

}
