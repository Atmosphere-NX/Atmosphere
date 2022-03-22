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
#include "htclow_ctrl_packet_factory.hpp"

namespace ams::htclow::ctrl {

    std::unique_ptr<HtcctrlPacket, HtcctrlPacketDeleter> HtcctrlPacketFactory::MakeSendPacketCommon(int body_size) {
        /* Allocate memory for the packet. */
        if (void *buffer = m_allocator->Allocate(sizeof(HtcctrlPacket), alignof(HtcctrlPacket)); buffer != nullptr) {
            /* Convert the buffer to a packet. */
            HtcctrlPacket *packet = static_cast<HtcctrlPacket *>(buffer);

            /* Construct the packet. */
            std::construct_at(packet, m_allocator, body_size + sizeof(HtcctrlPacketHeader));

            /* Create the unique pointer. */
            std::unique_ptr<HtcctrlPacket, HtcctrlPacketDeleter> ptr(packet, HtcctrlPacketDeleter{m_allocator});

            /* Set packet header fields. */
            if (ptr && ptr->IsAllocationSucceeded()) {
                HtcctrlPacketHeader *header = ptr->GetHeader();

                header->signature   = HtcctrlSignature;
                header->sequence_id = m_sequence_id++;
                header->reserved    = 0;
                header->body_size   = body_size;
                header->version     = 1;
                header->channel     = {};
                header->share       = 0;
            }

            return ptr;
        } else {
            return std::unique_ptr<HtcctrlPacket, HtcctrlPacketDeleter>(nullptr, HtcctrlPacketDeleter{m_allocator});
        }
    }

    std::unique_ptr<HtcctrlPacket, HtcctrlPacketDeleter> HtcctrlPacketFactory::MakeSuspendPacket() {
        auto packet = this->MakeSendPacketCommon(0);
        if (packet && packet->IsAllocationSucceeded()) {
            packet->GetHeader()->packet_type = HtcctrlPacketType_SuspendFromTarget;
        }

        return packet;
    }

    std::unique_ptr<HtcctrlPacket, HtcctrlPacketDeleter> HtcctrlPacketFactory::MakeResumePacket() {
        auto packet = this->MakeSendPacketCommon(0);
        if (packet && packet->IsAllocationSucceeded()) {
            packet->GetHeader()->packet_type = HtcctrlPacketType_ResumeFromTarget;
        }

        return packet;
    }

    std::unique_ptr<HtcctrlPacket, HtcctrlPacketDeleter> HtcctrlPacketFactory::MakeReadyPacket(const void *body, int body_size) {
        auto packet = this->MakeSendPacketCommon(body_size);
        if (packet && packet->IsAllocationSucceeded()) {
            packet->GetHeader()->packet_type = HtcctrlPacketType_ReadyFromTarget;

            std::memcpy(packet->GetBody(), body, packet->GetBodySize());
        }

        return packet;
    }

    std::unique_ptr<HtcctrlPacket, HtcctrlPacketDeleter> HtcctrlPacketFactory::MakeInformationPacket(const void *body, int body_size) {
        auto packet = this->MakeSendPacketCommon(body_size);
        if (packet && packet->IsAllocationSucceeded()) {
            packet->GetHeader()->packet_type = HtcctrlPacketType_InformationFromTarget;

            std::memcpy(packet->GetBody(), body, packet->GetBodySize());
        }

        return packet;
    }

    std::unique_ptr<HtcctrlPacket, HtcctrlPacketDeleter> HtcctrlPacketFactory::MakeDisconnectPacket() {
        auto packet = this->MakeSendPacketCommon(0);
        if (packet) {
            packet->GetHeader()->packet_type = HtcctrlPacketType_DisconnectFromTarget;
        }

        return packet;
    }

    std::unique_ptr<HtcctrlPacket, HtcctrlPacketDeleter> HtcctrlPacketFactory::MakeConnectPacket(const void *body, int body_size) {
        auto packet = this->MakeSendPacketCommon(body_size);
        if (packet && packet->IsAllocationSucceeded()) {
            packet->GetHeader()->packet_type = HtcctrlPacketType_ConnectFromTarget;

            std::memcpy(packet->GetBody(), body, packet->GetBodySize());
        }

        return packet;
    }

    std::unique_ptr<HtcctrlPacket, HtcctrlPacketDeleter> HtcctrlPacketFactory::MakeBeaconResponsePacket(const void *body, int body_size) {
        auto packet = this->MakeSendPacketCommon(body_size);
        if (packet && packet->IsAllocationSucceeded()) {
            packet->GetHeader()->packet_type = HtcctrlPacketType_BeaconResponse;

            std::memcpy(packet->GetBody(), body, packet->GetBodySize());
        }

        return packet;
    }

    void HtcctrlPacketFactory::Delete(HtcctrlPacket *packet) {
        HtcctrlPacketDeleter{m_allocator}(packet);
    }

}
