/*
 * Copyright (c) 2019-2020 Atmosphère-NX
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

#include <cstdio>

#include "hvisor_gdb_defines_internal.hpp"
#include "hvisor_gdb_packet_data.hpp"

namespace {

    void WriteAck(TransportInterface *iface)
    {
        char c = '+';
        transportInterfaceWriteData(iface, &c, 1);
    }

    int WriteNack(TransportInterface *iface)
    {
        char c = '-';
        transportInterfaceWriteData(iface, &c, 1);
        return 1;
    }

}

namespace ams::hvisor::gdb {

    int Context::ReceivePacket()
    {
        char hdr;
        bool ctrlC = false;
        TransportInterface *iface = m_transportInterface;

        // Read the first character...
        transportInterfaceReadData(iface, &hdr, 1);

        switch (hdr) {
            case '+': {
                // Ack, don't do anything else except maybe NoAckMode state transition
                if (m_noAckSent) {
                    m_noAck = true;
                    m_noAckSent = false;
                }
                return 0;
            }
            case '-':
                // Nack, return the previous packet
                transportInterfaceWriteData(iface, m_buffer, m_lastSentPacketSize);
                return m_lastSentPacketSize;
            case '$':
                // Normal packet, handled below
                break;
            case '\x03':
                // Normal packet (Control-C), handled below
                ctrlC = true;
                break;
            default:
                // Oops, send a nack
                DEBUG("Received a packed with an invalid header from GDB, hdr=%c\n", hdr);
                return WriteNack(iface);
        }

        // We didn't get a nack past this point, read the remaining data if any

        m_buffer[0] = hdr;
        if (ctrlC) {
            // Will never normally happen, but ok
            if (m_state < State::Attached) {
                DEBUG("Received connection from GDB, now attaching...\n");
                Attach();
                m_state = State::Attached;
            }
            return 1;
        }

        size_t delimPos = transportInterfaceReadDataUntil(iface, m_buffer + 1, 4 + GDB_BUF_LEN - 1, '#');
        if (m_buffer[delimPos] != '#' || delimPos == 1) {
            // The packet is malformed, send a nack. Refuse empty packets
            return WriteNack(iface);
        }

        m_commandLetter = m_buffer[1];
        m_commandData = std::string_view{m_buffer + 1, delimPos};

        // Read the checksum
        size_t checksumPos = delimPos + 1;
        transportInterfaceReadData(iface, m_buffer + checksumPos, 2);

        auto checksumOpt = DecodeHexByte(std::string_view{m_buffer + checksumPos, 2});

        if (!checksumOpt || *checksumOpt != ComputeChecksum(m_commandData)) {
            // Malformed or invalid checksum
            return WriteNack(iface);
        } else if (!m_noAck) {
            WriteAck(iface);
        }

        // Remove command letter
        m_commandData.remove_prefix(1);

        // State transitions...
        if (m_state < State::Attached) {
            DEBUG("Received connection from GDB, now attaching...\n");
            Attach();
             m_state = State::Attached;
        }

        // Debug
        /*m_buffer[checksumPos + 2] = '\0';
        DEBUGRAW("->");
        DEBUGRAW(m_buffer);
        DEBUGRAW("\n");*/

        return static_cast<int>(delimPos + 2);
    }

    int Context::DoSendPacket(size_t len)
    {
        transportInterfaceWriteData(m_transportInterface, m_buffer, len);
        m_lastSentPacketSize = len;

        // Debugging:
        /*m_buffer[len] = 0;
        DEBUGRAW("<-");
        DEBUGRAW(ctx->buffer);
        DEBUGRAW("\n");*/

        return static_cast<int>(len);
    }

    int Context::SendPacket(std::string_view packetData, char hdr)
    {
        u8 checksum = ComputeChecksum(packetData);
        if (packetData.data() != m_buffer + 1) {
            std::memmove(m_buffer + 1, packetData.data(), packetData.size());
        }

        size_t checksumPos = 1 + packetData.size() + 1;
        m_buffer[0] = '$';
        m_buffer[checksumPos - 1] = '#';
        EncodeHex(m_buffer + checksumPos, &checksum, 1);

        return DoSendPacket(4 + packetData.size());
    }

    int Context::SendFormattedPacket(const char *packetDataFmt, ...)
    {
        va_list args;

        va_start(args, packetDataFmt);
        int n = vsprintf(m_buffer + 1, packetDataFmt, args);
        va_end(args);

        if (n < 0) {
            return -1;
        } else {
            return SendPacket(std::string_view{m_buffer + 1, n});
        }
    }

    int Context::SendHexPacket(const void *packetData, size_t len)
    {
        if (4 + 2 * len < GDB_BUF_LEN) {
            return -1;
        }

        EncodeHex(m_buffer + 1, packetData, len);
        return SendPacket(std::string_view{m_buffer + 1, 2 * len});
    }

    int Context::SendStreamData(std::string_view streamData, size_t offset, size_t length, bool forceEmptyLast)
    {
        size_t totalSize = streamData.size();

        // GDB_BUF_LEN does not include the usual $#<1-byte checksum>
        length = std::min(length, GDB_BUF_LEN - 1ul);

        char letter;

        if ((forceEmptyLast && offset >= totalSize) || (!forceEmptyLast && offset + length >= totalSize)) {
            length = offset >= totalSize ? 0 : totalSize - offset;
            letter = 'l';
        } else {
            letter = 'm';
        }

        // Note: ctx->buffer[0] = '$'
        if (streamData.data() + offset != m_buffer + 2) {
            memmove(m_buffer + 2, streamData.data() + offset, length);
        }

        m_buffer[1] = letter; 
        return SendPacket(std::string_view{m_buffer + 1, 1 + length});
    }

    int Context::ReplyOk()
    {
        return SendPacket("OK");
    }

    int Context::ReplyEmpty()
    {
        return SendPacket("");
    }

    int Context::ReplyErrno(int no)
    {
        u8 no8 = static_cast<u8>(no);
        char resp[] = "E00";
        EncodeHex(resp + 1, &no8, 1);
        return SendPacket(resp);
    }
}
