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
#include <stratosphere/fs/fs_content_attributes.hpp>

namespace ams::fs {

    /* ACCURATE_TO_VERSION: Unknown */
    union RightsId {
        u8 data[0x10];
        u64 data64[2];
    };
    static_assert(sizeof(RightsId) == 0x10);
    static_assert(util::is_pod<RightsId>::value);

    inline bool operator==(const RightsId &lhs, const RightsId &rhs) {
        return std::memcmp(std::addressof(lhs), std::addressof(rhs), sizeof(RightsId)) == 0;
    }

    inline bool operator!=(const RightsId &lhs, const RightsId &rhs) {
        return !(lhs == rhs);
    }

    inline bool operator<(const RightsId &lhs, const RightsId &rhs) {
        return std::memcmp(std::addressof(lhs), std::addressof(rhs), sizeof(RightsId)) < 0;
    }

    constexpr inline RightsId InvalidRightsId = {};

    /* Rights ID API */
    Result GetRightsId(RightsId *out, const char *path, fs::ContentAttributes attr);
    Result GetRightsId(RightsId *out, u8 *out_key_generation, const char *path, fs::ContentAttributes attr);

}