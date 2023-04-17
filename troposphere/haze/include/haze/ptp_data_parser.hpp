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

#include <haze/async_usb_server.hpp>
#include <haze/common.hpp>
#include <haze/ptp.hpp>

namespace haze {

    class PtpDataParser final {
        private:
            AsyncUsbServer *m_server;
            u32 m_received_size;
            u32 m_offset;
            u8 *m_data;
            bool m_eot;
        private:
            Result Flush() {
                R_UNLESS(!m_eot, haze::ResultEndOfTransmission());

                m_received_size = 0;
                m_offset = 0;

                ON_SCOPE_EXIT {
                    /* End of transmission occurs when receiving a bulk transfer less than the buffer size. */
                    /* PTP uses zero-length termination, so zero is a possible size to receive. */
                    m_eot = m_received_size < haze::UsbBulkPacketBufferSize;
                };

                R_RETURN(m_server->ReadPacket(m_data, haze::UsbBulkPacketBufferSize, std::addressof(m_received_size)));
            }
        public:
            constexpr explicit PtpDataParser(void *data, AsyncUsbServer *server) : m_server(server), m_received_size(), m_offset(), m_data(static_cast<u8 *>(data)), m_eot() { /* ... */ }

            Result Finalize() {
                /* Read until the transmission completes. */
                while (true) {
                    Result rc = this->Flush();

                    R_SUCCEED_IF(m_eot || haze::ResultEndOfTransmission::Includes(rc));
                    R_TRY(rc);
                }
            }

            Result ReadBuffer(u8 *buffer, u32 count, u32 *out_read_count) {
                *out_read_count = 0;

                while (count > 0) {
                    /* If we cannot read more bytes now, flush. */
                    if (m_offset == m_received_size) {
                        R_TRY(this->Flush());
                    }

                    /* Calculate how many bytes we can read now. */
                    u32 read_size = std::min<u32>(count, m_received_size - m_offset);

                    /* Read this number of bytes. */
                    std::memcpy(buffer + *out_read_count, m_data + m_offset, read_size);
                    *out_read_count += read_size;
                    m_offset += read_size;
                    count -= read_size;
                }

                R_SUCCEED();
            }

            template <typename T>
            Result Read(T *out_t) {
                u32 read_count;
                u8 bytes[sizeof(T)];

                R_TRY(this->ReadBuffer(bytes, sizeof(T), std::addressof(read_count)));

                std::memcpy(out_t, bytes, sizeof(T));

                R_SUCCEED();
            }

            /* NOTE: out_string must contain room for 256 bytes. */
            /* The result will be null-terminated on successful completion. */
            Result ReadString(char *out_string) {
                u8 len;
                R_TRY(this->Read(std::addressof(len)));

                /* Read characters one by one. */
                for (size_t i = 0; i < len; i++) {
                    u16 chr;
                    R_TRY(this->Read(std::addressof(chr)));

                    *out_string++ = static_cast<char>(chr);
                }

                /* Write null terminator. */
                *out_string++ = '\x00';

                R_SUCCEED();
            }
    };

}
