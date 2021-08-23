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
#include <exosphere.hpp>
#include "fusee_display.hpp"

namespace ams::nxboot {

    constexpr inline const size_t SecondaryArchiveSize         = 4_MB + FrameBufferSize;

    constexpr inline const size_t InitialProcessStorageSizeMax = 3_MB / 8;

    struct SecondaryArchiveHeader {
        /* TODO */
        u8 data[1_MB];
    };

    struct SecondaryArchive {
        SecondaryArchiveHeader header;
        u8 kips[8][InitialProcessStorageSizeMax];
        u8 splash_screen_framebuffer[FrameBufferSize];
    };
    static_assert(sizeof(SecondaryArchive) == SecondaryArchiveSize);

    ALWAYS_INLINE const SecondaryArchive &GetSecondaryArchive() { return *reinterpret_cast<const SecondaryArchive *>(0xC0000000); }

}
