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
#include "dmnt2_transport_session.hpp"

namespace ams::dmnt {

    static constexpr size_t GdbPacketBufferSize = 32_KB;

    class GdbPacketIo {
        private:
            os::SdkMutex m_mutex;
            bool m_no_ack;
        public:
            GdbPacketIo() : m_mutex(), m_no_ack(false) { /* ... */ }

            void SetNoAck() { m_no_ack = true; }

            void SendPacket(bool *out_break, const char *src, TransportSession *session);
            char *ReceivePacket(bool *out_break, char *dst, size_t size, TransportSession *session);
    };

}