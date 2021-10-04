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
#include <stratosphere.hpp>

namespace ams::htclow::driver {

    constexpr inline u8 TmipcVersion = 5;

    struct TmipcHeader {
        u32 service_id;
        u32 reserved_00;
        u16 reserved_01;
        u8  reserved_02;
        u8  version;
        u32 data_len;
        u32 reserved[4];
    };
    static_assert(util::is_pod<TmipcHeader>::value);
    static_assert(sizeof(TmipcHeader) == 0x20);

    s32 MakeAutoConnectIpv4RequestPacket(char *dst, size_t dst_size, const socket::SockAddrIn &sockaddr);
    s32 MakeBeaconResponsePacket(char *dst, size_t dst_size);

}
