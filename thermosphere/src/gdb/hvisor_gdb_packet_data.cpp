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

#pragma once

#include "hvisor_gdb_packet_data.hpp"

namespace ams::hvisor::gdb {

    u8 ComputeChecksum(std::string_view packetData)
    {
        return std::accumulate(packetData.cbegin(), packetData.cend(), u8{0u});
    }

    size_t EncodeHex(char *dst, const void *src, size_t len)
    {
        static const char *alphabet = "0123456789abcdef";
        const u8 *src8 = reinterpret_cast<const u8 *>(src);

        for (size_t i = 0; i < len; i++) {
            dst[2 * i] = alphabet[(src8[i] & 0xF0) >> 4];
            dst[2 * i + 1] = alphabet[src8[i] & 0x0F];
        }

        return 2 * len;
    }

    size_t DecodeHex(void *dst, std::string_view data)
    {
        size_t i = 0;
        u8 *dst8 = reinterpret_cast<u8 *>(dst);
        for (i = 0; i < data.size() / 2; i++) {
            auto bOpt = DecodeHexByte(data);
            if (!bOpt) {
                return i;
            }

            dst8[i] = *bOpt;
            data.remove_prefix(2);
        }

        return i;
    }

    size_t DecodeHex(void *dst, const void *src, size_t len)
    {
        return DecodeHex(dst, std::string_view{reinterpret_cast<const char *>(src), len});
    }

    size_t EscapeBinaryData(size_t *encodedCount, void *dst, const void *src, size_t len, size_t maxLen)
    {
        u8 *dst8 = reinterpret_cast<u8 *>(dst);
        const u8 *src8 = reinterpret_cast<const u8 *>(src);
        len = std::min(len, maxLen);

        u8 *dstMax = dst8 + len;

        while (dst8 < dstMax) {
            if (*src8 == '$' || *src8 == '#' || *src8 == '}' || *src8 == '*') {
                if (dst8 + 1 >= dstMax) {
                    break;
                }
                *dst8++ = '}';
                *dst8++ = *src8++ ^ 0x20;
            }
            else {
                *dst8++ = *src8++;
            }
        }

        *encodedCount = dst8 - reinterpret_cast<u8 *>(dst);
        return src8 - reinterpret_cast<const u8 *>(src);
    }

    size_t UnescapeBinaryData(void *dst, const void *src, size_t len)
    {
        u8 *dst8 = reinterpret_cast<u8 *>(dst);
        const u8 *src8 = reinterpret_cast<const u8 *>(src);
        const u8 *srcEnd = src8 + len;

        while (src8 < srcEnd) {
            if (*src8 == '}') {
                src8++;
                *dst8++ = *src8++ ^ 0x20;
            } else {
                *dst8++ = *src8++;
            }
        }

        return dst8 - reinterpret_cast<u8 *>(dst);
    }

}
