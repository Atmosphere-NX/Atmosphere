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
#include <stratosphere/tma/tma_i_socket.hpp>

/* NOTE: Minimum firmware version not enforced for any commands. */
#define AMS_TMA_I_HTCS_MANAGER_INTERFACE_INFO(C, H)                                                                                                                                                                                                                                                                                                                                        \
    AMS_SF_METHOD_INFO(C, H,   0, Result, Socket,             (sf::Out<s32> out_err, sf::Out<s32> out_sock),                                                                                                                                                                                    (out_err, out_sock))                                                                       \
    AMS_SF_METHOD_INFO(C, H,   1, Result, Close,              (sf::Out<s32> out_err, sf::Out<s32> out_res, s32 desc),                                                                                                                                                                           (out_err, out_res, desc))                                                                  \
    AMS_SF_METHOD_INFO(C, H,   2, Result, Connect,            (sf::Out<s32> out_err, sf::Out<s32> out_res, s32 desc, const htcs::SockAddrHtcs &address),                                                                                                                                        (out_err, out_res, desc, address))                                                         \
    AMS_SF_METHOD_INFO(C, H,   3, Result, Bind,               (sf::Out<s32> out_err, sf::Out<s32> out_res, s32 desc, const htcs::SockAddrHtcs &address),                                                                                                                                        (out_err, out_res, desc, address))                                                         \
    AMS_SF_METHOD_INFO(C, H,   4, Result, Listen,             (sf::Out<s32> out_err, sf::Out<s32> out_res, s32 desc, s32 backlog_count),                                                                                                                                                        (out_err, out_res, desc, backlog_count))                                                   \
    AMS_SF_METHOD_INFO(C, H,   5, Result, Accept,             (sf::Out<s32> out_err, sf::Out<s32> out_res, sf::Out<htcs::SockAddrHtcs> out_address, s32 desc),                                                                                                                                  (out_err, out_res, out_address, desc))                                                     \
    AMS_SF_METHOD_INFO(C, H,   6, Result, Recv,               (sf::Out<s32> out_err, sf::Out<s64> out_size, const sf::OutBuffer &buffer, s32 desc, s32 flags),                                                                                                                                  (out_err, out_size, buffer, desc, flags))                                                  \
    AMS_SF_METHOD_INFO(C, H,   7, Result, Send,               (sf::Out<s32> out_err, sf::Out<s64> out_size, s32 desc, const sf::InBuffer &buffer, s32 flags),                                                                                                                                   (out_err, out_size, desc, buffer, flags))                                                  \
    AMS_SF_METHOD_INFO(C, H,   8, Result, Shutdown,           (sf::Out<s32> out_err, sf::Out<s32> out_res, s32 desc, s32 how),                                                                                                                                                                  (out_err, out_res, desc, how))                                                             \
    AMS_SF_METHOD_INFO(C, H,   9, Result, Fcntl,              (sf::Out<s32> out_err, sf::Out<s32> out_res, s32 desc, s32 command, s32 value),                                                                                                                                                   (out_err, out_res, desc, command, value))                                                  \
    AMS_SF_METHOD_INFO(C, H,  10, Result, GetPeerNameAny,     (sf::Out<htcs::HtcsPeerName> out),                                                                                                                                                                                                (out))                                                                                     \
    AMS_SF_METHOD_INFO(C, H,  11, Result, GetDefaultHostName, (sf::Out<htcs::HtcsPeerName> out),                                                                                                                                                                                                (out))                                                                                     \
    AMS_SF_METHOD_INFO(C, H,  12, Result, CreateSocketOld,    (sf::Out<s32> out_err, sf::Out<sf::SharedPointer<tma::ISocket>> out),                                                                                                                                                             (out_err, out))                                                                            \
    AMS_SF_METHOD_INFO(C, H,  13, Result, CreateSocket,       (sf::Out<s32> out_err, sf::Out<sf::SharedPointer<tma::ISocket>> out, bool enable_disconnection_emulation),                                                                                                                        (out_err, out, enable_disconnection_emulation))                                            \
    AMS_SF_METHOD_INFO(C, H, 100, Result, RegisterProcessId,  (const sf::ClientProcessId &client_pid),                                                                                                                                                                                          (client_pid))                                                                              \
    AMS_SF_METHOD_INFO(C, H, 101, Result, MonitorManager,     (const sf::ClientProcessId &client_pid),                                                                                                                                                                                          (client_pid))                                                                              \
    AMS_SF_METHOD_INFO(C, H, 130, Result, StartSelect,        (sf::Out<u32> out_task_id, sf::OutCopyHandle out_event, const sf::InMapAliasArray<s32> &read_handles, const sf::InMapAliasArray<s32> &write_handles, const sf::InMapAliasArray<s32> &exception_handles, s64 tv_sec, s64 tv_usec), (out_task_id, out_event, read_handles, write_handles, exception_handles, tv_sec, tv_usec)) \
    AMS_SF_METHOD_INFO(C, H, 131, Result, EndSelect,          (sf::Out<s32> out_err, sf::Out<s32> out_count, const sf::OutMapAliasArray<s32> &read_handles, const sf::OutMapAliasArray<s32> &write_handles, const sf::OutMapAliasArray<s32> &exception_handles, u32 task_id),                   (out_err, out_count, read_handles, write_handles, exception_handles, task_id))

AMS_SF_DEFINE_INTERFACE(ams::tma, IHtcsManager, AMS_TMA_I_HTCS_MANAGER_INTERFACE_INFO)
