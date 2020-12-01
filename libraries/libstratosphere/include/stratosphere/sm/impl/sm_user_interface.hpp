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

namespace ams::sm::impl {

    #define AMS_SM_I_USER_INTERFACE_INTERFACE_INFO(C, H)                                                                                                                \
        AMS_SF_METHOD_INFO(C, H,     0, Result, RegisterClient,                   (const sf::ClientProcessId &client_process_id))                                       \
        AMS_SF_METHOD_INFO(C, H,     1, Result, GetServiceHandle,                 (sf::OutMoveHandle out_h, ServiceName service))                                       \
        AMS_SF_METHOD_INFO(C, H,     2, Result, RegisterService,                  (sf::OutMoveHandle out_h, ServiceName service, u32 max_sessions, bool is_light))      \
        AMS_SF_METHOD_INFO(C, H,     3, Result, UnregisterService,                (ServiceName service))                                                                \
        AMS_SF_METHOD_INFO(C, H,     4, Result, DetachClient,                     (const sf::ClientProcessId &client_process_id))                                       \
        AMS_SF_METHOD_INFO(C, H, 65000, Result, AtmosphereInstallMitm,            (sf::OutMoveHandle srv_h, sf::OutMoveHandle qry_h, ServiceName service))              \
        AMS_SF_METHOD_INFO(C, H, 65001, Result, AtmosphereUninstallMitm,          (ServiceName service))                                                                \
        AMS_SF_METHOD_INFO(C, H, 65003, Result, AtmosphereAcknowledgeMitmSession, (sf::Out<MitmProcessInfo> client_info, sf::OutMoveHandle fwd_h, ServiceName service)) \
        AMS_SF_METHOD_INFO(C, H, 65004, Result, AtmosphereHasMitm,                (sf::Out<bool> out, ServiceName service))                                             \
        AMS_SF_METHOD_INFO(C, H, 65005, Result, AtmosphereWaitMitm,               (ServiceName service))                                                                \
        AMS_SF_METHOD_INFO(C, H, 65006, Result, AtmosphereDeclareFutureMitm,      (ServiceName service))                                                                \
        AMS_SF_METHOD_INFO(C, H, 65007, Result, AtmosphereClearFutureMitm,        (ServiceName service))                                                                \
        AMS_SF_METHOD_INFO(C, H, 65100, Result, AtmosphereHasService,             (sf::Out<bool> out, ServiceName service))                                             \
        AMS_SF_METHOD_INFO(C, H, 65101, Result, AtmosphereWaitService,            (ServiceName service))

    AMS_SF_DEFINE_INTERFACE(IUserInterface, AMS_SM_I_USER_INTERFACE_INTERFACE_INFO)

}
