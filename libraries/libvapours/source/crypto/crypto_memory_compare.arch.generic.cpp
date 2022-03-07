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
#include <vapours.hpp>

namespace ams::crypto {

    bool IsSameBytes(const void *lhs, const void *rhs, size_t size) {
        /* TODO: Should the generic impl be constant time? */
        volatile u8 diff = 0;

        const volatile u8 *lhs8 = static_cast<const volatile u8 *>(lhs);
        const volatile u8 *rhs8 = static_cast<const volatile u8 *>(rhs);
        for (size_t i = 0; i < size; ++i) {
            diff = diff | (lhs8[i] ^ rhs8[i]);
        }

        return diff == 0;
    }

}