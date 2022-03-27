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
#include <stratosphere.hpp>
#include "socket_api.hpp"

#include <ws2tcpip.h>

#include <stratosphere/socket/impl/socket_platform_types_translation.hpp>

namespace ams::socket::impl {

    ///* TODO: Custom sys/* headers, probably. */
    #define AF_LINK 18

    #define	MSG_TRUNC	 0x10
    #define	MSG_CTRUNC	 0x20
    #define	MSG_DONTWAIT 0x80

    #define SHUT_RD   0
    #define SHUT_WR   1
    #define SHUT_RDWR 2

    #define O_NONBLOCK 4

    #define TCP_MAXSEG 4

    PosixWinSockConverter g_posix_winsock_converter;

    s32 PosixWinSockConverter::AcquirePosixHandle(SOCKET winsock, bool exempt) {
        /* Acquire exclusive access. */
        std::scoped_lock lk(m_mutex);

        /* Get initial index. */
        const auto initial_index = GetInitialIndex(winsock);

        /* Try to find an open index. */
        for (auto posix = initial_index; posix < static_cast<int>(MaxSocketsPerClient); ++posix) {
            if (m_data[posix].winsock == static_cast<SOCKET>(INVALID_SOCKET)) {
                m_data[posix].winsock = winsock;
                m_data[posix].exempt  = exempt;
                return posix;
            }
        }

        for (auto posix = 0; posix < initial_index; ++posix) {
            if (m_data[posix].winsock == static_cast<SOCKET>(INVALID_SOCKET)) {
                m_data[posix].winsock = winsock;
                m_data[posix].exempt  = exempt;
                return posix;
            }
        }

        /* We're out of open handles. */
        socket::impl::SetLastError(Errno::EMFile);
        return SOCKET_ERROR;
    }

    s32 PosixWinSockConverter::GetShutdown(bool &shutdown, s32 posix) {
        /* Acquire exclusive access. */
        std::scoped_lock lk(m_mutex);

        /* Check input. */
        if (static_cast<u32>(posix) >= MaxSocketsPerClient || m_data[posix].winsock == static_cast<SOCKET>(INVALID_SOCKET)) {
            socket::impl::SetLastError(Errno::EBadf);
            return SOCKET_ERROR;
        }

        /* Set the output. */
        shutdown = m_data[posix].shutdown;
        return 0;
    }

    s32 PosixWinSockConverter::GetSocketExempt(bool &exempt, s32 posix) {
        /* Acquire exclusive access. */
        std::scoped_lock lk(m_mutex);

        /* Check input. */
        if (static_cast<u32>(posix) >= MaxSocketsPerClient || m_data[posix].winsock == static_cast<SOCKET>(INVALID_SOCKET)) {
            socket::impl::SetLastError(Errno::EBadf);
            return SOCKET_ERROR;
        }

        /* Set the output. */
        exempt = m_data[posix].exempt;
        return 0;
    }

    SOCKET PosixWinSockConverter::PosixToWinsockSocket(s32 posix) {
        /* Acquire exclusive access. */
        std::scoped_lock lk(m_mutex);

        /* Check input. */
        if (static_cast<u32>(posix) >= MaxSocketsPerClient || m_data[posix].winsock == static_cast<SOCKET>(INVALID_SOCKET)) {
            socket::impl::SetLastError(Errno::EBadf);
            return SOCKET_ERROR;
        }

        return m_data[posix].winsock;
    }

    void PosixWinSockConverter::ReleaseAllPosixHandles() {
        /* Acquire exclusive access. */
        std::scoped_lock lk(m_mutex);

        for (size_t i = 0; i < MaxSocketsPerClient; ++i) {
            m_data[i] = SocketData{};
        }
    }

    void PosixWinSockConverter::ReleasePosixHandle(s32 posix) {
        /* Acquire exclusive access. */
        std::scoped_lock lk(m_mutex);

        AMS_ASSERT(static_cast<u32>(posix) < MaxSocketsPerClient);

        m_data[posix] = SocketData{};
    }

    s32 PosixWinSockConverter::SetShutdown(s32 posix, bool shutdown) {
        /* Acquire exclusive access. */
        std::scoped_lock lk(m_mutex);

        /* Check input. */
        if (static_cast<u32>(posix) >= MaxSocketsPerClient || m_data[posix].winsock == static_cast<SOCKET>(INVALID_SOCKET)) {
            socket::impl::SetLastError(Errno::EBadf);
            return SOCKET_ERROR;
        }

        /* Set the shutdown. */
        m_data[posix].shutdown = shutdown;
        return 0;
    }

    s32 PosixWinSockConverter::SetSocketExempt(s32 posix, bool exempt) {
        /* Acquire exclusive access. */
        std::scoped_lock lk(m_mutex);

        /* Check input. */
        if (static_cast<u32>(posix) >= MaxSocketsPerClient || m_data[posix].winsock == static_cast<SOCKET>(INVALID_SOCKET)) {
            socket::impl::SetLastError(Errno::EBadf);
            return SOCKET_ERROR;
        }

        /* Set the exempt. */
        m_data[posix].exempt = exempt;
        return 0;
    }

    s32 PosixWinSockConverter::WinsockToPosixSocket(SOCKET winsock) {
        /* Acquire exclusive access. */
        std::scoped_lock lk(m_mutex);

        /* Get initial index. */
        const auto initial_index = GetInitialIndex(winsock);

        /* Try to find an open index. */
        for (auto posix = initial_index; posix < static_cast<int>(MaxSocketsPerClient); ++posix) {
            if (m_data[posix].winsock == winsock) {
                return posix;
            }
        }

        for (auto posix = 0; posix < initial_index; ++posix) {
            if (m_data[posix].winsock == winsock) {
                return posix;
            }
        }

        /* We failed to find the posix handle. */
        return -1;
    }

    s32 MapProtocolValue(Protocol protocol) {
        s32 mapped = -1;

        switch (protocol) {
            case Protocol::IpProto_Ip:      mapped = IPPROTO_IP;   break;
            case Protocol::IpProto_Icmp:    mapped = IPPROTO_ICMP; break;
            case Protocol::IpProto_Tcp:     mapped = IPPROTO_TCP;  break;
            case Protocol::IpProto_Udp:     mapped = IPPROTO_UDP;  break;
            case Protocol::IpProto_None:    mapped = IPPROTO_NONE; break;
            case Protocol::IpProto_UdpLite: mapped = IPPROTO_UDP;  break;
            case Protocol::IpProto_Raw:     mapped = IPPROTO_RAW;  break;
            case Protocol::IpProto_Max:     mapped = IPPROTO_MAX;  break;
            default:
                AMS_SDK_LOG("WARNING: Invalid ams::Socket::Protocol %d\n", static_cast<int>(protocol));
                break;
        }

        if (mapped == -1) {
            AMS_SDK_LOG("WARNING: ams::Socket::Protocol %d is not supported by Win32/Win64.\n", static_cast<int>(protocol));
        }

        return mapped;
    }

    Protocol MapProtocolValue(s32 protocol) {
        Protocol mapped = Protocol::IpProto_None;

        switch (protocol) {
            case IPPROTO_IP:   mapped = Protocol::IpProto_Ip;   break;
            case IPPROTO_ICMP: mapped = Protocol::IpProto_Icmp; break;
            case IPPROTO_TCP:  mapped = Protocol::IpProto_Tcp;  break;
            case IPPROTO_UDP:  mapped = Protocol::IpProto_Udp;  break;
            case IPPROTO_NONE: mapped = Protocol::IpProto_None; break;
            case IPPROTO_RAW:  mapped = Protocol::IpProto_Raw;  break;
            case IPPROTO_MAX:  mapped = Protocol::IpProto_Max;  break;
            default:
                AMS_SDK_LOG("WARNING: Invalid socket protocol %d\n", static_cast<int>(protocol));
                break;
        }

        return mapped;
    }

    s32 MapTypeValue(Type type) {
        s32 mapped = -1;

        switch (type) {
            case Type::Sock_Default:   mapped = 0;              break;
            case Type::Sock_Stream:    mapped = SOCK_STREAM;    break;
            case Type::Sock_Dgram:     mapped = SOCK_DGRAM;     break;
            case Type::Sock_Raw:       mapped = SOCK_RAW;       break;
            case Type::Sock_SeqPacket: mapped = SOCK_SEQPACKET; break;
            case Type::Sock_NonBlock:  mapped = -1;             break;
            default:
                AMS_SDK_LOG("WARNING: Invalid ams::Socket::Type %d\n", static_cast<int>(type));
                break;
        }

        if (mapped == -1) {
            AMS_SDK_LOG("WARNING: ams::Socket::Type %d is not supported by Win32/Win64.\n", static_cast<int>(type));
        }

        return mapped;
    }

    Type MapTypeValue(s32 type) {
        Type mapped = Type::Sock_Default;

        switch (type) {
            case 0:              mapped = Type::Sock_Default;   break;
            case SOCK_STREAM:    mapped = Type::Sock_Stream;    break;
            case SOCK_DGRAM:     mapped = Type::Sock_Dgram;     break;
            case SOCK_RAW:       mapped = Type::Sock_Raw;       break;
            case SOCK_SEQPACKET: mapped = Type::Sock_SeqPacket; break;
            default:
                AMS_SDK_LOG("WARNING: Invalid socket type %d\n", static_cast<int>(type));
                break;
        }

        return mapped;
    }

    s8 MapFamilyValue(Family family) {
        s8 mapped = -1;

        switch (family) {
            case Family::Af_Unspec: mapped = AF_UNSPEC; break;
            case Family::Af_Inet:   mapped = AF_INET;   break;
            case Family::Af_Route:  mapped = -1;        break;
            case Family::Af_Link:   mapped = AF_LINK;   break;
            case Family::Af_Inet6:  mapped = AF_INET6;  break;
            case Family::Af_Max:    mapped = AF_MAX;    break;
            default:
                AMS_SDK_LOG("WARNING: Invalid ams::Socket::Family %d\n", static_cast<int>(family));
                break;
        }

        if (mapped == -1) {
            AMS_SDK_LOG("WARNING: ams::Socket::Family %d is not supported by Win32/Win64.\n", static_cast<int>(family));
        }

        return mapped;
    }

    Family MapFamilyValue(s8 family) {
        Family mapped = Family::Af_Unspec;

        switch (family) {
            case AF_UNSPEC:mapped = Family::Af_Unspec; break;
            case AF_INET:  mapped = Family::Af_Inet;   break;
            case AF_LINK:  mapped = Family::Af_Link;   break;
            case AF_INET6: mapped = Family::Af_Inet6;  break;
            case AF_MAX:   mapped = Family::Af_Max;    break;
            default:
                AMS_SDK_LOG("WARNING: Invalid socket family %d\n", static_cast<int>(family));
                break;
        }

        return mapped;
    }

    s32 MapMsgFlagValue(MsgFlag flag) {
        s32 mapped = 0;

        if ((flag & MsgFlag::Msg_Oob) != MsgFlag::Msg_None) { mapped |= MSG_OOB; }
        if ((flag & MsgFlag::Msg_Peek) != MsgFlag::Msg_None) { mapped |= MSG_PEEK; }
        if ((flag & MsgFlag::Msg_DontRoute) != MsgFlag::Msg_None) { mapped |= MSG_DONTROUTE; }
        if ((flag & MsgFlag::Msg_Trunc) != MsgFlag::Msg_None) { mapped |= MSG_TRUNC; }
        if ((flag & MsgFlag::Msg_CTrunc) != MsgFlag::Msg_None) { mapped |= MSG_CTRUNC; }
        if ((flag & MsgFlag::Msg_WaitAll) != MsgFlag::Msg_None) { mapped |= MSG_WAITALL; }
        if ((flag & MsgFlag::Msg_DontWait) != MsgFlag::Msg_None) { mapped |= MSG_DONTWAIT; }

        return mapped;
    }

    MsgFlag MapMsgFlagValue(s32 flag) {
        MsgFlag mapped = MsgFlag::Msg_None;

        if (flag & MSG_OOB) { mapped |= MsgFlag::Msg_Oob; }
        if (flag & MSG_PEEK) { mapped |= MsgFlag::Msg_Peek; }
        if (flag & MSG_DONTROUTE) { mapped |= MsgFlag::Msg_DontRoute; }
        if (flag & MSG_TRUNC) { mapped |= MsgFlag::Msg_Trunc; }
        if (flag & MSG_CTRUNC) { mapped |= MsgFlag::Msg_CTrunc; }
        if (flag & MSG_WAITALL) { mapped |= MsgFlag::Msg_WaitAll; }
        if (flag & MSG_DONTWAIT) { mapped |= MsgFlag::Msg_DontWait; }

        return mapped;
    }

    u32 MapAddrInfoFlagValue(AddrInfoFlag flag) {
        u32 mapped = 0;

        if ((flag & AddrInfoFlag::Ai_Passive) != AddrInfoFlag::Ai_None) { mapped |= AI_PASSIVE; }
        if ((flag & AddrInfoFlag::Ai_CanonName) != AddrInfoFlag::Ai_None) { mapped |= AI_CANONNAME; }
        if ((flag & AddrInfoFlag::Ai_NumericHost) != AddrInfoFlag::Ai_None) { mapped |= AI_NUMERICHOST; }
        if ((flag & AddrInfoFlag::Ai_NumericServ) != AddrInfoFlag::Ai_None) { mapped |= AI_NUMERICSERV; }
        if ((flag & AddrInfoFlag::Ai_AddrConfig) != AddrInfoFlag::Ai_None) { mapped |= AI_ADDRCONFIG; }

        return mapped;
    }

    AddrInfoFlag MapAddrInfoFlagValue(u32 flag) {
        AddrInfoFlag mapped = AddrInfoFlag::Ai_None;

        if (flag & AI_PASSIVE) { mapped |= AddrInfoFlag::Ai_Passive; }
        if (flag & AI_CANONNAME) { mapped |= AddrInfoFlag::Ai_CanonName; }
        if (flag & AI_NUMERICHOST) { mapped |= AddrInfoFlag::Ai_NumericHost; }
        if (flag & AI_NUMERICSERV) { mapped |= AddrInfoFlag::Ai_NumericServ; }
        if (flag & AI_ADDRCONFIG) { mapped |= AddrInfoFlag::Ai_AddrConfig; }

        return mapped;
    }

    u32 MapShutdownMethodValue(ShutdownMethod how) {
        u32 mapped = -1;

        switch (how) {
            case ShutdownMethod::Shut_Rd:   mapped = SHUT_RD;   break;
            case ShutdownMethod::Shut_Wr:   mapped = SHUT_WR;   break;
            case ShutdownMethod::Shut_RdWr: mapped = SHUT_RDWR; break;
            default:
                AMS_SDK_LOG("WARNING: Invalid ams::Socket::ShutdownMethod %d\n", static_cast<int>(how));
                break;
        }

        return mapped;
    }

    ShutdownMethod MapShutdownMethodValue(u32 how) {
        ShutdownMethod mapped = static_cast<ShutdownMethod>(-1);

        switch (how) {
            case SHUT_RD:   mapped = ShutdownMethod::Shut_Rd;   break;
            case SHUT_WR:   mapped = ShutdownMethod::Shut_Wr;   break;
            case SHUT_RDWR: mapped = ShutdownMethod::Shut_RdWr; break;
            default:
                AMS_SDK_LOG("WARNING: Invalid socket shutdown %d\n", static_cast<int>(how));
                break;
        }

        return mapped;
    }

    u32 MapFcntlFlagValue(FcntlFlag flag) {
        u32 mapped = 0;

        switch (flag) {
            case FcntlFlag::O_NonBlock: mapped = O_NONBLOCK; break;
            default:
                AMS_SDK_LOG("WARNING: Invalid ams::Socket::FcntlFlag %d\n", static_cast<int>(flag));
                break;
        }

        return mapped;
    }

    FcntlFlag MapFcntlFlagValue(u32 flag) {
        FcntlFlag mapped = FcntlFlag::None;

        switch (flag) {
            case O_NONBLOCK: mapped = FcntlFlag::O_NonBlock; break;
            default:
                AMS_SDK_LOG("WARNING: Invalid socket fcntl flag %d\n", static_cast<int>(flag));
                break;
        }

        return mapped;
    }

    s32 MapLevelValue(Level level) {
        s32 mapped = -1;

        switch (level) {
            case Level::Sol_Socket:  mapped = SOL_SOCKET;   break;
            case Level::Sol_Ip:      mapped = IPPROTO_IP;   break;
            case Level::Sol_Icmp:    mapped = IPPROTO_ICMP; break;
            case Level::Sol_Tcp:     mapped = IPPROTO_TCP;  break;
            case Level::Sol_Udp:     mapped = IPPROTO_UDP;  break;
            case Level::Sol_UdpLite: mapped = IPPROTO_UDP;  break;
            default:
                AMS_SDK_LOG("WARNING: Invalid ams::Socket::Level %d\n", static_cast<int>(level));
                break;
        }

        return mapped;
    }

    Level MapLevelValue(s32 level) {
        Level mapped = static_cast<Level>(0);

        switch (level) {
            case SOL_SOCKET:   mapped = Level::Sol_Socket; break;
            case IPPROTO_IP:   mapped = Level::Sol_Ip;     break;
            case IPPROTO_ICMP: mapped = Level::Sol_Icmp;   break;
            case IPPROTO_TCP:  mapped = Level::Sol_Tcp;    break;
            case IPPROTO_UDP:  mapped = Level::Sol_Udp;    break;
            default:
                AMS_SDK_LOG("WARNING: Invalid socket level %d\n", static_cast<int>(level));
                break;
        }

        return mapped;
    }

    s32 MapOptionValue(Level level, Option option) {
        s32 mapped = -1;

        switch (level) {
            case Level::Sol_Socket:
                switch (option) {
                    case Option::So_Debug:              mapped = SO_DEBUG;       break;
                    case Option::So_AcceptConn:         mapped = SO_ACCEPTCONN;  break;
                    case Option::So_ReuseAddr:          mapped = SO_REUSEADDR;   break;
                    case Option::So_KeepAlive:          mapped = SO_KEEPALIVE;   break;
                    case Option::So_DontRoute:          mapped = SO_DONTROUTE;   break;
                    case Option::So_Broadcast:          mapped = SO_BROADCAST;   break;
                    case Option::So_UseLoopback:        mapped = SO_USELOOPBACK; break;
                    case Option::So_Linger:             mapped = SO_LINGER;      break;
                    case Option::So_OobInline:          mapped = SO_OOBINLINE;   break;
                    case Option::So_ReusePort:          mapped = -1;             break;
                    case Option::So_SndBuf:             mapped = SO_SNDBUF;      break;
                    case Option::So_RcvBuf:             mapped = SO_RCVBUF;      break;
                    case Option::So_SndLoWat:           mapped = SO_SNDLOWAT;    break;
                    case Option::So_RcvLoWat:           mapped = SO_RCVLOWAT;    break;
                    case Option::So_SndTimeo:           mapped = SO_SNDTIMEO;    break;
                    case Option::So_RcvTimeo:           mapped = SO_RCVTIMEO;    break;
                    case Option::So_Error:              mapped = SO_ERROR;       break;
                    case Option::So_Type:               mapped = SO_TYPE;        break;
                    case Option::So_Label:              mapped = -1;             break;
                    case Option::So_PeerLabel:          mapped = -1;             break;
                    case Option::So_ListenQLimit:       mapped = -1;             break;
                    case Option::So_ListenQLen:         mapped = -1;             break;
                    case Option::So_ListenIncQLen:      mapped = -1;             break;
                    case Option::So_SetFib:             mapped = -1;             break;
                    case Option::So_User_Cookie:        mapped = -1;             break;
                    case Option::So_Protocol:           mapped = -1;             break;
                    case Option::So_Vendor:             mapped = -1;             break;
                    case Option::So_Nn_Linger:          mapped = -1;             break;
                    case Option::So_Nn_Shutdown_Exempt: mapped = -1;             break;
                    default:
                        AMS_SDK_LOG("WARNING: Invalid ams::Socket::Option %d for Level::Sol_Socket\n", static_cast<int>(option));
                        break;
                }
                break;
            case Level::Sol_Ip:
                switch (option) {
                    case Option::Ip_Options:                mapped = IP_OPTIONS;                break;
                    case Option::Ip_HdrIncl:                mapped = IP_HDRINCL;                break;
                    case Option::Ip_Tos:                    mapped = IP_TOS;                    break;
                    case Option::Ip_Ttl:                    mapped = IP_TTL;                    break;
                    case Option::Ip_RecvOpts:               mapped = -1;                        break;
                    case Option::Ip_Multicast_If:           mapped = IP_MULTICAST_IF;           break;
                    case Option::Ip_Multicast_Ttl:          mapped = IP_MULTICAST_TTL;          break;
                    case Option::Ip_Multicast_Loop:         mapped = IP_MULTICAST_LOOP;         break;
                    case Option::Ip_Add_Membership:         mapped = IP_ADD_MEMBERSHIP;         break;
                    case Option::Ip_Drop_Membership:        mapped = IP_DROP_MEMBERSHIP;        break;
                    case Option::Ip_Multicast_Vif:          mapped = -1;                        break;
                    case Option::Ip_Rsvp_On:                mapped = -1;                        break;
                    case Option::Ip_Rsvp_Off:               mapped = -1;                        break;
                    case Option::Ip_Rsvp_Vif_On:            mapped = -1;                        break;
                    case Option::Ip_Rsvp_Vif_Off:           mapped = -1;                        break;
                    case Option::Ip_PortRange:              mapped = -1;                        break;
                    case Option::Ip_Faith:                  mapped = -1;                        break;
                    case Option::Ip_OnesBcast:              mapped = -1;                        break;
                    case Option::Ip_BindAny:                mapped = -1;                        break;
                    case Option::Ip_RecvTtl:                mapped = -1;                        break;
                    case Option::Ip_MinTtl:                 mapped = -1;                        break;
                    case Option::Ip_DontFrag:               mapped = -1;                        break;
                    case Option::Ip_RecvTos:                mapped = -1;                        break;
                    case Option::Ip_Add_Source_Membership:  mapped = IP_ADD_SOURCE_MEMBERSHIP;  break;
                    case Option::Ip_Drop_Source_Membership: mapped = IP_DROP_SOURCE_MEMBERSHIP; break;
                    case Option::Ip_Block_Source:           mapped = IP_BLOCK_SOURCE;           break;
                    case Option::Ip_Unblock_Source:         mapped = IP_UNBLOCK_SOURCE;         break;
                    default:
                        AMS_SDK_LOG("WARNING: Invalid ams::Socket::Option %d for Level::Sol_Ip\n", static_cast<int>(option));
                        break;
                }
                break;
            case Level::Sol_Tcp:
                switch (option) {
                    case Option::Tcp_NoDelay:    mapped = TCP_NODELAY; break;
                    case Option::Tcp_MaxSeg:     mapped = TCP_MAXSEG;  break;
                    case Option::Tcp_NoPush:     mapped = -1;          break;
                    case Option::Tcp_NoOpt:      mapped = -1;          break;
                    case Option::Tcp_Md5Sig:     mapped = -1;          break;
                    case Option::Tcp_Info:       mapped = -1;          break;
                    case Option::Tcp_Congestion: mapped = -1;          break;
                    case Option::Tcp_KeepInit:   mapped = -1;          break;
                    case Option::Tcp_KeepIdle:   mapped = -1;          break;
                    case Option::Tcp_KeepIntvl:  mapped = -1;          break;
                    case Option::Tcp_KeepCnt:    mapped = -1;          break;
                    case Option::Tcp_Vendor:     mapped = -1;          break;
                    default:
                        AMS_SDK_LOG("WARNING: Invalid ams::Socket::Option %d for Level::Sol_Tcp\n", static_cast<int>(option));
                        break;
                }
                break;
            default:
                AMS_SDK_LOG("WARNING: Invalid option level %d\n", static_cast<int>(level));
                break;
        }

        if (mapped == -1) {
            AMS_SDK_LOG("WARNING: ams::Socket::Option %d is not supported by Win32/Win64.\n", static_cast<int>(option));
        }

        return mapped;
    }

    Option MapOptionValue(s32 level, s32 option) {
        Option mapped = static_cast<Option>(0);

        switch (level) {
            case SOL_SOCKET:
                switch (option) {
                    case SO_DEBUG:       mapped = Option::So_Debug;       break;
                    case SO_ACCEPTCONN:  mapped = Option::So_AcceptConn;  break;
                    case SO_REUSEADDR:   mapped = Option::So_ReuseAddr;   break;
                    case SO_KEEPALIVE:   mapped = Option::So_KeepAlive;   break;
                    case SO_DONTROUTE:   mapped = Option::So_DontRoute;   break;
                    case SO_BROADCAST:   mapped = Option::So_Broadcast;   break;
                    case SO_USELOOPBACK: mapped = Option::So_UseLoopback; break;
                    case SO_LINGER:      mapped = Option::So_Linger;      break;
                    case SO_OOBINLINE:   mapped = Option::So_OobInline;   break;
                    case SO_SNDBUF:      mapped = Option::So_SndBuf;      break;
                    case SO_RCVBUF:      mapped = Option::So_RcvBuf;      break;
                    case SO_SNDLOWAT:    mapped = Option::So_SndLoWat;    break;
                    case SO_RCVLOWAT:    mapped = Option::So_RcvLoWat;    break;
                    case SO_SNDTIMEO:    mapped = Option::So_SndTimeo;    break;
                    case SO_RCVTIMEO:    mapped = Option::So_RcvTimeo;    break;
                    case SO_ERROR:       mapped = Option::So_Error;       break;
                    case SO_TYPE:        mapped = Option::So_Type;        break;
                    default:
                        AMS_SDK_LOG("WARNING: Invalid socket option %d for SOL_SOCKET\n", static_cast<int>(option));
                        break;
                }
                break;
            case IPPROTO_IP:
                switch (option) {
                    case IP_OPTIONS:                mapped = Option::Ip_Options;                break;
                    case IP_HDRINCL:                mapped = Option::Ip_HdrIncl;                break;
                    case IP_TOS:                    mapped = Option::Ip_Tos;                    break;
                    case IP_TTL:                    mapped = Option::Ip_Ttl;                    break;
                    case IP_MULTICAST_IF:           mapped = Option::Ip_Multicast_If;           break;
                    case IP_MULTICAST_TTL:          mapped = Option::Ip_Multicast_Ttl;          break;
                    case IP_MULTICAST_LOOP:         mapped = Option::Ip_Multicast_Loop;         break;
                    case IP_ADD_MEMBERSHIP:         mapped = Option::Ip_Add_Membership;         break;
                    case IP_DROP_MEMBERSHIP:        mapped = Option::Ip_Drop_Membership;        break;
                    case IP_ADD_SOURCE_MEMBERSHIP:  mapped = Option::Ip_Add_Source_Membership;  break;
                    case IP_DROP_SOURCE_MEMBERSHIP: mapped = Option::Ip_Drop_Source_Membership; break;
                    case IP_BLOCK_SOURCE:           mapped = Option::Ip_Block_Source;           break;
                    case IP_UNBLOCK_SOURCE:         mapped = Option::Ip_Unblock_Source;         break;
                    default:
                        AMS_SDK_LOG("WARNING: Invalid socket option %d for SOL_IP\n", static_cast<int>(option));
                        break;
                }
                break;
            case IPPROTO_TCP:
                switch (option) {
                    case TCP_NODELAY: mapped = Option::Tcp_NoDelay; break;
                    case TCP_MAXSEG:  mapped = Option::Tcp_MaxSeg;  break;
                    default:
                        AMS_SDK_LOG("WARNING: Invalid ams::Socket::Option %d for SOL_TCP\n", static_cast<int>(option));
                        break;
                }
                break;
            default:
                AMS_SDK_LOG("WARNING: Invalid option level %d\n", static_cast<int>(level));
                break;
        }

        return mapped;
    }

    s32 MapErrnoValue(Errno error) {
        s32 mapped = -1;

        switch (error) {
            case Errno::EIntr:           mapped = WSAEINTR;                break;
            case Errno::EBadf:           mapped = WSAEBADF;                break;
            case Errno::EWouldBlock:     mapped = WSAEWOULDBLOCK;          break;
            case Errno::EAcces:          mapped = WSAEACCES;               break;
            case Errno::EFault:          mapped = WSAEFAULT;               break;
            case Errno::EInval:          mapped = WSAEINVAL;               break;
            case Errno::EMFile:          mapped = WSAEMFILE;               break;
            case Errno::EPipe:           mapped = 109;                     break;
            case Errno::ENameTooLong:    mapped = WSAENAMETOOLONG;         break;
            case Errno::ENotEmpty:       mapped = WSAENOTEMPTY;            break;
            case Errno::ELoop:           mapped = WSAELOOP;                break;
            case Errno::ERemote:         mapped = WSAEREMOTE;              break;
            case Errno::EUsers:          mapped = WSAEUSERS;               break;
            case Errno::ENotSock:        mapped = WSAENOTSOCK;             break;
            case Errno::EDestAddrReq:    mapped = WSAEDESTADDRREQ;         break;
            case Errno::EMsgSize:        mapped = WSAEMSGSIZE;             break;
            case Errno::EPrototype:      mapped = WSAEPROTOTYPE;           break;
            case Errno::ENoProtoOpt:     mapped = WSAENOPROTOOPT;          break;
            case Errno::EProtoNoSupport: mapped = WSAEPROTONOSUPPORT;      break;
            case Errno::ESocktNoSupport: mapped = WSAESOCKTNOSUPPORT;      break;
            case Errno::EOpNotSupp:      mapped = WSAEOPNOTSUPP;           break;
            case Errno::EPfNoSupport:    mapped = WSAEPFNOSUPPORT;         break;
            case Errno::EAfNoSupport:    mapped = WSAEAFNOSUPPORT;         break;
            case Errno::EAddrInUse:      mapped = WSAEADDRINUSE;           break;
            case Errno::EAddrNotAvail:   mapped = WSAEADDRNOTAVAIL;        break;
            case Errno::ENetDown:        mapped = WSAENETDOWN;             break;
            case Errno::ENetUnreach:     mapped = WSAENETUNREACH;          break;
            case Errno::ENetReset:       mapped = WSAENETRESET;            break;
            case Errno::EConnAborted:    mapped = WSAECONNABORTED;         break;
            case Errno::EConnReset:      mapped = WSAECONNRESET;           break;
            case Errno::ENoBufs:         mapped = WSAENOBUFS;              break;
            case Errno::EIsConn:         mapped = WSAEISCONN;              break;
            case Errno::ENotConn:        mapped = WSAENOTCONN;             break;
            case Errno::EShutDown:       mapped = WSAESHUTDOWN;            break;
            case Errno::ETooManyRefs:    mapped = WSAETOOMANYREFS;         break;
            case Errno::ETimedOut:       mapped = WSAETIMEDOUT;            break;
            case Errno::EConnRefused:    mapped = WSAECONNREFUSED;         break;
            case Errno::EHostDown:       mapped = WSAEHOSTDOWN;            break;
            case Errno::EHostUnreach:    mapped = WSAEHOSTUNREACH;         break;
            case Errno::EAlready:        mapped = WSAEALREADY;             break;
            case Errno::EInProgress:     mapped = WSAEINPROGRESS;          break;
            case Errno::EStale:          mapped = WSAESTALE;               break;
            case Errno::EDQuot:          mapped = WSAEDQUOT;               break;
            case Errno::ECanceled:       mapped = WSAECANCELLED;           break;
            case Errno::EProcLim:        mapped = WSAEPROCLIM;             break;
            default:                     mapped = static_cast<s32>(error); break;
        }

        return mapped;
    }

    Errno MapErrnoValue(s32 error) {
        Errno mapped = Errno::ESuccess;

        switch (error) {
            case WSAEBADF:           mapped = Errno::EBadf;               break;
            case WSAEACCES:          mapped = Errno::EAcces;              break;
            case WSAEFAULT:          mapped = Errno::EFault;              break;
            case WSAEINVAL:          mapped = Errno::EInval;              break;
            case WSAEMFILE:          mapped = Errno::EMFile;              break;
            case WSAEWOULDBLOCK:     mapped = Errno::EWouldBlock;         break;
            case WSAEINPROGRESS:     mapped = Errno::EInProgress;         break;
            case WSAEALREADY:        mapped = Errno::EAlready;            break;
            case WSAENOTSOCK:        mapped = Errno::ENotSock;            break;
            case WSAEDESTADDRREQ:    mapped = Errno::EDestAddrReq;        break;
            case WSAEMSGSIZE:        mapped = Errno::EMsgSize;            break;
            case WSAEPROTOTYPE:      mapped = Errno::EPrototype;          break;
            case WSAENOPROTOOPT:     mapped = Errno::ENoProtoOpt;         break;
            case WSAEPROTONOSUPPORT: mapped = Errno::EProtoNoSupport;     break;
            case WSAESOCKTNOSUPPORT: mapped = Errno::ESocktNoSupport;     break;
            case WSAEOPNOTSUPP:      mapped = Errno::EOpNotSupp;          break;
            case WSAEPFNOSUPPORT:    mapped = Errno::EPfNoSupport;        break;
            case WSAEAFNOSUPPORT:    mapped = Errno::EAfNoSupport;        break;
            case WSAEADDRINUSE:      mapped = Errno::EAddrInUse;          break;
            case WSAEADDRNOTAVAIL:   mapped = Errno::EAddrNotAvail;       break;
            case WSAENETDOWN:        mapped = Errno::ENetDown;            break;
            case WSAENETUNREACH:     mapped = Errno::ENetUnreach;         break;
            case WSAENETRESET:       mapped = Errno::ENetReset;           break;
            case WSAECONNABORTED:    mapped = Errno::EConnAborted;        break;
            case WSAECONNRESET:      mapped = Errno::EConnReset;          break;
            case WSAENOBUFS:         mapped = Errno::ENoBufs;             break;
            case WSAEISCONN:         mapped = Errno::EIsConn;             break;
            case WSAENOTCONN:        mapped = Errno::ENotConn;            break;
            case WSAESHUTDOWN:       mapped = Errno::EShutDown;           break;
            case WSAETOOMANYREFS:    mapped = Errno::ETooManyRefs;        break;
            case WSAETIMEDOUT:       mapped = Errno::ETimedOut;           break;
            case WSAECONNREFUSED:    mapped = Errno::EConnRefused;        break;
            case WSAELOOP:           mapped = Errno::ELoop;               break;
            case WSAENAMETOOLONG:    mapped = Errno::ENameTooLong;        break;
            case WSAEHOSTDOWN:       mapped = Errno::EHostDown;           break;
            case WSAEHOSTUNREACH:    mapped = Errno::EHostUnreach;        break;
            case WSAENOTEMPTY:       mapped = Errno::ENotEmpty;           break;
            case WSAEPROCLIM:        mapped = Errno::EProcLim;            break;
            case WSAEUSERS:          mapped = Errno::EUsers;              break;
            case WSAEDQUOT:          mapped = Errno::EDQuot;              break;
            case WSAESTALE:          mapped = Errno::EStale;              break;
            case WSAEREMOTE:         mapped = Errno::ERemote;             break;
            case WSAECANCELLED:      mapped = Errno::ECanceled;           break;
            case WSAEINTR:           mapped = Errno::EIntr;               break;
            case 109:                mapped = Errno::EPipe;               break;
            default:                 mapped = static_cast<Errno>(error); break;
        }

        return mapped;
    }

    void CopyToPlatform(sockaddr_in *dst, const SockAddrIn *src) {
        if (src != nullptr) {
            dst->sin_family = MapFamilyValue(src->sin_family);
            dst->sin_port   = src->sin_port;
            std::memcpy(std::addressof(dst->sin_addr), std::addressof(src->sin_addr), sizeof(dst->sin_addr));
            std::memcpy(dst->sin_zero, src->sin_zero, sizeof(dst->sin_zero));
        }
    }

    void CopyFromPlatform(SockAddrIn *dst, const sockaddr_in *src) {
        if (dst != nullptr) {
            dst->sin_family = MapFamilyValue(static_cast<s8>(src->sin_family));
            dst->sin_port   = src->sin_port;
            std::memcpy(std::addressof(dst->sin_addr), std::addressof(src->sin_addr), sizeof(dst->sin_addr));
            std::memcpy(dst->sin_zero, src->sin_zero, sizeof(dst->sin_zero));
        }
    }

    void CopyToPlatform(timeval *dst, const TimeVal *src) {
        if (src != nullptr) {
            dst->tv_sec  = src->tv_sec;
            dst->tv_usec = src->tv_usec;
        }
    }

    void CopyFromPlatform(TimeVal *dst, const timeval *src) {
        if (dst != nullptr) {
            dst->tv_sec  = src->tv_sec;
            dst->tv_usec = src->tv_usec;
        }
    }

    void CopyToPlatform(linger *dst, const Linger *src) {
        if (src != nullptr) {
            dst->l_onoff  = static_cast<u_short>(src->l_onoff);
            dst->l_linger = static_cast<u_short>(src->l_linger);
        }
    }

    void CopyFromPlatform(Linger *dst, const linger *src) {
        if (dst != nullptr) {
            dst->l_onoff  = src->l_onoff;
            dst->l_linger = src->l_linger;
        }
    }

}
