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

namespace ams::ncm {

    struct FirmwareVariationInfo {
        bool refer_to_base;
        u8   _0x1[3];
        u32  content_meta_count;
        u8   reserved[0x18];
    };

    struct FirmwareVariationId {
        u32 value;
    };

    constexpr inline bool operator==(const FirmwareVariationId &lhs, const FirmwareVariationId &rhs) {
        return lhs.value == rhs.value;
    }

    constexpr inline bool operator!=(const FirmwareVariationId &lhs, const FirmwareVariationId &rhs) {
        return lhs.value != rhs.value;
    }

}
