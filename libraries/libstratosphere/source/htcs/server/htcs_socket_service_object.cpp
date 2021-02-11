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
#include "htcs_socket_service_object.hpp"

namespace ams::htcs::server {

    SocketServiceObject::SocketServiceObject(ManagerServiceObject *manager, s32 desc) : m_manager(manager, true), m_desc(desc) {
        /* ... */
    }

    SocketServiceObject::~SocketServiceObject() {
        AMS_ABORT("SocketServiceObject::~SocketServiceObject");
    }

    Result SocketServiceObject::Close(sf::Out<s32> out_err, sf::Out<s32> out_res) {
        AMS_ABORT("SocketServiceObject::Close");
    }

    Result SocketServiceObject::Connect(sf::Out<s32> out_err, sf::Out<s32> out_res, const htcs::SockAddrHtcs &address) {
        AMS_ABORT("SocketServiceObject::Connect");
    }

    Result SocketServiceObject::Bind(sf::Out<s32> out_err, sf::Out<s32> out_res, const htcs::SockAddrHtcs &address) {
        AMS_ABORT("SocketServiceObject::Bind");
    }

    Result SocketServiceObject::Listen(sf::Out<s32> out_err, sf::Out<s32> out_res, s32 backlog_count) {
        AMS_ABORT("SocketServiceObject::Listen");
    }

    Result SocketServiceObject::Accept(sf::Out<s32> out_err, sf::Out<sf::SharedPointer<tma::ISocket>> out, sf::Out<htcs::SockAddrHtcs> out_address) {
        AMS_ABORT("SocketServiceObject::Accept");
    }

    Result SocketServiceObject::Recv(sf::Out<s32> out_err, sf::Out<s64> out_size, const sf::OutAutoSelectBuffer &buffer, s32 flags) {
        AMS_ABORT("SocketServiceObject::Recv");
    }

    Result SocketServiceObject::Send(sf::Out<s32> out_err, sf::Out<s64> out_size, const sf::InAutoSelectBuffer &buffer, s32 flags) {
        AMS_ABORT("SocketServiceObject::Send");
    }

    Result SocketServiceObject::Shutdown(sf::Out<s32> out_err, sf::Out<s32> out_res, s32 how) {
        AMS_ABORT("SocketServiceObject::Shutdown");
    }

    Result SocketServiceObject::Fcntl(sf::Out<s32> out_err, sf::Out<s32> out_res, s32 command, s32 value) {
        AMS_ABORT("SocketServiceObject::Fcntl");
    }

    Result SocketServiceObject::AcceptStart(sf::Out<u32> out_task_id, sf::OutCopyHandle out_event) {
        AMS_ABORT("SocketServiceObject::AcceptStart");
    }

    Result SocketServiceObject::AcceptResults(sf::Out<s32> out_err, sf::Out<sf::SharedPointer<tma::ISocket>> out, sf::Out<htcs::SockAddrHtcs> out_address, u32 task_id) {
        AMS_ABORT("SocketServiceObject::AcceptResults");
    }

    Result SocketServiceObject::RecvStart(sf::Out<u32> out_task_id, sf::OutCopyHandle out_event, s32 mem_size, s32 flags) {
        AMS_ABORT("SocketServiceObject::RecvStart");
    }

    Result SocketServiceObject::RecvResults(sf::Out<s32> out_err, sf::Out<s64> out_size, const sf::OutAutoSelectBuffer &buffer, u32 task_id) {
        AMS_ABORT("SocketServiceObject::RecvResults");
    }

    Result SocketServiceObject::RecvLargeStart(sf::Out<u32> out_task_id, sf::OutCopyHandle out_event, s32 unaligned_size_start, s32 unaligned_size_end, s64 aligned_size, sf::CopyHandle mem_handle, s32 flags) {
        AMS_ABORT("SocketServiceObject::RecvLargeStart");
    }

    Result SocketServiceObject::SendStartOld(sf::Out<u32> out_task_id, sf::OutCopyHandle out_event, const sf::InAutoSelectBuffer &buffer, s32 flags) {
        AMS_ABORT("SocketServiceObject::SendStartOld");
    }

    Result SocketServiceObject::SendLargeStart(sf::Out<u32> out_task_id, sf::OutCopyHandle out_event, const sf::InAutoSelectBuffer &start_buffer, const sf::InAutoSelectBuffer &end_buffer, sf::CopyHandle mem_handle, s64 aligned_size, s32 flags) {
        AMS_ABORT("SocketServiceObject::SendLargeStart");
    }

    Result SocketServiceObject::SendResults(sf::Out<s32> out_err, sf::Out<s64> out_size, u32 task_id) {
        AMS_ABORT("SocketServiceObject::SendResults");
    }

    Result SocketServiceObject::StartSend(sf::Out<u32> out_task_id, sf::OutCopyHandle out_event, sf::Out<s64> out_max_size, s64 size, s32 flags) {
        AMS_ABORT("SocketServiceObject::StartSend");
    }

    Result SocketServiceObject::ContinueSendOld(sf::Out<s64> out_size, sf::Out<bool> out_wait, const sf::InAutoSelectBuffer &buffer, u32 task_id) {
        AMS_ABORT("SocketServiceObject::ContinueSendOld");
    }

    Result SocketServiceObject::EndSend(sf::Out<s32> out_err, sf::Out<s64> out_size, u32 task_id) {
        AMS_ABORT("SocketServiceObject::EndSend");
    }

    Result SocketServiceObject::StartRecv(sf::Out<u32> out_task_id, sf::OutCopyHandle out_event, s64 size, s32 flags) {
        AMS_ABORT("SocketServiceObject::StartRecv");
    }

    Result SocketServiceObject::EndRecv(sf::Out<s32> out_err, sf::Out<s64> out_size, const sf::OutAutoSelectBuffer &buffer, u32 task_id) {
        AMS_ABORT("SocketServiceObject::EndRecv");
    }

    Result SocketServiceObject::SendStart(sf::Out<u32> out_task_id, sf::OutCopyHandle out_event, const sf::InNonSecureAutoSelectBuffer &buffer, s32 flags) {
        AMS_ABORT("SocketServiceObject::SendStart");
    }

    Result SocketServiceObject::ContinueSend(sf::Out<s64> out_size, sf::Out<bool> out_wait, const sf::InNonSecureAutoSelectBuffer &buffer, u32 task_id) {
        AMS_ABORT("SocketServiceObject::ContinueSend");
    }

    Result SocketServiceObject::GetPrimitive(sf::Out<s32> out) {
        AMS_ABORT("SocketServiceObject::GetPrimitive");
    }

}
