/*
 * Copyright (c) 2019-2020 Adubbz, Atmosphère-NX
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
#include <stratosphere/fs/fs_rights_id.hpp>

namespace ams::ncm {

    struct RightsId {
        fs::RightsId id;
        u8 key_generation;
        u8 reserved[7];
    };
    static_assert(sizeof(RightsId) == 0x18);
    static_assert(util::is_pod<RightsId>::value);

    inline bool operator==(const RightsId &lhs, const RightsId &rhs) {
        return std::tie(lhs.id, lhs.key_generation) == std::tie(rhs.id, rhs.key_generation);
    }

    inline bool operator!=(const RightsId &lhs, const RightsId &rhs) {
        return !(lhs == rhs);
    }

    inline bool operator<(const RightsId &lhs, const RightsId &rhs) {
        return std::tie(lhs.id, lhs.key_generation) < std::tie(rhs.id, rhs.key_generation);
    }
}
