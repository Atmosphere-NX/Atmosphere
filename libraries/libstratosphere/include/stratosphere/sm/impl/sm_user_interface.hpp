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
#include <stratosphere/sm/sm_types.hpp>
#include <stratosphere/tipc.hpp>

#define AMS_SM_I_USER_INTERFACE_INTERFACE_INFO(C, H)                                                                                                                                                                        \
    AMS_TIPC_METHOD_INFO(C, H,     0, Result, RegisterClient,                   (const tipc::ClientProcessId client_process_id),                                                  (client_process_id))                      \
    AMS_TIPC_METHOD_INFO(C, H,     1, Result, GetServiceHandle,                 (tipc::OutMoveHandle out_h, sm::ServiceName service),                                             (out_h, service))                         \
    AMS_TIPC_METHOD_INFO(C, H,     2, Result, RegisterService,                  (tipc::OutMoveHandle out_h, sm::ServiceName service, u32 max_sessions, bool is_light),            (out_h, service, max_sessions, is_light)) \
    AMS_TIPC_METHOD_INFO(C, H,     3, Result, UnregisterService,                (sm::ServiceName service),                                                                        (service))                                \
    AMS_TIPC_METHOD_INFO(C, H,     4, Result, DetachClient,                     (const tipc::ClientProcessId client_process_id),                                                  (client_process_id))                      \
    AMS_TIPC_METHOD_INFO(C, H, 65000, Result, AtmosphereInstallMitm,            (tipc::OutMoveHandle srv_h, tipc::OutMoveHandle qry_h, sm::ServiceName service),                  (srv_h, qry_h, service))                  \
    AMS_TIPC_METHOD_INFO(C, H, 65001, Result, AtmosphereUninstallMitm,          (sm::ServiceName service),                                                                        (service))                                \
    AMS_TIPC_METHOD_INFO(C, H, 65003, Result, AtmosphereAcknowledgeMitmSession, (tipc::Out<sm::MitmProcessInfo> client_info, tipc::OutMoveHandle fwd_h, sm::ServiceName service), (client_info, fwd_h, service))            \
    AMS_TIPC_METHOD_INFO(C, H, 65004, Result, AtmosphereHasMitm,                (tipc::Out<bool> out, sm::ServiceName service),                                                   (out, service))                           \
    AMS_TIPC_METHOD_INFO(C, H, 65005, Result, AtmosphereWaitMitm,               (sm::ServiceName service),                                                                        (service))                                \
    AMS_TIPC_METHOD_INFO(C, H, 65006, Result, AtmosphereDeclareFutureMitm,      (sm::ServiceName service),                                                                        (service))                                \
    AMS_TIPC_METHOD_INFO(C, H, 65007, Result, AtmosphereClearFutureMitm,        (sm::ServiceName service),                                                                        (service))                                \
    AMS_TIPC_METHOD_INFO(C, H, 65100, Result, AtmosphereHasService,             (tipc::Out<bool> out, sm::ServiceName service),                                                   (out, service))                           \
    AMS_TIPC_METHOD_INFO(C, H, 65101, Result, AtmosphereWaitService,            (sm::ServiceName service),                                                                        (service))

AMS_TIPC_DEFINE_INTERFACE(ams::sm::impl, IUserInterface, AMS_SM_I_USER_INTERFACE_INTERFACE_INFO)
