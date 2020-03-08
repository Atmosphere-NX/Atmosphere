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
#include <stratosphere/os.hpp>
#include <stratosphere/ncm/ncm_ids.hpp>

namespace ams::cfg {

    namespace impl {

        enum OverrideStatusFlag : u64 {
            OverrideStatusFlag_Hbl             = BIT(0),
            OverrideStatusFlag_ProgramSpecific = BIT(1),
            OverrideStatusFlag_CheatEnabled    = BIT(2),
        };

    }

    struct OverrideStatus {
        u64 keys_held;
        u64 flags;

        constexpr inline u64 GetKeysHeld() const { return this->keys_held; }

        #define DEFINE_FLAG_ACCESSORS(flag) \
        constexpr inline bool Is##flag() const { return this->flags & impl::OverrideStatusFlag_##flag; } \
        constexpr inline void Set##flag() { this->flags |= impl::OverrideStatusFlag_##flag; } \
        constexpr inline void Clear##flag() { this->flags &= ~u64(impl::OverrideStatusFlag_##flag); }

        DEFINE_FLAG_ACCESSORS(Hbl)
        DEFINE_FLAG_ACCESSORS(ProgramSpecific)
        DEFINE_FLAG_ACCESSORS(CheatEnabled)

        #undef DEFINE_FLAG_ACCESSORS
    };

    static_assert(sizeof(OverrideStatus) == 0x10, "sizeof(OverrideStatus)");
    static_assert(std::is_pod<OverrideStatus>::value, "std::is_pod<OverrideStatus>::value");

    constexpr inline bool operator==(const OverrideStatus &lhs, const OverrideStatus &rhs) {
        return std::memcmp(&lhs, &rhs, sizeof(lhs)) == 0;
    }

    constexpr inline bool operator!=(const OverrideStatus &lhs, const OverrideStatus &rhs) {
        return !(lhs == rhs);
    }

}
