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

namespace ams::htcs::server {

    class ManagerServiceObject : public sf::ISharedObject {
        public:
            Result Socket(sf::Out<s32> out_err, sf::Out<s32> out_sock);
            Result Close(sf::Out<s32> out_err, sf::Out<s32> out_res, s32 desc);
            Result Connect(sf::Out<s32> out_err, sf::Out<s32> out_res, s32 desc, const htcs::SockAddrHtcs &address);
            Result Bind(sf::Out<s32> out_err, sf::Out<s32> out_res, s32 desc, const htcs::SockAddrHtcs &address);
            Result Listen(sf::Out<s32> out_err, sf::Out<s32> out_res, s32 desc, s32 backlog_count);
            Result Accept(sf::Out<s32> out_err, sf::Out<s32> out_res, sf::Out<htcs::SockAddrHtcs> out_address, s32 desc);
            Result Recv(sf::Out<s32> out_err, sf::Out<s64> out_size, const sf::OutBuffer &buffer, s32 desc, s32 flags);
            Result Send(sf::Out<s32> out_err, sf::Out<s64> out_size, s32 desc, const sf::InBuffer &buffer, s32 flags);
            Result Shutdown(sf::Out<s32> out_err, sf::Out<s32> out_res, s32 desc, s32 how);
            Result Fcntl(sf::Out<s32> out_err, sf::Out<s32> out_res, s32 desc, s32 command, s32 value);
            Result GetPeerNameAny(sf::Out<htcs::HtcsPeerName> out);
            Result GetDefaultHostName(sf::Out<htcs::HtcsPeerName> out);
            Result CreateSocketOld(sf::Out<s32> out_err, sf::Out<sf::SharedPointer<tma::ISocket>> out);
            Result CreateSocket(sf::Out<s32> out_err, sf::Out<sf::SharedPointer<tma::ISocket>> out, bool enable_disconnection_emulation);
            Result RegisterProcessId(const sf::ClientProcessId &client_pid);
            Result MonitorManager(const sf::ClientProcessId &client_pid);
            Result StartSelect(sf::Out<u32> out_task_id, sf::OutCopyHandle out_event, const sf::InMapAliasArray<s32> &read_handles, const sf::InMapAliasArray<s32> &write_handles, const sf::InMapAliasArray<s32> &exception_handles, s64 tv_sec, s64 tv_usec);
            Result EndSelect(sf::Out<s32> out_err, sf::Out<s32> out_count, const sf::OutMapAliasArray<s32> &read_handles, const sf::OutMapAliasArray<s32> &write_handles, const sf::OutMapAliasArray<s32> &exception_handles, u32 task_id);
    };
    static_assert(tma::IsIHtcsManager<ManagerServiceObject>);

}
