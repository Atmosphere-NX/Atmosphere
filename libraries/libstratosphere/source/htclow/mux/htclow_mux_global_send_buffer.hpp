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

namespace ams::htclow {

    class PacketFactory;

}

namespace ams::htclow::mux {

    class GlobalSendBuffer {
        private:
            using PacketList = util::IntrusiveListBaseTraits<Packet>::ListType;
        private:
            PacketFactory *m_packet_factory;
            PacketList m_packet_list;
        public:
            GlobalSendBuffer(PacketFactory *pf) : m_packet_factory(pf), m_packet_list() { /* ... */ }

            Packet *GetNextPacket();

            Result AddPacket(std::unique_ptr<Packet, PacketDeleter> ptr);
            void RemovePacket();
    };

}
