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
#include <stratosphere.hpp>
#include "htcs_manager_service_object.hpp"

namespace ams::htcs::server {

    class SocketServiceObject {
        private:
            sf::SharedPointer<ManagerServiceObject> m_manager;
            s32 m_desc;
        public:
            SocketServiceObject(ManagerServiceObject *manager, s32 desc);
            ~SocketServiceObject();
        public:
            Result Close(sf::Out<s32> out_err, sf::Out<s32> out_res);
            Result Connect(sf::Out<s32> out_err, sf::Out<s32> out_res, const htcs::SockAddrHtcs &address);
            Result Bind(sf::Out<s32> out_err, sf::Out<s32> out_res, const htcs::SockAddrHtcs &address);
            Result Listen(sf::Out<s32> out_err, sf::Out<s32> out_res, s32 backlog_count);
            Result Accept(sf::Out<s32> out_err, sf::Out<sf::SharedPointer<tma::ISocket>> out, sf::Out<htcs::SockAddrHtcs> out_address);
            Result Recv(sf::Out<s32> out_err, sf::Out<s64> out_size, const sf::OutAutoSelectBuffer &buffer, s32 flags);
            Result Send(sf::Out<s32> out_err, sf::Out<s64> out_size, const sf::InAutoSelectBuffer &buffer, s32 flags);
            Result Shutdown(sf::Out<s32> out_err, sf::Out<s32> out_res, s32 how);
            Result Fcntl(sf::Out<s32> out_err, sf::Out<s32> out_res, s32 command, s32 value);
            Result AcceptStart(sf::Out<u32> out_task_id, sf::OutCopyHandle out_event);
            Result AcceptResults(sf::Out<s32> out_err, sf::Out<sf::SharedPointer<tma::ISocket>> out, sf::Out<htcs::SockAddrHtcs> out_address, u32 task_id);
            Result RecvStart(sf::Out<u32> out_task_id, sf::OutCopyHandle out_event, s32 mem_size, s32 flags);
            Result RecvResults(sf::Out<s32> out_err, sf::Out<s64> out_size, const sf::OutAutoSelectBuffer &buffer, u32 task_id);
            Result RecvLargeStart(sf::Out<u32> out_task_id, sf::OutCopyHandle out_event, s32 unaligned_size_start, s32 unaligned_size_end, s64 aligned_size, sf::CopyHandle &&mem_handle, s32 flags);
            Result SendStartOld(sf::Out<u32> out_task_id, sf::OutCopyHandle out_event, const sf::InAutoSelectBuffer &buffer, s32 flags);
            Result SendLargeStart(sf::Out<u32> out_task_id, sf::OutCopyHandle out_event, const sf::InAutoSelectBuffer &start_buffer, const sf::InAutoSelectBuffer &end_buffer, sf::CopyHandle &&mem_handle, s64 aligned_size, s32 flags);
            Result SendResults(sf::Out<s32> out_err, sf::Out<s64> out_size, u32 task_id);
            Result StartSend(sf::Out<u32> out_task_id, sf::OutCopyHandle out_event, sf::Out<s64> out_max_size, s64 size, s32 flags);
            Result ContinueSendOld(sf::Out<s64> out_size, sf::Out<bool> out_wait, const sf::InAutoSelectBuffer &buffer, u32 task_id);
            Result EndSend(sf::Out<s32> out_err, sf::Out<s64> out_size, u32 task_id);
            Result StartRecv(sf::Out<u32> out_task_id, sf::OutCopyHandle out_event, s64 size, s32 flags);
            Result EndRecv(sf::Out<s32> out_err, sf::Out<s64> out_size, const sf::OutAutoSelectBuffer &buffer, u32 task_id);
            Result SendStart(sf::Out<u32> out_task_id, sf::OutCopyHandle out_event, const sf::InNonSecureAutoSelectBuffer &buffer, s32 flags);
            Result ContinueSend(sf::Out<s64> out_size, sf::Out<bool> out_wait, const sf::InNonSecureAutoSelectBuffer &buffer, u32 task_id);
            Result GetPrimitive(sf::Out<s32> out);
    };
    static_assert(tma::IsISocket<SocketServiceObject>);

}
