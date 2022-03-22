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

    enum class Protocol : s32 {
        IpProto_Ip      = 0,
        IpProto_Icmp    = 1,

        IpProto_Tcp     = 6,

        IpProto_Udp     = 17,

        IpProto_UdpLite = 136,

        IpProto_Raw     = 255,

        IpProto_Max     = 256,
    };

    enum class Type : u32 {
        Sock_Default   = 0,
        Sock_Stream    = 1,
        Sock_Dgram     = 2,
        Sock_Raw       = 3,
        Sock_SeqPacket = 5,

        Sock_NonBlock  = 0x20000000,
    };

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

    enum class MsgFlag : s32 {
        MsgFlag_None    = (0 << 0),
        /* ... */
        MsgFlag_WaitAll = (1 << 6),
        /* ... */
    };

    enum class ShutdownMethod : u32 {
        Shut_Rd   = 0,
        Shut_Wr   = 1,
        Shut_RdWr = 2,
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

    enum class AddrInfoFlag : u32 {
        Ai_None        = (0 <<  0),
        Ai_Passive     = (1 <<  0),
        Ai_CanonName   = (1 <<  1),
        Ai_NumericHost = (1 <<  2),
        Ai_NumericServ = (1 <<  3),

        Ai_AddrConfig  = (1 << 10),
    };

    struct SockAddr {
        u8 sa_len;
        Family sa_family;
        char sa_data[14];
    };

    struct SockAddrIn {
        u8 sin_len;
        Family sin_family;
        InPortT sin_port;
        InAddr sin_addr;
        u8 sin_zero[8];
    };
    static_assert(sizeof(SockAddr) == sizeof(SockAddrIn));

    struct AddrInfo {
        AddrInfoFlag ai_flags;
        Family ai_family;
        Type ai_socktype;
        Protocol ai_protocol;
        SockLenT ai_addrlen;
        SockAddr *ai_addr;
        char *ai_canonname;
        AddrInfo *ai_next;
    };

    #define AMS_SOCKET_IMPL_DEFINE_ENUM_OPERATORS(__ENUM__) \
        constexpr inline __ENUM__ operator | (__ENUM__  lhs, __ENUM__ rhs) { return static_cast<__ENUM__>(static_cast<std::underlying_type_t<__ENUM__>>(lhs) | static_cast<std::underlying_type_t<__ENUM__>>(rhs)); } \
        constexpr inline __ENUM__ operator |=(__ENUM__ &lhs, __ENUM__ rhs) { return lhs = lhs | rhs; }                                                                                                                \
        constexpr inline __ENUM__ operator & (__ENUM__  lhs, __ENUM__ rhs) { return static_cast<__ENUM__>(static_cast<std::underlying_type_t<__ENUM__>>(lhs) & static_cast<std::underlying_type_t<__ENUM__>>(rhs)); } \
        constexpr inline __ENUM__ operator &=(__ENUM__ &lhs, __ENUM__ rhs) { return lhs = lhs & rhs; }                                                                                                                \
        constexpr inline __ENUM__ operator ^ (__ENUM__  lhs, __ENUM__ rhs) { return static_cast<__ENUM__>(static_cast<std::underlying_type_t<__ENUM__>>(lhs) ^ static_cast<std::underlying_type_t<__ENUM__>>(rhs)); } \
        constexpr inline __ENUM__ operator ^=(__ENUM__ &lhs, __ENUM__ rhs) { return lhs = lhs ^ rhs; }                                                                                                                \
        constexpr inline __ENUM__ operator ~ (__ENUM__ e)                  { return static_cast<__ENUM__>(~static_cast<std::underlying_type_t<__ENUM__>>(e)); }

    AMS_SOCKET_IMPL_DEFINE_ENUM_OPERATORS(Type)
    AMS_SOCKET_IMPL_DEFINE_ENUM_OPERATORS(AddrInfoFlag)

    #undef AMS_SOCKET_IMPL_DEFINE_ENUM_OPERATORS

}
