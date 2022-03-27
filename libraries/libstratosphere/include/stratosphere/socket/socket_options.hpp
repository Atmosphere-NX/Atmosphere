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

    enum class Level : s32 {
        Sol_Ip      =   0,
        Sol_Icmp    =   1,
        Sol_Tcp     =   6,
        Sol_Udp     =  17,
        Sol_UdpLite = 136,

        Sol_Socket  = 0xFFFF,
    };

    enum class Option : u32 {
        /* ==================================== */
        So_Debug              = (1 <<  0),
        So_AcceptConn         = (1 <<  1),
        So_ReuseAddr          = (1 <<  2),
        So_KeepAlive          = (1 <<  3),
        So_DontRoute          = (1 <<  4),
        So_Broadcast          = (1 <<  5),
        So_UseLoopback        = (1 <<  6),
        So_Linger             = (1 <<  7),
        So_OobInline          = (1 <<  8),
        So_ReusePort          = (1 <<  9),

        So_SndBuf             = (1 << 12) | 0x01,
        So_RcvBuf             = (1 << 12) | 0x02,
        So_SndLoWat           = (1 << 12) | 0x03,
        So_RcvLoWat           = (1 << 12) | 0x04,
        So_SndTimeo           = (1 << 12) | 0x05,
        So_RcvTimeo           = (1 << 12) | 0x06,
        So_Error              = (1 << 12) | 0x07,
        So_Type               = (1 << 12) | 0x08,
        So_Label              = (1 << 12) | 0x09,
        So_PeerLabel          = (1 << 12) | 0x10,
        So_ListenQLimit       = (1 << 12) | 0x11,
        So_ListenQLen         = (1 << 12) | 0x12,
        So_ListenIncQLen      = (1 << 12) | 0x13,
        So_SetFib             = (1 << 12) | 0x14,
        So_User_Cookie        = (1 << 12) | 0x15,
        So_Protocol           = (1 << 12) | 0x16,

        So_Nn_Shutdown_Exempt = (1 << 16),

        So_Vendor             = (1u << 31),
        So_Nn_Linger          = So_Vendor | 0x01,
        /* ==================================== */

        /* ==================================== */
        Ip_Options                =  1,
        Ip_HdrIncl                =  2,
        Ip_Tos                    =  3,
        Ip_Ttl                    =  4,
        Ip_RecvOpts               =  5,
        Ip_Multicast_If           =  9,
        Ip_Multicast_Ttl          = 10,
        Ip_Multicast_Loop         = 11,
        Ip_Add_Membership         = 12,
        Ip_Drop_Membership        = 13,
        Ip_Multicast_Vif          = 14,
        Ip_Rsvp_On                = 15,
        Ip_Rsvp_Off               = 16,
        Ip_Rsvp_Vif_On            = 17,
        Ip_Rsvp_Vif_Off           = 18,
        Ip_PortRange              = 19,
        Ip_Faith                  = 22,
        Ip_OnesBcast              = 23,
        Ip_BindAny                = 24,

        Ip_RecvTtl                = 65,
        Ip_MinTtl                 = 66,
        Ip_DontFrag               = 67,
        Ip_RecvTos                = 68,

        Ip_Add_Source_Membership  = 70,
        Ip_Drop_Source_Membership = 71,
        Ip_Block_Source           = 72,
        Ip_Unblock_Source         = 73,
        /* ==================================== */

        /* ==================================== */
        Tcp_NoDelay    = (1 <<  0),
        Tcp_MaxSeg     = (1 <<  1),
        Tcp_NoPush     = (1 <<  2),
        Tcp_NoOpt      = (1 <<  3),
        Tcp_Md5Sig     = (1 <<  4),
        Tcp_Info       = (1 <<  5),
        Tcp_Congestion = (1 <<  6),
        Tcp_KeepInit   = (1 <<  7),
        Tcp_KeepIdle   = (1 <<  8),
        Tcp_KeepIntvl  = (1 <<  9),
        Tcp_KeepCnt    = (1 << 10),

        Tcp_Vendor     = So_Vendor,
        /* ==================================== */
    };

}
