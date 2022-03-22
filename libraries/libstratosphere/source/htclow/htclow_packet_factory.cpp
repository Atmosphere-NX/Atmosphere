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
#include "htclow_packet_factory.hpp"

namespace ams::htclow {

    void PacketFactory::Delete(Packet *packet) {
        PacketDeleter{m_allocator}(packet);
    }

    std::unique_ptr<Packet, PacketDeleter> PacketFactory::MakeSendPacketCommon(impl::ChannelInternalType channel, s16 version, int body_size) {
        /* Allocate memory for the packet. */
        if (void *buffer = m_allocator->Allocate(sizeof(Packet), alignof(Packet)); buffer != nullptr) {
            /* Convert the buffer to a packet. */
            Packet *packet = static_cast<Packet *>(buffer);

            /* Construct the packet. */
            std::construct_at(packet, m_allocator, body_size + sizeof(PacketHeader));

            /* Create the unique pointer. */
            std::unique_ptr<Packet, PacketDeleter> ptr(packet, PacketDeleter{m_allocator});

            /* Set packet header fields. */
            if (ptr && ptr->IsAllocationSucceeded()) {
                PacketHeader *header = ptr->GetHeader();

                header->signature = HtcGen2Signature;
                header->offset    = 0;
                header->reserved  = 0;
                header->body_size = body_size;
                header->version   = version;
                header->channel   = channel;
                header->share     = 0;
            }

            return ptr;
        } else {
            return std::unique_ptr<Packet, PacketDeleter>(nullptr, PacketDeleter{m_allocator});
        }
    }

    std::unique_ptr<Packet, PacketDeleter> PacketFactory::MakeDataPacket(impl::ChannelInternalType channel, s16 version, const void *body, int body_size, u64 share, u32 offset) {
        auto packet = this->MakeSendPacketCommon(channel, version, body_size);
        if (packet) {
            PacketHeader *header = packet->GetHeader();

            header->packet_type = PacketType_Data;
            header->offset      = offset;
            header->share       = share;

            packet->CopyBody(body, body_size);

            AMS_ASSERT(packet->GetBodySize() == body_size);
        }

        return packet;
    }


    std::unique_ptr<Packet, PacketDeleter> PacketFactory::MakeMaxDataPacket(impl::ChannelInternalType channel, s16 version, u64 share) {
        auto packet = this->MakeSendPacketCommon(channel, version, 0);
        if (packet) {
            PacketHeader *header = packet->GetHeader();

            header->packet_type = PacketType_MaxData;
            header->share       = share;
        }

        return packet;
    }

    std::unique_ptr<Packet, PacketDeleter> PacketFactory::MakeErrorPacket(impl::ChannelInternalType channel) {
        auto packet = this->MakeSendPacketCommon(channel, 0, 0);
        if (packet) {
            PacketHeader *header = packet->GetHeader();

            header->packet_type = PacketType_Error;
        }

        return packet;
    }

}
