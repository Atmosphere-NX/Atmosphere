/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
#include "dmnt2_gdb_packet_io.hpp"

namespace ams::dmnt {

    class GdbPacketParser {
        private:
            char *m_receive_packet;
            char *m_reply_packet;
            char m_buffer[GdbPacketBufferSize / 2];
            bool m_64_bit;
            bool m_no_ack;
        public:
            GdbPacketParser(char *recv, char *reply, bool is_64_bit) : m_receive_packet(recv), m_reply_packet(reply), m_64_bit(is_64_bit), m_no_ack(false) { /* ... */ }

            void Process();

            bool Is64Bit() const { return m_64_bit; }
        private:
            void q();

            void qSupported();
            void qXferFeaturesRead();
    };

}