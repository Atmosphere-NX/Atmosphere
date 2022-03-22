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

namespace ams::socket::impl {

    Errno GetLastError();
    void SetLastError(Errno err);

    bool HeapIsAvailable(int generation);
    int GetHeapGeneration();

    u32 InetHtonl(u32 host);
    u16 InetHtons(u16 host);
    u32 InetNtohl(u32 net);
    u16 InetNtohs(u16 net);

    Result Initialize(const Config &config);
    Result Finalize();

    Result InitializeAllocatorForInternal(void *buffer, size_t size);

    ssize_t RecvFrom(s32 desc, void *buffer, size_t buffer_size, MsgFlag flags, SockAddr *out_address, SockLenT *out_addr_len);
    ssize_t Recv(s32 desc, void *buffer, size_t buffer_size, MsgFlag flags);

    ssize_t SendTo(s32 desc, const void *buffer, size_t buffer_size, MsgFlag flags, const SockAddr *address, SockLenT len);
    ssize_t Send(s32 desc, const void *buffer, size_t buffer_size, MsgFlag flags);

    s32 Shutdown(s32 desc, ShutdownMethod how);

    s32 Socket(Family domain, Type type, Protocol protocol);
    s32 SocketExempt(Family domain, Type type, Protocol protocol);

    s32 Accept(s32 desc, SockAddr *out_address, SockLenT *out_addr_len);
    s32 Bind(s32 desc, const SockAddr *address, SockLenT len);

    s32 GetSockName(s32 desc, SockAddr *out_address, SockLenT *out_addr_len);
    s32 SetSockOpt(s32 desc, Level level, Option option_name, const void *option_value, SockLenT option_size);

    s32 Listen(s32 desc, s32 backlog);

    s32 Close(s32 desc);
}
