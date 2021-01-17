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

#pragma once
#include <vapours.hpp>
#include <stratosphere/sm/sm_types.hpp>
#include <stratosphere/sf.hpp>

#define AMS_SM_I_USER_INTERFACE_INTERFACE_INFO(C, H)                                                                                                                                                              \
    AMS_SF_METHOD_INFO(C, H,     0, Result, RegisterClient,                   (const sf::ClientProcessId &client_process_id),                                           (client_process_id))                      \
    AMS_SF_METHOD_INFO(C, H,     1, Result, GetServiceHandle,                 (sf::OutMoveHandle out_h, sm::ServiceName service),                                       (out_h, service))                         \
    AMS_SF_METHOD_INFO(C, H,     2, Result, RegisterService,                  (sf::OutMoveHandle out_h, sm::ServiceName service, u32 max_sessions, bool is_light),      (out_h, service, max_sessions, is_light)) \
    AMS_SF_METHOD_INFO(C, H,     3, Result, UnregisterService,                (sm::ServiceName service),                                                                (service))                                \
    AMS_SF_METHOD_INFO(C, H,     4, Result, DetachClient,                     (const sf::ClientProcessId &client_process_id),                                           (client_process_id))                      \
    AMS_SF_METHOD_INFO(C, H, 65000, Result, AtmosphereInstallMitm,            (sf::OutMoveHandle srv_h, sf::OutMoveHandle qry_h, sm::ServiceName service),              (srv_h, qry_h, service))                  \
    AMS_SF_METHOD_INFO(C, H, 65001, Result, AtmosphereUninstallMitm,          (sm::ServiceName service),                                                                (service))                                \
    AMS_SF_METHOD_INFO(C, H, 65003, Result, AtmosphereAcknowledgeMitmSession, (sf::Out<sm::MitmProcessInfo> client_info, sf::OutMoveHandle fwd_h, sm::ServiceName service), (client_info, fwd_h, service))        \
    AMS_SF_METHOD_INFO(C, H, 65004, Result, AtmosphereHasMitm,                (sf::Out<bool> out, sm::ServiceName service),                                             (out, service))                           \
    AMS_SF_METHOD_INFO(C, H, 65005, Result, AtmosphereWaitMitm,               (sm::ServiceName service),                                                                (service))                                \
    AMS_SF_METHOD_INFO(C, H, 65006, Result, AtmosphereDeclareFutureMitm,      (sm::ServiceName service),                                                                (service))                                \
    AMS_SF_METHOD_INFO(C, H, 65007, Result, AtmosphereClearFutureMitm,        (sm::ServiceName service),                                                                (service))                                \
    AMS_SF_METHOD_INFO(C, H, 65100, Result, AtmosphereHasService,             (sf::Out<bool> out, sm::ServiceName service),                                             (out, service))                           \
    AMS_SF_METHOD_INFO(C, H, 65101, Result, AtmosphereWaitService,            (sm::ServiceName service),                                                                (service))

AMS_SF_DEFINE_INTERFACE(ams::sm::impl, IUserInterface, AMS_SM_I_USER_INTERFACE_INTERFACE_INFO)
