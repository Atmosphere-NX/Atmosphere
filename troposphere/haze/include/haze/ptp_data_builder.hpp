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

    class PtpDataBuilder final {
        private:
            AsyncUsbServer *m_server;
            size_t m_transmitted_size;
            size_t m_offset;
            u8 *m_data;
            bool m_disabled;

        private:
            template <typename T>
            constexpr static size_t Strlen(const T *str) {
                const T *s = str;
                for (; *s; ++s) {
                }
                return s - str;
            }

            Result Flush() {
                ON_SCOPE_EXIT {
                    m_transmitted_size += m_offset;
                    m_offset = 0;
                };

                if (m_disabled) {
                    R_SUCCEED();
                } else {
                    R_RETURN(m_server->WritePacket(m_data, m_offset));
                }
            }

        public:
            constexpr explicit PtpDataBuilder(void *data, AsyncUsbServer *server) : m_server(server),  m_transmitted_size(), m_offset(), m_data(static_cast<u8 *>(data)), m_disabled() { /* ... */ }

            Result Commit() {
                if (m_offset > 0) {
                    /* If there is remaining data left to write, write it now. */
                    R_TRY(this->Flush());
                }

                if (m_transmitted_size % PtpUsbBulkHighSpeedMaxPacketLength == 0) {
                    /* If the transmission size was a multiple of wMaxPacketSize, send a zero length packet. */
                    R_TRY(this->Flush());
                }

                R_SUCCEED();
            }

            Result AddBuffer(const u8 *buffer, u32 count) {
                while (count > 0) {
                    /* Calculate how many bytes we can write now. */
                    u32 write_size = std::min<u32>(count, haze::UsbBulkPacketBufferSize - m_offset);

                    /* Write this number of bytes. */
                    std::memcpy(m_data + m_offset, buffer, write_size);
                    m_offset += write_size;
                    buffer += write_size;
                    count -= write_size;

                    /* If we cannot write more bytes now, flush. */
                    if (m_offset == haze::UsbBulkPacketBufferSize) {
                        R_TRY(this->Flush());
                    }
                }

                R_SUCCEED();
            }

            template <typename T>
            Result Add(T value) {
                const auto bytes = std::bit_cast<std::array<u8, sizeof(T)>>(value);

                R_RETURN(this->AddBuffer(bytes.data(), bytes.size()));
            }

            Result AddDataHeader(PtpUsbBulkContainer &request, u32 data_size) {
                R_TRY(this->Add<u32>(PtpUsbBulkHeaderLength + data_size));
                R_TRY(this->Add<u16>(PtpUsbBulkContainerType_Data));
                R_TRY(this->Add<u16>(request.code));
                R_TRY(this->Add<u32>(request.trans_id));

                R_SUCCEED();
            }

            Result AddResponseHeader(PtpUsbBulkContainer &request, PtpResponseCode code, u32 params_size) {
                R_TRY(this->Add<u32>(PtpUsbBulkHeaderLength + params_size));
                R_TRY(this->Add<u16>(PtpUsbBulkContainerType_Response));
                R_TRY(this->Add<u16>(code));
                R_TRY(this->Add<u32>(request.trans_id));

                R_SUCCEED();
            }

            template <typename F>
            Result WriteVariableLengthData(PtpUsbBulkContainer &request, F &&func) {
                m_disabled = true;
                R_TRY(func());
                R_TRY(this->Commit());

                const size_t data_size = m_transmitted_size;

                m_transmitted_size = 0;
                m_offset = 0;
                m_disabled = false;

                R_TRY(this->AddDataHeader(request, data_size));
                R_TRY(func());
                R_TRY(this->Commit());

                R_SUCCEED();
            }

            template <typename T>
            Result AddString(const T *str) {
                u8 len = static_cast<u8>(std::min(Strlen(str), PtpStringMaxLength - 1));
                if (len > 0) {
                    /* Write null terminator for non-empty strings. */
                    R_TRY(this->Add<u8>(len + 1));

                    for (size_t i = 0; i < len; i++) {
                        R_TRY(this->Add<u16>(str[i]));
                    }

                    R_TRY(this->Add<u16>(0));
                } else {
                    R_TRY(this->Add<u8>(len));
                }

                R_SUCCEED();
            }

            template <typename T>
            Result AddArray(const T *ary, u32 count) {
                R_TRY(this->Add<u32>(count));

                for (size_t i = 0; i < count; i++) {
                    R_TRY(this->Add<T>(ary[i]));
                }

                R_SUCCEED();
            }
    };

}
