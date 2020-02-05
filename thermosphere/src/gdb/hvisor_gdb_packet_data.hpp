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

namespace ams::hvisor::gdb {

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
        auto errval = std::tuple{0ul, 0ul};

        size_t total = 0;

        if ((base == 0 && !allowPrefix) || base > 16 || str.empty()) {
            return errval;
        }

        // Check for +, -
        if (str[0] == '+') {
            if (!allowPrefix) {
                return errval;
            }
            str.remove_prefix(1);
            ++total;
        } else if (str[0] == '-') {
            if (!allowPrefix) {
                return errval;
            }
            str.remove_prefix(1);
            mult = -1;
            ++total;
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
                total += 2;
            }
        } else if (base == 0 && str[0] == '0') {
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
            ++total;
        }

        return std::tuple{total, res * mult};
    }

    template<size_t N>
    constexpr auto ParseIntegerList(std::string_view str, u32 base, bool allowPrefix, char sep, char lastSep = '\0')
    {
        // First element is parsed size
        std::array<unsigned long, 1+N> res{ 0 };

        size_t total = 0;
        for (size_t i = 0; i < N && !str.empty(); i++) {
            auto [nread, val] = ParseInteger(str, base, allowPrefix);

            // Parse failure
            if (nread == 0) {
                return res;
            }

            str.remove_prefix(nread);

            // Check separators
            if (i != N - 1) {
                if (str.empty() || str[0] != sep)) {
                    return res;
                }
                str.remove_prefix(1);
                ++total;
            } else if (i == N - 1) {
                if (lastSep == '\0') && !str.empty()) {
                    return res;
                } else if (lastSep != '\0') {
                    if (str.empty() || str[0] != lastSep)) {
                        return res;
                    }
                    str.remove_prefix(1);
                    ++total;
                }
            }

            total += nread;
            res[1 + i] = val;
        }

        res[0] = total;
        return res;
    }

    template<size_t N>
    constexpr auto ParseHexIntegerList(std::string_view str, char lastSep = '\0')
    {
        return ParseIntegerList<N>(str, 16, false, ',', lastSep);
    }

    constexpr std::optional<u8> DecodeHexByte(std::string_view data)
    {
        if (data.size() < 2) {
            return {};
        }

        auto v1 = DecodeHexDigit(data[0]);
        auto v2 = DecodeHexDigit(data[1]);

        if (v1 >= 16 || v2 >= 16) {
            return {};
        }

        return (v1 << 4) | v2;
    }

    u8 ComputeChecksum(std::string_view packetData);
    size_t EncodeHex(char *dst, const void *src, size_t len);
    size_t DecodeHex(void *dst, std::string_view data);

    size_t EscapeBinaryData(size_t *encodedCount, void *dst, const void *src, size_t len, size_t maxLen);
    size_t UnescapeBinaryData(void *dst, const void *src, size_t len);

}
