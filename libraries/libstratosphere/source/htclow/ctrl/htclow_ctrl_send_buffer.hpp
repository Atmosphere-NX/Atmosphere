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
#include "htclow_ctrl_packet.hpp"

namespace ams::htclow::ctrl {

    class HtcctrlPacketFactory;

    class HtcctrlSendBuffer {
        private:
            using PacketList = util::IntrusiveListBaseTraits<HtcctrlPacket>::ListType;
        private:
            HtcctrlPacketFactory *m_packet_factory;
            PacketList m_prior_packet_list;
            PacketList m_posterior_packet_list;
        private:
            bool IsPriorPacket(HtcctrlPacketType packet_type) const;
            bool IsPosteriorPacket(HtcctrlPacketType packet_type) const;

            void CopyPacket(HtcctrlPacketHeader *header, HtcctrlPacketBody *body, int *out_body_size, const HtcctrlPacket &packet);
        public:
            HtcctrlSendBuffer(HtcctrlPacketFactory *pf) : m_packet_factory(pf), m_prior_packet_list(), m_posterior_packet_list() { /* ... */ }

            void AddPacket(std::unique_ptr<HtcctrlPacket, HtcctrlPacketDeleter> ptr);
            void RemovePacket(const HtcctrlPacketHeader &header);

            bool QueryNextPacket(HtcctrlPacketHeader *header, HtcctrlPacketBody *body, int *out_body_size);
    };

}
