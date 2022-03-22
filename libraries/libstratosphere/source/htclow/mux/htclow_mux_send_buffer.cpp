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
#include "htclow_mux_channel_impl.hpp"
#include "../htclow_packet_factory.hpp"
#include "../htclow_default_channel_config.hpp"

namespace ams::htclow::mux {

    SendBuffer::SendBuffer(impl::ChannelInternalType channel, PacketFactory *pf)
        : m_channel(channel), m_packet_factory(pf), m_ring_buffer(), m_packet_list(),
          m_version(ProtocolVersion), m_flow_control_enabled(true), m_max_packet_size(DefaultChannelConfig.max_packet_size)
    {
        /* ... */
    }

    SendBuffer::~SendBuffer() {
        m_ring_buffer.Clear();
        this->Clear();
    }

    bool SendBuffer::IsPriorPacket(PacketType packet_type) const {
        return packet_type == PacketType_MaxData;
    }

    void SendBuffer::SetVersion(s16 version) {
        /* Set version. */
        m_version = version;
    }

    void SendBuffer::SetFlowControlEnabled(bool en) {
        /* Set flow control enabled. */
        m_flow_control_enabled = en;
    }

    void SendBuffer::MakeDataPacketHeader(PacketHeader *header, int body_size, s16 version, u64 share, u32 offset) const {
        /* Set all packet fields. */
        header->signature   = HtcGen2Signature;
        header->offset      = offset;
        header->reserved    = 0;
        header->version     = version;
        header->body_size   = body_size;
        header->channel     = m_channel;
        header->packet_type = PacketType_Data;
        header->share       = share;
    }

    void SendBuffer::CopyPacket(PacketHeader *header, PacketBody *body, int *out_body_size, const Packet &packet) {
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

    bool SendBuffer::QueryNextPacket(PacketHeader *header, PacketBody *body, int *out_body_size, u64 max_data, u64 total_send_size, bool has_share, u64 share) {
        /* Check for a max data packet. */
        if (!m_packet_list.empty()) {
            this->CopyPacket(header, body, out_body_size, m_packet_list.front());
            return true;
        }

        /* Check that we have data. */
        const auto ring_buffer_data_size = m_ring_buffer.GetDataSize();
        if (ring_buffer_data_size == 0) {
            return false;
        }

        /* Check that we're valid for flow control. */
        if (m_flow_control_enabled && !has_share) {
            return false;
        }

        /* Determine the sendable size. */
        const auto offset = total_send_size - ring_buffer_data_size;
        const auto sendable_size = m_flow_control_enabled ? std::min(share - offset, ring_buffer_data_size) : ring_buffer_data_size;
        if (sendable_size == 0) {
            return false;
        }

        /* We're additionally bound by the actual packet size. */
        const auto data_size = std::min(sendable_size, m_max_packet_size);

        /* Make data packet header. */
        this->MakeDataPacketHeader(header, data_size, m_version, max_data, offset);

        /* Copy the data. */
        R_ABORT_UNLESS(m_ring_buffer.Copy(body, data_size));

        /* Set output body size. */
        *out_body_size = data_size;
        return true;
    }

    void SendBuffer::AddPacket(std::unique_ptr<Packet, PacketDeleter> ptr) {
        /* Get the packet. */
        auto *packet = ptr.release();

        /* Check the packet type. */
        AMS_ABORT_UNLESS(this->IsPriorPacket(packet->GetHeader()->packet_type));

        /* Add the packet. */
        m_packet_list.push_back(*packet);
    }

    void SendBuffer::RemovePacket(const PacketHeader &header) {
        /* Get the packet type. */
        const auto packet_type = header.packet_type;

        if (this->IsPriorPacket(packet_type)) {
            /* Packet will be using our list. */
            auto *packet = std::addressof(m_packet_list.front());
            m_packet_list.pop_front();
            m_packet_factory->Delete(packet);
        } else {
            /* Packet managed by ring buffer. */
            AMS_ABORT_UNLESS(packet_type == PacketType_Data);

            /* Discard the packet's data. */
            const Result result = m_ring_buffer.Discard(header.body_size);
            if (!htclow::ResultChannelCannotDiscard::Includes(result)) {
                R_ABORT_UNLESS(result);
            }
        }
    }

    size_t SendBuffer::AddData(const void *data, size_t size) {
        /* Determine how much to actually add. */
        size = std::min(size, m_ring_buffer.GetBufferSize() - m_ring_buffer.GetDataSize());

        /* Write the data. */
        R_ABORT_UNLESS(m_ring_buffer.Write(data, size));

        /* Return the size we wrote. */
        return size;
    }

    void SendBuffer::SetBuffer(void *buffer, size_t buffer_size) {
        m_ring_buffer.Initialize(buffer, buffer_size);
    }

    void SendBuffer::SetReadOnlyBuffer(const void *buffer, size_t buffer_size) {
        m_ring_buffer.InitializeForReadOnly(buffer, buffer_size);
    }

    void SendBuffer::SetMaxPacketSize(size_t max_packet_size) {
        m_max_packet_size = max_packet_size;
    }

    bool SendBuffer::Empty() {
        return m_packet_list.empty() && m_ring_buffer.GetDataSize() == 0;
    }

    void SendBuffer::Clear() {
        while (!m_packet_list.empty()) {
            auto *packet = std::addressof(m_packet_list.front());
            m_packet_list.pop_front();
            m_packet_factory->Delete(packet);
        }
    }

}
