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
#include <vapours.hpp>

namespace ams::se {

    constexpr inline int Sha256HashSize = crypto::Sha256Generator::HashSize;

    union Sha256Hash {
        u8  bytes[Sha256HashSize / sizeof(u8) ];
        u32 words[Sha256HashSize / sizeof(u32)];
    };

    void CalculateSha256(Sha256Hash *dst, const void *src, size_t src_size);

}
