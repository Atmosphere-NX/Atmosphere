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

namespace ams::htcs {

    using ssize_t           = intptr_t;
    using AddressFamilyType = u16;

    constexpr inline int PeerNameBufferLength = 32;
    constexpr inline int PortNameBufferLength = 32;

    constexpr inline int SessionCountMax = 0x10;

    constexpr inline int SocketCountMax = 40;
    constexpr inline int FdSetSize      = SocketCountMax;

    struct HtcsPeerName {
        char name[PeerNameBufferLength];
    };

    struct HtcsPortName {
        char name[PortNameBufferLength];
    };

    struct SockAddrHtcs {
        AddressFamilyType family;
        HtcsPeerName      peer_name;
        HtcsPortName      port_name;
    };

    struct TimeVal {
        s64 tv_sec;
        s64 tv_usec;
    };

    struct FdSet {
        int fds[FdSetSize];
    };

    enum SocketError {
        HTCS_ENONE         =  0,
        HTCS_EACCES        =  2,
        HTCS_EADDRINUSE    =  3,
        HTCS_EADDRNOTAVAIL =  4,
        HTCS_EAGAIN        =  6,
        HTCS_EALREADY      =  7,
        HTCS_EBADF         =  8,
        HTCS_EBUSY         = 10,
        HTCS_ECONNABORTED  = 13,
        HTCS_ECONNREFUSED  = 14,
        HTCS_ECONNRESET    = 15,
        HTCS_EDESTADDRREQ  = 17,
        HTCS_EFAULT        = 21,
        HTCS_EINPROGRESS   = 26,
        HTCS_EINTR         = 27,
        HTCS_EINVAL        = 28,
        HTCS_EIO           = 29,
        HTCS_EISCONN       = 30,
        HTCS_EMFILE        = 33,
        HTCS_EMSGSIZE      = 35,
        HTCS_ENETDOWN      = 38,
        HTCS_ENETRESET     = 39,
        HTCS_ENOBUFS       = 42,
        HTCS_ENOMEM        = 49,
        HTCS_ENOTCONN      = 56,
        HTCS_ETIMEDOUT     = 76,
        HTCS_EUNKNOWN      = 79,

        HTCS_EWOULDBLOCK   = HTCS_EAGAIN,
    };

    enum MessageFlag {
        HTCS_MSG_PEEK    = 1,
        HTCS_MSG_WAITALL = 2,
    };

    enum ShutdownType {
        HTCS_SHUT_RD   = 0,
        HTCS_SHUT_WR   = 1,
        HTCS_SHUT_RDWR = 2,
    };

    enum FcntlOperation {
        HTCS_F_GETFL = 3,
        HTCS_F_SETFL = 4,
    };

    enum FcntlFlag {
        HTCS_O_NONBLOCK = 4,
    };

    enum AddressFamily {
        HTCS_AF_HTCS = 0,
    };

}
