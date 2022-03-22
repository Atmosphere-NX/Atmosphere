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
#include <stratosphere/htcs/htcs_types.hpp>

namespace ams::htcs {

    const HtcsPeerName GetPeerNameAny();
    const HtcsPeerName GetDefaultHostName();

    s32 GetLastError();

    s32 Socket();
    s32 Close(s32 desc);
    s32 Connect(s32 desc, const SockAddrHtcs *address);
    s32 Bind(s32 desc, const SockAddrHtcs *address);
    s32 Listen(s32 desc, s32 backlog_count);
    s32 Accept(s32 desc, SockAddrHtcs *address);
    s32 Shutdown(s32 desc, s32 how);
    s32 Fcntl(s32 desc, s32 command, s32 value);

    s32 Select(s32 count, FdSet *read, FdSet *write, FdSet *exception, TimeVal *timeout);

    ssize_t Recv(s32 desc, void *buffer, size_t buffer_size, s32 flags);
    ssize_t Send(s32 desc, const void *buffer, size_t buffer_size, s32 flags);

    void FdSetZero(FdSet *set);
    void FdSetSet(s32 fd, FdSet *set);
    void FdSetClr(s32 fd, FdSet *set);
    bool FdSetIsSet(s32 fd, const FdSet *set);

}
