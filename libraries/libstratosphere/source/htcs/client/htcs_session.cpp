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
#include <stratosphere.hpp>
#include "htcs_session.hpp"

extern "C" {

#include <switch/services/htcs.h>

}

namespace ams::htcs::client {

    namespace {

        struct HtcsObjectAllocatorTag;
        using ObjectAllocator = ams::sf::ExpHeapStaticAllocator<16_KB, HtcsObjectAllocatorTag>;
        using ObjectFactory   = ams::sf::ObjectFactory<typename ObjectAllocator::Policy>;

        class StaticAllocatorInitializer {
            public:
                StaticAllocatorInitializer() {
                    ObjectAllocator::Initialize(lmem::CreateOption_ThreadSafe);
                }
        } g_static_allocator_initializer;

    }

    namespace {

        class RemoteSocket {
            private:
                ::HtcsSocket m_s;
            public:
                RemoteSocket(::HtcsSocket &s) : m_s(s) { /* ... */ }
                ~RemoteSocket() { ::htcsCloseSocket(std::addressof(m_s)); }
            public:
                Result Accept(sf::Out<s32> out_err, sf::Out<sf::SharedPointer<tma::ISocket>> out, sf::Out<htcs::SockAddrHtcs> out_address) { AMS_ABORT("Not Implemented"); }
                Result Recv(sf::Out<s32> out_err, sf::Out<s64> out_size, const sf::OutAutoSelectBuffer &buffer, s32 flags) { AMS_ABORT("Not Implemented"); }
                Result Send(sf::Out<s32> out_err, sf::Out<s64> out_size, const sf::InAutoSelectBuffer &buffer, s32 flags) { AMS_ABORT("Not Implemented"); }
                Result RecvLargeStart(sf::Out<u32> out_task_id, sf::OutCopyHandle out_event, s32 unaligned_size_start, s32 unaligned_size_end, s64 aligned_size, sf::CopyHandle mem_handle, s32 flags) { AMS_ABORT("Not Implemented"); }
                Result SendStartOld(sf::Out<u32> out_task_id, sf::OutCopyHandle out_event, const sf::InAutoSelectBuffer &buffer, s32 flags) { AMS_ABORT("Not Implemented"); }
                Result SendLargeStart(sf::Out<u32> out_task_id, sf::OutCopyHandle out_event, const sf::InAutoSelectBuffer &start_buffer, const sf::InAutoSelectBuffer &end_buffer, sf::CopyHandle mem_handle, s64 aligned_size, s32 flags) { AMS_ABORT("Not Implemented"); }
                Result ContinueSendOld(sf::Out<s64> out_size, sf::Out<bool> out_wait, const sf::InAutoSelectBuffer &buffer, u32 task_id) { AMS_ABORT("Not Implemented"); }

                Result Close(sf::Out<s32> out_err, sf::Out<s32> out_res);
                Result Connect(sf::Out<s32> out_err, sf::Out<s32> out_res, const htcs::SockAddrHtcs &address);
                Result Bind(sf::Out<s32> out_err, sf::Out<s32> out_res, const htcs::SockAddrHtcs &address);
                Result Listen(sf::Out<s32> out_err, sf::Out<s32> out_res, s32 backlog_count);
                Result Shutdown(sf::Out<s32> out_err, sf::Out<s32> out_res, s32 how);
                Result Fcntl(sf::Out<s32> out_err, sf::Out<s32> out_res, s32 command, s32 value);

                Result AcceptStart(sf::Out<u32> out_task_id, sf::OutCopyHandle out_event);
                Result AcceptResults(sf::Out<s32> out_err, sf::Out<sf::SharedPointer<tma::ISocket>> out, sf::Out<htcs::SockAddrHtcs> out_address, u32 task_id);

                Result RecvStart(sf::Out<u32> out_task_id, sf::OutCopyHandle out_event, s32 mem_size, s32 flags);
                Result RecvResults(sf::Out<s32> out_err, sf::Out<s64> out_size, const sf::OutAutoSelectBuffer &buffer, u32 task_id);

                Result SendStart(sf::Out<u32> out_task_id, sf::OutCopyHandle out_event, const sf::InNonSecureAutoSelectBuffer &buffer, s32 flags);
                Result SendResults(sf::Out<s32> out_err, sf::Out<s64> out_size, u32 task_id);

                Result StartSend(sf::Out<u32> out_task_id, sf::OutCopyHandle out_event, sf::Out<s64> out_max_size, s64 size, s32 flags);
                Result ContinueSend(sf::Out<s64> out_size, sf::Out<bool> out_wait, const sf::InNonSecureAutoSelectBuffer &buffer, u32 task_id);
                Result EndSend(sf::Out<s32> out_err, sf::Out<s64> out_size, u32 task_id);

                Result StartRecv(sf::Out<u32> out_task_id, sf::OutCopyHandle out_event, s64 size, s32 flags);
                Result EndRecv(sf::Out<s32> out_err, sf::Out<s64> out_size, const sf::OutAutoSelectBuffer &buffer, u32 task_id);

                Result GetPrimitive(sf::Out<s32> out);
        };
        static_assert(tma::IsISocket<RemoteSocket>);

        class RemoteManager {
            public:
                Result Socket(sf::Out<s32> out_err, sf::Out<s32> out_sock) { AMS_ABORT("Not Implemented"); }
                Result Close(sf::Out<s32> out_err, sf::Out<s32> out_res, s32 desc) { AMS_ABORT("Not Implemented"); }
                Result Connect(sf::Out<s32> out_err, sf::Out<s32> out_res, s32 desc, const htcs::SockAddrHtcs &address) { AMS_ABORT("Not Implemented"); }
                Result Bind(sf::Out<s32> out_err, sf::Out<s32> out_res, s32 desc, const htcs::SockAddrHtcs &address) { AMS_ABORT("Not Implemented"); }
                Result Listen(sf::Out<s32> out_err, sf::Out<s32> out_res, s32 desc, s32 backlog_count) { AMS_ABORT("Not Implemented"); }
                Result Accept(sf::Out<s32> out_err, sf::Out<s32> out_res, sf::Out<htcs::SockAddrHtcs> out_address, s32 desc) { AMS_ABORT("Not Implemented"); }
                Result Recv(sf::Out<s32> out_err, sf::Out<s64> out_size, const sf::OutBuffer &buffer, s32 desc, s32 flags) { AMS_ABORT("Not Implemented"); }
                Result Send(sf::Out<s32> out_err, sf::Out<s64> out_size, s32 desc, const sf::InBuffer &buffer, s32 flags) { AMS_ABORT("Not Implemented"); }
                Result Shutdown(sf::Out<s32> out_err, sf::Out<s32> out_res, s32 desc, s32 how) { AMS_ABORT("Not Implemented"); }
                Result Fcntl(sf::Out<s32> out_err, sf::Out<s32> out_res, s32 desc, s32 command, s32 value) { AMS_ABORT("Not Implemented"); }
                Result CreateSocketOld(sf::Out<s32> out_err, sf::Out<sf::SharedPointer<tma::ISocket>> out) { AMS_ABORT("Not Implemented"); }

                Result GetPeerNameAny(sf::Out<htcs::HtcsPeerName> out);
                Result GetDefaultHostName(sf::Out<htcs::HtcsPeerName> out);
                Result CreateSocket(sf::Out<s32> out_err, sf::Out<sf::SharedPointer<tma::ISocket>> out, bool enable_disconnection_emulation);
                Result RegisterProcessId(const sf::ClientProcessId &client_pid);
                Result MonitorManager(const sf::ClientProcessId &client_pid);
                Result StartSelect(sf::Out<u32> out_task_id, sf::OutCopyHandle out_event, const sf::InMapAliasArray<s32> &read_handles, const sf::InMapAliasArray<s32> &write_handles, const sf::InMapAliasArray<s32> &exception_handles, s64 tv_sec, s64 tv_usec);
                Result EndSelect(sf::Out<s32> out_err, sf::Out<s32> out_count, const sf::OutMapAliasArray<s32> &read_handles, const sf::OutMapAliasArray<s32> &write_handles, const sf::OutMapAliasArray<s32> &exception_handles, u32 task_id);
        };
        static_assert(tma::IsIHtcsManager<RemoteManager>);

        Result RemoteManager::GetPeerNameAny(sf::Out<htcs::HtcsPeerName> out) {
            static_assert(sizeof(htcs::HtcsPeerName) == sizeof(::HtcsPeerName));
            return ::htcsGetPeerNameAny(reinterpret_cast<::HtcsPeerName *>(out.GetPointer()));
        }

        Result RemoteManager::GetDefaultHostName(sf::Out<htcs::HtcsPeerName> out) {
            static_assert(sizeof(htcs::HtcsPeerName) == sizeof(::HtcsPeerName));
            return ::htcsGetDefaultHostName(reinterpret_cast<::HtcsPeerName *>(out.GetPointer()));
        }

        Result RemoteManager::CreateSocket(sf::Out<s32> out_err, sf::Out<sf::SharedPointer<tma::ISocket>> out, bool enable_disconnection_emulation) {
            ::HtcsSocket libnx_socket;
            R_TRY(::htcsCreateSocket(out_err.GetPointer(), std::addressof(libnx_socket), enable_disconnection_emulation));

            R_SUCCEED_IF(*out_err != 0);

            *out = ObjectFactory::CreateSharedEmplaced<tma::ISocket, RemoteSocket>(libnx_socket);
            return ResultSuccess();
        }

        Result RemoteManager::RegisterProcessId(const sf::ClientProcessId &client_pid) {
            /* Handled by libnx init. */
            return ResultSuccess();
        }

        Result RemoteManager::MonitorManager(const sf::ClientProcessId &client_pid) {
            /* Handled by libnx init. */
            return ResultSuccess();
        }

        Result RemoteManager::StartSelect(sf::Out<u32> out_task_id, sf::OutCopyHandle out_event, const sf::InMapAliasArray<s32> &read_handles, const sf::InMapAliasArray<s32> &write_handles, const sf::InMapAliasArray<s32> &exception_handles, s64 tv_sec, s64 tv_usec) {
            return ::htcsStartSelect(out_task_id.GetPointer(), out_event.GetHandlePointer(), read_handles.GetPointer(), read_handles.GetSize(), write_handles.GetPointer(), write_handles.GetSize(), exception_handles.GetPointer(), exception_handles.GetSize(), tv_sec, tv_usec);
        }

        Result RemoteManager::EndSelect(sf::Out<s32> out_err, sf::Out<s32> out_count, const sf::OutMapAliasArray<s32> &read_handles, const sf::OutMapAliasArray<s32> &write_handles, const sf::OutMapAliasArray<s32> &exception_handles, u32 task_id) {
            return ::htcsEndSelect(out_err.GetPointer(), out_count.GetPointer(), read_handles.GetPointer(), read_handles.GetSize(), write_handles.GetPointer(), write_handles.GetSize(), exception_handles.GetPointer(), exception_handles.GetSize(), task_id);
        }

        Result RemoteSocket::Close(sf::Out<s32> out_err, sf::Out<s32> out_res) {
            return ::htcsSocketClose(std::addressof(m_s), out_err.GetPointer(), out_res.GetPointer());
        }

        Result RemoteSocket::Connect(sf::Out<s32> out_err, sf::Out<s32> out_res, const htcs::SockAddrHtcs &address) {
            static_assert(sizeof(htcs::SockAddrHtcs) == sizeof(::SockAddrHtcs));
            return ::htcsSocketConnect(std::addressof(m_s), out_err.GetPointer(), out_res.GetPointer(), reinterpret_cast<const ::SockAddrHtcs *>(std::addressof(address)));
        }

        Result RemoteSocket::Bind(sf::Out<s32> out_err, sf::Out<s32> out_res, const htcs::SockAddrHtcs &address) {
            static_assert(sizeof(htcs::SockAddrHtcs) == sizeof(::SockAddrHtcs));
            return ::htcsSocketBind(std::addressof(m_s), out_err.GetPointer(), out_res.GetPointer(), reinterpret_cast<const ::SockAddrHtcs *>(std::addressof(address)));
        }

        Result RemoteSocket::Listen(sf::Out<s32> out_err, sf::Out<s32> out_res, s32 backlog_count) {
            return ::htcsSocketListen(std::addressof(m_s), out_err.GetPointer(), out_res.GetPointer(), backlog_count);
        }

        Result RemoteSocket::Shutdown(sf::Out<s32> out_err, sf::Out<s32> out_res, s32 how) {
            return ::htcsSocketShutdown(std::addressof(m_s), out_err.GetPointer(), out_res.GetPointer(), how);
        }

        Result RemoteSocket::Fcntl(sf::Out<s32> out_err, sf::Out<s32> out_res, s32 command, s32 value) {
            return ::htcsSocketFcntl(std::addressof(m_s), out_err.GetPointer(), out_res.GetPointer(), command, value);
        }

        Result RemoteSocket::AcceptStart(sf::Out<u32> out_task_id, sf::OutCopyHandle out_event) {
            return ::htcsSocketAcceptStart(std::addressof(m_s), out_task_id.GetPointer(), out_event.GetHandlePointer());
        }

        Result RemoteSocket::AcceptResults(sf::Out<s32> out_err, sf::Out<sf::SharedPointer<tma::ISocket>> out, sf::Out<htcs::SockAddrHtcs> out_address, u32 task_id) {
            static_assert(sizeof(htcs::SockAddrHtcs) == sizeof(::SockAddrHtcs));
            ::HtcsSocket libnx_socket;
            R_TRY(::htcsSocketAcceptResults(std::addressof(m_s), out_err.GetPointer(), std::addressof(libnx_socket), reinterpret_cast<::SockAddrHtcs *>(out_address.GetPointer()), task_id));

            R_SUCCEED_IF(*out_err != 0);

            *out = ObjectFactory::CreateSharedEmplaced<tma::ISocket, RemoteSocket>(libnx_socket);
            return ResultSuccess();
        }

        Result RemoteSocket::RecvStart(sf::Out<u32> out_task_id, sf::OutCopyHandle out_event, s32 mem_size, s32 flags) {
            return ::htcsSocketRecvStart(std::addressof(m_s), out_task_id.GetPointer(), out_event.GetHandlePointer(), mem_size, flags);
        }

        Result RemoteSocket::RecvResults(sf::Out<s32> out_err, sf::Out<s64> out_size, const sf::OutAutoSelectBuffer &buffer, u32 task_id) {
            return ::htcsSocketRecvResults(std::addressof(m_s), out_err.GetPointer(), out_size.GetPointer(), buffer.GetPointer(), buffer.GetSize(), task_id);
        }

        Result RemoteSocket::SendStart(sf::Out<u32> out_task_id, sf::OutCopyHandle out_event, const sf::InNonSecureAutoSelectBuffer &buffer, s32 flags) {
            return ::htcsSocketSendStart(std::addressof(m_s), out_task_id.GetPointer(), out_event.GetHandlePointer(), buffer.GetPointer(), buffer.GetSize(), flags);
        }

        Result RemoteSocket::SendResults(sf::Out<s32> out_err, sf::Out<s64> out_size, u32 task_id) {
            return ::htcsSocketSendResults(std::addressof(m_s), out_err.GetPointer(), out_size.GetPointer(), task_id);
        }

        Result RemoteSocket::StartSend(sf::Out<u32> out_task_id, sf::OutCopyHandle out_event, sf::Out<s64> out_max_size, s64 size, s32 flags) {
            return ::htcsSocketStartSend(std::addressof(m_s), out_task_id.GetPointer(), out_event.GetHandlePointer(), out_max_size.GetPointer(), size, flags);
        }

        Result RemoteSocket::ContinueSend(sf::Out<s64> out_size, sf::Out<bool> out_wait, const sf::InNonSecureAutoSelectBuffer &buffer, u32 task_id) {
            return ::htcsSocketContinueSend(std::addressof(m_s), out_size.GetPointer(), out_wait.GetPointer(), buffer.GetPointer(), buffer.GetSize(), task_id);
        }

        Result RemoteSocket::EndSend(sf::Out<s32> out_err, sf::Out<s64> out_size, u32 task_id) {
            return ::htcsSocketEndSend(std::addressof(m_s), out_err.GetPointer(), out_size.GetPointer(), task_id);
        }

        Result RemoteSocket::StartRecv(sf::Out<u32> out_task_id, sf::OutCopyHandle out_event, s64 size, s32 flags) {
            return ::htcsSocketStartRecv(std::addressof(m_s), out_task_id.GetPointer(), out_event.GetHandlePointer(), size, flags);
        }

        Result RemoteSocket::EndRecv(sf::Out<s32> out_err, sf::Out<s64> out_size, const sf::OutAutoSelectBuffer &buffer, u32 task_id) {
            return ::htcsSocketEndRecv(std::addressof(m_s), out_err.GetPointer(), out_size.GetPointer(), buffer.GetPointer(), buffer.GetSize(), task_id);
        }

        Result RemoteSocket::GetPrimitive(sf::Out<s32> out) {
            return ::htcsSocketGetPrimitive(std::addressof(m_s), out.GetPointer());
        }

    }

    void InitializeSessionManager(tma::IHtcsManager **out_manager, tma::IHtcsManager **out_monitor, u32 num_sessions) {
        /* Initialize the libnx wrapper. */
        sm::DoWithSession([&] {
            R_ABORT_UNLESS(::htcsInitialize(num_sessions));
        });

        /* Create the output objects. */
        *out_manager = ObjectFactory::CreateSharedEmplaced<tma::IHtcsManager, RemoteManager>().Detach();

        /* Create the output objects. */
        *out_monitor = ObjectFactory::CreateSharedEmplaced<tma::IHtcsManager, RemoteManager>().Detach();
    }

    void FinalizeSessionManager() {
        /* Exit the libnx wrapper. */
        ::htcsExit();
    }

}
