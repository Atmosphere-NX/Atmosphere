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
#include <vapours/util/util_endian.hpp>

namespace ams::util {

    template<char A, char B, char C, char D>
    struct FourCC {
        static constexpr u32 Code = IsLittleEndian() ? ((static_cast<u32>(A) << 0x00) | (static_cast<u32>(B) << 0x08) | (static_cast<u32>(C) << 0x10) | (static_cast<u32>(D) << 0x18))
                                                     : ((static_cast<u32>(A) << 0x18) | (static_cast<u32>(B) << 0x10) | (static_cast<u32>(C) << 0x08) | (static_cast<u32>(D) << 0x00));

        static constexpr const char String[] = {A, B, C, D};

        static_assert(sizeof(Code)   == 4);
        static_assert(sizeof(String) == 4);
    };

    template<char A, char B, char C, char D>
    struct ReverseFourCC {
        static constexpr u32 Code = IsLittleEndian() ? ((static_cast<u32>(A) << 0x18) | (static_cast<u32>(B) << 0x10) | (static_cast<u32>(C) << 0x08) | (static_cast<u32>(D) << 0x00))
                                                     : ((static_cast<u32>(A) << 0x00) | (static_cast<u32>(B) << 0x08) | (static_cast<u32>(C) << 0x10) | (static_cast<u32>(D) << 0x18));

        static constexpr const char String[] = {D, C, B, A};

        static_assert(sizeof(Code)   == 4);
        static_assert(sizeof(String) == 4);
    };

}