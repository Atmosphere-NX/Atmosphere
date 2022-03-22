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
#include <stratosphere/htclow/htclow_channel_types.hpp>

namespace ams::htclow::impl {

    struct ChannelInternalType {
        ChannelId channel_id;
        s8 reserved;
        ModuleId module_id;
    };
    static_assert(sizeof(ChannelInternalType) == 4);

    constexpr ALWAYS_INLINE ChannelInternalType ConvertChannelType(ChannelType channel) {
        return {
            .channel_id = channel._channel_id,
            .reserved   = 0,
            .module_id  = channel._module_id,
        };
    }

    constexpr ALWAYS_INLINE bool operator==(const ChannelInternalType &lhs, const ChannelInternalType &rhs) {
        return lhs.module_id == rhs.module_id && lhs.reserved == rhs.reserved && lhs.channel_id == rhs.channel_id;
    }

    constexpr ALWAYS_INLINE bool operator!=(const ChannelInternalType &lhs, const ChannelInternalType &rhs) {
        return !(lhs == rhs);
    }

    constexpr ALWAYS_INLINE bool operator<(const ChannelInternalType &lhs, const ChannelInternalType &rhs) {
        return std::tie(lhs.module_id, lhs.reserved, lhs.channel_id) < std::tie(rhs.module_id, rhs.reserved, rhs.channel_id);
    }

    constexpr ALWAYS_INLINE bool operator>(const ChannelInternalType &lhs, const ChannelInternalType &rhs) {
        return rhs < lhs;
    }

    constexpr ALWAYS_INLINE bool operator<=(const ChannelInternalType &lhs, const ChannelInternalType &rhs) {
        return !(lhs > rhs);
    }

    constexpr ALWAYS_INLINE bool operator>=(const ChannelInternalType &lhs, const ChannelInternalType &rhs) {
        return !(lhs < rhs);
    }

}
