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

namespace ams::socket {

    using InAddrT  = u32;
    using InPortT  = u16;
    using SockLenT = u32;
    using NfdsT    = u64;
    using FdMask   = u64;

    constexpr inline unsigned int FdSetSize = 0x400;

    template<u32 A, u32 B, u32 C, u32 D>
    constexpr inline InAddrT EncodeInAddr = util::ConvertToBigEndian(InAddrT{(A << 24) | (B << 16) | (C << 8) | (D << 0)});

    constexpr inline InAddrT InAddr_Any       = EncodeInAddr<  0,   0,   0,   0>;
    constexpr inline InAddrT InAddr_Broadcast = EncodeInAddr<255, 255, 255, 255>;
    constexpr inline InAddrT InAddr_None      = EncodeInAddr<255, 255, 255, 255>;
    constexpr inline InAddrT InAddr_Loopback  = EncodeInAddr<127,   0,   0,   1>;

    enum class Family : u8 {
        Af_Unspec = 0,
        Pf_Unspec = Af_Unspec,

        Af_Inet   = 2,
        Pf_Inet   = Af_Inet,

        Af_Route  = 17,
        Pf_Route  = Af_Route,

        Af_Link   = 18,
        Pf_Link   = Af_Link,

        Af_Inet6  = 28,
        Pf_Inet6  = Af_Inet6,

        Af_Max    = 42,
        Pf_Max    = Af_Max
    };

    struct HostEnt {
        char *h_name;
        char **h_aliases;
        Family h_addrtype;
        int h_length;
        char **h_addr_list;
    };

    struct InAddr {
        InAddrT s_addr;
    };

}
