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
#include "impl/socket_api.hpp"

namespace ams::socket {

    Errno GetLastError() {
        return impl::GetLastError();
    }

    void SetLastError(Errno err) {
        return impl::SetLastError(err);
    }

    u32 InetHtonl(u32 host) {
        return impl::InetHtonl(host);
    }

    u16 InetHtons(u16 host) {
        return impl::InetHtons(host);
    }

    u32 InetNtohl(u32 net) {
        return impl::InetNtohl(net);
    }

    u16 InetNtohs(u16 net) {
        return impl::InetNtohs(net);
    }

    Result Initialize(const Config &config) {
        return impl::Initialize(config);
    }

    Result Finalize() {
        return impl::Finalize();
    }

    Result InitializeAllocatorForInternal(void *buffer, size_t size) {
        return impl::InitializeAllocatorForInternal(buffer, size);
    }

    ssize_t RecvFrom(s32 desc, void *buffer, size_t buffer_size, MsgFlag flags, SockAddr *out_address, SockLenT *out_addr_len){
        return impl::RecvFrom(desc, buffer, buffer_size, flags, out_address, out_addr_len);
    }

    ssize_t Recv(s32 desc, void *buffer, size_t buffer_size, MsgFlag flags) {
        return impl::Recv(desc, buffer, buffer_size, flags);
    }

    ssize_t SendTo(s32 desc, const void *buffer, size_t buffer_size, MsgFlag flags, const SockAddr *address, SockLenT len) {
        return impl::SendTo(desc, buffer, buffer_size, flags, address, len);
    }

    ssize_t Send(s32 desc, const void *buffer, size_t buffer_size, MsgFlag flags) {
        return impl::Send(desc, buffer, buffer_size, flags);
    }

    s32 Shutdown(s32 desc, ShutdownMethod how) {
        return impl::Shutdown(desc, how);
    }

    s32 Socket(Family domain, Type type, Protocol protocol) {
        return impl::Socket(domain, type, protocol);
    }

    s32 SocketExempt(Family domain, Type type, Protocol protocol) {
        return impl::SocketExempt(domain, type, protocol);
    }

    s32 Accept(s32 desc, SockAddr *out_address, SockLenT *out_addr_len) {
        return impl::Accept(desc, out_address, out_addr_len);
    }

    s32 Bind(s32 desc, const SockAddr *address, SockLenT len) {
        return impl::Bind(desc, address, len);
    }

    s32 GetSockName(s32 desc, SockAddr *out_address, SockLenT *out_addr_len) {
        return impl::GetSockName(desc, out_address, out_addr_len);
    }

    s32 SetSockOpt(s32 desc, Level level, Option option_name, const void *option_value, SockLenT option_size) {
        return impl::SetSockOpt(desc, level, option_name, option_value, option_size);
    }

    s32 Listen(s32 desc, s32 backlog) {
        return impl::Listen(desc, backlog);
    }

    s32 Close(s32 desc) {
        return impl::Close(desc);
    }

}
