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

#include "../defines.hpp"
#include <string_view>

namespace ams::hyp::gdb {

    constexpr unsigned long DecodeHexDigit(char src)
    {
        switch (src) {
            case '0' ... '9': return  0 + (src - '0');
            case 'a' ... 'f': return 10 + (src - 'a');
            case 'A' ... 'F': return 10 + (src - 'A');
            default:
                return 16;
        }
    }

    constexpr auto ParseInteger(std::string_view str, u32 base = 0, bool allowPrefix = true)
    {
        unsigned long res = 0;
        long mult = 1;
        auto errval = std::tuple{false, 0ul, str.end()};

        if ((base == 0 && !allowPrefix) || base > 16 || str.empty()) {
            return errval;
        }

        // Check for +, -
        if (str[0] == '+') {
            if (!allowPrefix) {
                return errval;
            }
            str.remove_prefix(1);
        } else if (str[0] == '-') {
            if (!allowPrefix) {
                return errval;
            }
            str.remove_prefix(1);
            mult = -1;
        }

        if (str.empty()) {
            // Oops
            return errval;
        }

        // Now, check for 0x or leading 0
        if (str.size() >= 2 && str[0] == '0' && str[1] == 'x') {
            if (!allowPrefix || (base != 16 && base != 0)) {
                return errval;
            } else {
                str.remove_prefix(2);
                base = 16;
            }
        } else if (base == 0 || str[0] == '0') {
            if (!allowPrefix) {
                return errval;
            }
            base = 8;
        } else if (base == 0) {
            base = 10;
        }

        if (str.empty()) {
            // Oops
            return errval;
        }

        auto it = str.begin();
        for (; it != str.end(); ++it) {
            unsigned long v = DecodeHexDigit(*it);
            if (v >= base) {
                break;
            }

            res *= base;
            res += v;
        }

        return std::tuple{true, res * mult, it};
    }

    std::string_view::iterator ParseIntegerList(unsigned long *dst, std::string_view str, size_t nb, char sep, char lastSep, u32 base, bool allowPrefix);
    std::string_view::iterator ParseHexIntegerList(unsigned long *dst, std::string_view str, size_t nb, char lastSep);

    u8 ComputeChecksum(std::string_view packetData);
    size_t EncodeHex(char *dst, const void *src, size_t len);
    size_t DecodeHex(void *dst, std::string_view data);
    size_t EscapeBinaryData(size_t *encodedCount, void *dst, const void *src, size_t len, size_t maxLen);
    size_t UnescapeBinaryData(void *dst, const void *src, size_t len);

}
