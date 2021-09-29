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
#include <stratosphere.hpp>
#include "os_utility.hpp"

namespace ams::os::impl {

    void ExpandUnsignedValueToAscii(char *dst, u64 value) {
        /* Determine size needed. */
        constexpr size_t SizeNeeded = (BITSIZEOF(value) / 4) * sizeof(char);

        /* Write null-terminator. */
        dst[SizeNeeded] = '\x00';

        /* Write characters. */
        for (size_t i = 0; i < SizeNeeded; ++i) {
            /* Determine current character. */
            const auto nybble = (value & 0xF);
            const char cur_c  = (nybble < 10) ? ('0' + nybble) : ('A' + nybble - 10);

            /* Write current character. */
            dst[SizeNeeded - 1 - i] = cur_c;

            /* Shift to next nybble. */
            value >>= 4;
        }
    }

}
