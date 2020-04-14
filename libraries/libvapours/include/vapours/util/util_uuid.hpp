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
#include <vapours/common.hpp>
#include <vapours/assert.hpp>

namespace ams::util {

    struct Uuid {
        static constexpr size_t Size       = 0x10;
        static constexpr size_t StringSize = sizeof("XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX");

        u8 data[Size];

        friend bool operator==(const Uuid &lhs, const Uuid &rhs) {
            return std::memcmp(lhs.data, rhs.data, Size) == 0;
        }

        friend bool operator!=(const Uuid &lhs, const Uuid &rhs) {
            return !(lhs == rhs);
        }

        const char *ToString(char *dst, size_t dst_size) const {
            std::snprintf(dst, dst_size, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                         this->data[ 0], this->data[ 1], this->data[ 2], this->data[ 3], this->data[ 4], this->data[ 5], this->data[ 6], this->data[ 7],
                         this->data[ 8], this->data[ 9], this->data[10], this->data[11], this->data[12], this->data[13], this->data[14], this->data[15]);

            return dst;
        }

        void FromString(const char *str) {
            char buf[2 + 1] = {};
            char *end;
            s32 i = 0;

            for (/* ... */; i <  4; ++i, str += 2) {
                std::memcpy(buf, str, 2);
                this->data[i] = static_cast<u8>(std::strtoul(buf, std::addressof(end), 16));
            }
            ++str;

            for (/* ... */; i <  6; ++i, str += 2) {
                std::memcpy(buf, str, 2);
                this->data[i] = static_cast<u8>(std::strtoul(buf, std::addressof(end), 16));
            }
            ++str;

            for (/* ... */; i <  8; ++i, str += 2) {
                std::memcpy(buf, str, 2);
                this->data[i] = static_cast<u8>(std::strtoul(buf, std::addressof(end), 16));
            }
            ++str;

            for (/* ... */; i < 10; ++i, str += 2) {
                std::memcpy(buf, str, 2);
                this->data[i] = static_cast<u8>(std::strtoul(buf, std::addressof(end), 16));
            }
            ++str;

            for (/* ... */; i < 16; ++i, str += 2) {
                std::memcpy(buf, str, 2);
                this->data[i] = static_cast<u8>(std::strtoul(buf, std::addressof(end), 16));
            }
        }
    };

    constexpr inline Uuid InvalidUuid = {};

}
