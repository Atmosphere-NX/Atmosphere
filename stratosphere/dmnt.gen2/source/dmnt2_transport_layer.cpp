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
#include "dmnt2_transport_layer.hpp"

namespace ams::dmnt::transport {

    namespace {

        enum SocketMode {
            SocketMode_Invalid,
            SocketMode_Htcs,
            SocketMode_Tcp,
        };

        constexpr inline const u16 ListenPort_GdbServer   = 22225;
        constexpr inline const u16 ListenPort_GdbDebugLog = 22227;

        constinit os::SdkMutex g_socket_init_mutex;
        constinit SocketMode g_socket_mode = SocketMode_Invalid;

        constexpr inline size_t RequiredAlignment = std::max(os::ThreadStackAlignment, os::MemoryPageSize);

        using SocketConfigType = socket::SystemConfigLightDefault;

        /* TODO: If we ever use resolvers, increase this. */
        constexpr inline size_t SocketAllocatorSize  = 4_KB;
        constexpr inline size_t SocketMemoryPoolSize = util::AlignUp(SocketConfigType::PerTcpSocketWorstCaseMemoryPoolSize + SocketConfigType::PerUdpSocketWorstCaseMemoryPoolSize, os::MemoryPageSize);

        constexpr inline size_t SocketRequiredSize   = util::AlignUp(SocketMemoryPoolSize + SocketAllocatorSize, os::MemoryPageSize);

        /* Declare the memory pool. */
        alignas(RequiredAlignment) constinit u8 g_socket_memory[SocketRequiredSize];

        constexpr inline const SocketConfigType SocketConfig(g_socket_memory, SocketRequiredSize, SocketAllocatorSize, 2);

    }

    void InitializeByHtcs() {
        std::scoped_lock lk(g_socket_init_mutex);
        AMS_ABORT_UNLESS(g_socket_mode == SocketMode_Invalid);

        constexpr auto HtcsSocketCountMax = 8;
        const size_t buffer_size = htcs::GetWorkingMemorySize(HtcsSocketCountMax);
        AMS_ABORT_UNLESS(sizeof(g_socket_memory) >= buffer_size);
        htcs::InitializeForSystem(g_socket_memory, sizeof(g_socket_memory), HtcsSocketCountMax);

        g_socket_mode = SocketMode_Htcs;
    }

    void InitializeByTcp() {
        std::scoped_lock lk(g_socket_init_mutex);
        AMS_ABORT_UNLESS(g_socket_mode == SocketMode_Invalid);

        R_ABORT_UNLESS(socket::Initialize(SocketConfig));

        g_socket_mode = SocketMode_Tcp;
    }

    s32 Socket() {
        switch (g_socket_mode) {
            case SocketMode_Htcs: return htcs::Socket();
            case SocketMode_Tcp:  return socket::Socket(socket::Family::Af_Inet, socket::Type::Sock_Stream, socket::Protocol::IpProto_Tcp);
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    s32 Close(s32 desc) {
        switch (g_socket_mode) {
            case SocketMode_Htcs:   return htcs::Close(desc);
            case SocketMode_Tcp: return socket::Close(desc);
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    s32 Bind(s32 desc, PortName port_name) {
        switch (g_socket_mode) {
            case SocketMode_Htcs:
                {
                    htcs::SockAddrHtcs addr;
                    addr.family    = htcs::HTCS_AF_HTCS;
                    addr.peer_name = htcs::GetPeerNameAny();
                    switch (port_name) {
                        case PortName_GdbServer:   std::strcpy(addr.port_name.name, "iywys@$gdb"); break;
                        case PortName_GdbDebugLog: std::strcpy(addr.port_name.name, "iywys@$dmnt2_log"); break;
                        AMS_UNREACHABLE_DEFAULT_CASE();
                    }

                    return htcs::Bind(desc, std::addressof(addr));
                }
                break;
            case SocketMode_Tcp:
                {
                    socket::SockAddrIn addr = {};
                    addr.sin_family      = socket::Family::Af_Inet;
                    addr.sin_addr.s_addr = socket::InAddr_Any;

                    switch (port_name){
                        case PortName_GdbServer:   addr.sin_port = socket::InetHtons(static_cast<u16>(ListenPort_GdbServer)); break;
                        case PortName_GdbDebugLog: addr.sin_port = socket::InetHtons(static_cast<u16>(ListenPort_GdbDebugLog)); break;
                        AMS_UNREACHABLE_DEFAULT_CASE();
                    }

                    return socket::Bind(desc, reinterpret_cast<socket::SockAddr *>(std::addressof(addr)), sizeof(addr));
                }
                break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    s32 Listen(s32 desc, s32 backlog_count) {
        switch (g_socket_mode) {
            case SocketMode_Htcs: return htcs::Listen(desc, backlog_count);
            case SocketMode_Tcp:  return socket::Listen(desc, backlog_count);
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    s32 Accept(s32 desc) {
        switch (g_socket_mode) {
            case SocketMode_Htcs:
                {
                    htcs::SockAddrHtcs addr;
                    addr.family    = htcs::HTCS_AF_HTCS;
                    addr.peer_name = htcs::GetPeerNameAny();
                    addr.port_name.name[0] = '\x00';

                    return htcs::Accept(desc, std::addressof(addr));
                }
                break;
            case SocketMode_Tcp:
                {
                    socket::SockAddrIn addr = {};
                    socket::SockLenT addr_len = sizeof(addr);

                    return socket::Accept(desc, reinterpret_cast<socket::SockAddr *>(std::addressof(addr)), std::addressof(addr_len));
                }
                break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    s32 Shutdown(s32 desc) {
        switch (g_socket_mode) {
            case SocketMode_Htcs: return htcs::Shutdown(desc, htcs::HTCS_SHUT_RDWR);
            case SocketMode_Tcp:  return socket::Shutdown(desc, socket::ShutdownMethod::Shut_RdWr);
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    ssize_t Recv(s32 desc, void *buffer, size_t buffer_size, s32 flags) {
        switch (g_socket_mode) {
            case SocketMode_Htcs: return htcs::Recv(desc, buffer, buffer_size, flags);
            case SocketMode_Tcp:  return socket::Recv(desc, buffer, buffer_size, static_cast<socket::MsgFlag>(flags));
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    ssize_t Send(s32 desc, const void *buffer, size_t buffer_size, s32 flags) {
        switch (g_socket_mode) {
            case SocketMode_Htcs: return htcs::Send(desc, buffer, buffer_size, flags);
            case SocketMode_Tcp:  return socket::Send(desc, buffer, buffer_size, static_cast<socket::MsgFlag>(flags));
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    s32 GetLastError() {
        switch (g_socket_mode) {
            case SocketMode_Htcs: return htcs::GetLastError();
            case SocketMode_Tcp:  return static_cast<s32>(socket::GetLastError());
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    bool IsLastErrorEAgain() {
        switch (g_socket_mode) {
            case SocketMode_Htcs: return htcs::GetLastError() == htcs::HTCS_EAGAIN;
            case SocketMode_Tcp:  return socket::GetLastError() == socket::Errno::EAgain;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

}
