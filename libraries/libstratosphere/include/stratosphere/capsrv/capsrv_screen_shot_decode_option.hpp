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
#include <vapours.hpp>

namespace ams::capsrv {

    enum ScreenShotDecoderFlag : u64 {
        ScreenShotDecoderFlag_None                  = (0 << 0),
        ScreenShotDecoderFlag_EnableFancyUpsampling = (1 << 0),
        ScreenShotDecoderFlag_EnableBlockSmoothing  = (1 << 1),
    };

    using ScreenShotJpegDecoderFlagType = typename std::underlying_type<ScreenShotDecoderFlag>::type;

    struct ScreenShotDecodeOption {
        ScreenShotJpegDecoderFlagType flags;
        u8 reserved[0x20 - sizeof(ScreenShotJpegDecoderFlagType)];

        static constexpr ScreenShotDecodeOption GetDefaultOption() {
            return ScreenShotDecodeOption{};
        }

        constexpr bool HasJpegDecoderFlag(ScreenShotJpegDecoderFlagType flag) const {
            return (this->flags & flag) != 0;
        }
    };
    static_assert(sizeof(ScreenShotDecodeOption) == 0x20);
    static_assert(sizeof(ScreenShotDecodeOption) == sizeof(::CapsScreenShotDecodeOption));
    static_assert(util::is_pod<ScreenShotDecodeOption>::value);

}
