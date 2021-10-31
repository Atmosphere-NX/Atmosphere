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
#include "dmnt2_gdb_packet_io.hpp"
#include "dmnt2_debug_log.hpp"

namespace ams::dmnt {

    namespace {

        constexpr char BreakCharacter = '\x03'; /* ctrl-c */

        constexpr int DecodeHex(char c) {
            if ('a' <= c && c <= 'f') {
                return 10 + (c - 'a');
            } else if ('A' <= c && c <= 'F') {
                return 10 + (c - 'A');
            } else if ('0' <= c && c <= '9') {
                return 0 + (c - '0');
            } else {
                return -1;
            }
        }

        constexpr char EncodeHex(u8 v) {
            return "0123456789abcdef"[v & 0xF];
        }

    }

    void GdbPacketIo::SendPacket(bool *out_break, const char *src, TransportSession *session) {
        /* Default to not breaked. */
        *out_break = false;

        /* Send a packet. */
        while (true) {
            std::scoped_lock lk(m_mutex);

            size_t len = 0;
            u8 checksum = 0;

            while (src[len] != 0) {
                checksum += static_cast<u8>(src[len++]);
            }

            char buffer[1 + GdbPacketBufferSize + 4];
            buffer[0] = '$';
            std::memcpy(buffer + 1, src, len);
            buffer[1 + len] = '#';
            buffer[2 + len] = EncodeHex(checksum >> 4);
            buffer[3 + len] = EncodeHex(checksum >> 0);
            buffer[4 + len] = 0;

            if (session->PutString(buffer) < 0) {
                /* Log (truncated) copy of packet. */
                AMS_DMNT2_GDB_LOG_ERROR("Failed to send packet %s\n", buffer);
                return;
            }

            if (m_no_ack) {
                return;
            }

            /* Check to see if we need to retransmit. */
            bool retransmit = false;
            do {
                if (const auto char_holder = session->GetChar(); char_holder) {
                    switch (*char_holder) {
                        case BreakCharacter:
                            *out_break = true;
                            return;
                        case '+':
                            return;
                        case '-':
                            retransmit = true;
                            break;
                        default:
                            break;
                    }
                } else {
                    /* Log (truncated) copy of packet. */
                    AMS_DMNT2_GDB_LOG_ERROR("Failed to receive ack for %s\n", buffer);
                    return;
                }
            } while (!retransmit);
        }
    }

    char *GdbPacketIo::ReceivePacket(bool *out_break, char *dst, size_t size, TransportSession *session) {
        /* Default to not breaked. */
        *out_break = false;

        /* Receive a packet. */
        while (true) {
            /* Wait for data to be available. */
            if (!session->WaitToBeReadable()) {
                return nullptr;
            }

            /* Lock ourselves. */
            std::scoped_lock lk(m_mutex);

            /* Verify that we still have data immediately available. */
            if (!session->WaitToBeReadable(0)) {
                continue;
            }

            /* Prepare to parse. */
            enum class State {
                Initial,
                PacketData,
                ChecksumHigh,
                ChecksumLow
            };

            State state = State::Initial;
            u8 checksum = 0;
            int csum_high = -1, csum_low = -1;
            size_t count = 0;

            /* Read characters. */
            while (true) {
                if (const auto char_holder = session->GetChar(); char_holder) {
                    const auto c = *char_holder;

                    switch (state) {
                        case State::Initial:
                            if (c == '$') {
                                state = State::PacketData;
                            } else if (c == BreakCharacter) {
                                *out_break = true;
                                return nullptr;
                            }
                            break;
                        case State::PacketData:
                            /* TODO: Escaped characters. */
                            if (c == '#') {
                                dst[count] = 0;
                                state = State::ChecksumHigh;
                            } else {
                                AMS_ABORT_UNLESS(count < size - 1);
                                checksum += static_cast<u8>(c);
                                dst[count++] = c;
                            }
                            break;
                        case State::ChecksumHigh:
                            csum_high = DecodeHex(c);
                            state = State::ChecksumLow;
                            break;
                        case State::ChecksumLow:
                            csum_low = DecodeHex(c);

                            if (m_no_ack) {
                                return dst;
                            } else {
                                const u8 expectsum = (static_cast<u8>(csum_high) << 4) | (static_cast<u8>(csum_low) << 0);

                                if (csum_high < 0 || csum_low < 0 || checksum != expectsum) {
                                    /* Request retransmission. */
                                    state     = State::Initial;
                                    checksum  = 0;
                                    csum_high = -1;
                                    csum_low  = -1;
                                    count     = 0;
                                    session->PutChar('-');
                                } else {
                                    session->PutChar('+');
                                    return dst;
                                }
                            }

                            break;
                    }
                } else {
                    return nullptr;
                }
            }
        }
    }

}
