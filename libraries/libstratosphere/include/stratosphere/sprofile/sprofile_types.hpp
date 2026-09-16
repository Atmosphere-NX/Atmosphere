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
#include <vapours.hpp>

namespace ams::sprofile {

    struct HashKey {
        u8 data[7];

        friend bool operator==(const HashKey &lhs, const HashKey &rhs) {
            return std::memcmp(lhs.data, rhs.data, sizeof(lhs.data)) == 0;
        }

        friend bool operator!=(const HashKey &lhs, const HashKey &rhs) {
            return !(lhs == rhs);
        }
    };
    static_assert(sizeof(HashKey) == 7);
    static_assert(util::is_pod<HashKey>::value);

}
