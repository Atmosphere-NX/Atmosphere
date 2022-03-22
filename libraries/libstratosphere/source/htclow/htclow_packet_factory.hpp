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
#include "htclow_packet.hpp"

namespace ams::htclow {

    class PacketFactory {
        private:
            mem::StandardAllocator *m_allocator;
        public:
            PacketFactory(mem::StandardAllocator *allocator) : m_allocator(allocator) { /* ... */ }

            std::unique_ptr<Packet, PacketDeleter> MakeDataPacket(impl::ChannelInternalType channel, s16 version, const void *body, int body_size, u64 share, u32 offset);
            std::unique_ptr<Packet, PacketDeleter> MakeMaxDataPacket(impl::ChannelInternalType channel, s16 version, u64 share);
            std::unique_ptr<Packet, PacketDeleter> MakeErrorPacket(impl::ChannelInternalType channel);

            void Delete(Packet *packet);
         private:
            std::unique_ptr<Packet, PacketDeleter> MakeSendPacketCommon(impl::ChannelInternalType channel, s16 version, int body_size);
    };

}
