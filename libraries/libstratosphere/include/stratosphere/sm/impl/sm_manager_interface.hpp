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

#define AMS_SM_I_MANAGER_INTERFACE_INTERFACE_INFO(C, H)                                                                                                                                                                                                                                       \
    AMS_TIPC_METHOD_INFO(C, H,     0, Result, RegisterProcess,           (os::ProcessId process_id, const tipc::InBuffer acid_sac, const tipc::InBuffer aci_sac),                                                                 (process_id, acid_sac, aci_sac))                              \
    AMS_TIPC_METHOD_INFO(C, H,     1, Result, UnregisterProcess,         (os::ProcessId process_id),                                                                                                                              (process_id))                                                 \
    AMS_TIPC_METHOD_INFO(C, H, 65000, void,   AtmosphereEndInitDefers,   (),                                                                                                                                                      ())                                                           \
    AMS_TIPC_METHOD_INFO(C, H, 65001, void,   AtmosphereHasMitm,         (tipc::Out<bool> out, sm::ServiceName service),                                                                                                          (out, service))                                               \
    AMS_TIPC_METHOD_INFO(C, H, 65002, Result, AtmosphereRegisterProcess, (os::ProcessId process_id, ncm::ProgramId program_id, cfg::OverrideStatus override_status, const tipc::InBuffer acid_sac, const tipc::InBuffer aci_sac), (process_id, program_id, override_status, acid_sac, aci_sac))

AMS_TIPC_DEFINE_INTERFACE(ams::sm::impl, IManagerInterface, AMS_SM_I_MANAGER_INTERFACE_INTERFACE_INFO)
