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
        static constexpr size_t Size = 0x10;

        u8 data[Size];

        bool operator==(const Uuid &rhs) const {
            return std::memcmp(this->data, rhs.data, Size) == 0;
        }

        bool operator!=(const Uuid &rhs) const {
            return !(*this == rhs);
        }

        u8 operator[](size_t i) const {
            return this->data[i];
        }
    };

    constexpr inline Uuid InvalidUuid = {};

}
