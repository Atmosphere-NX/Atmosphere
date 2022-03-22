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
#include "../htclow_packet.hpp"
#include "htclow_mux_ring_buffer.hpp"

namespace ams::htclow {

    class PacketFactory;

}

namespace ams::htclow::mux {

    class SendBuffer {
        private:
            using PacketList = util::IntrusiveListBaseTraits<Packet>::ListType;
        private:
            impl::ChannelInternalType m_channel;
            PacketFactory *m_packet_factory;
            RingBuffer m_ring_buffer;
            PacketList m_packet_list;
            s16 m_version;
            bool m_flow_control_enabled;
            size_t m_max_packet_size;
        private:
            bool IsPriorPacket(PacketType packet_type) const;

            void MakeDataPacketHeader(PacketHeader *header, int body_size, s16 version, u64 share, u32 offset) const;

            void CopyPacket(PacketHeader *header, PacketBody *body, int *out_body_size, const Packet &packet);
        public:
            SendBuffer(impl::ChannelInternalType channel, PacketFactory *pf);
            ~SendBuffer();

            void SetVersion(s16 version);
            void SetFlowControlEnabled(bool en);

            bool QueryNextPacket(PacketHeader *header, PacketBody *body, int *out_body_size, u64 max_data, u64 total_send_size, bool has_share, u64 share);

            void AddPacket(std::unique_ptr<Packet, PacketDeleter> ptr);
            void RemovePacket(const PacketHeader &header);

            size_t AddData(const void *data, size_t size);

            void SetBuffer(void *buffer, size_t buffer_size);
            void SetReadOnlyBuffer(const void *buffer, size_t buffer_size);
            void SetMaxPacketSize(size_t max_packet_size);

            bool Empty();

            void Clear();
    };

}
