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
#include <stratosphere/ldr/ldr_types.hpp>
#include <stratosphere/sf.hpp>

namespace ams::ldr::impl {

    #define AMS_LDR_I_PROCESS_MANAGER_INTERFACE_INTERFACE_INFO(C, H)                                                                                                                                                   \
        AMS_SF_METHOD_INFO(C, H,     0, Result, CreateProcess,                 (sf::OutMoveHandle proc_h, PinId id, u32 flags, sf::CopyHandle reslimit_h))                                                             \
        AMS_SF_METHOD_INFO(C, H,     1, Result, GetProgramInfo,                (sf::Out<ProgramInfo> out_program_info, const ncm::ProgramLocation &loc))                                                               \
        AMS_SF_METHOD_INFO(C, H,     2, Result, PinProgram,                    (sf::Out<PinId> out_id, const ncm::ProgramLocation &loc))                                                                               \
        AMS_SF_METHOD_INFO(C, H,     3, Result, UnpinProgram,                  (PinId id))                                                                                                                             \
        AMS_SF_METHOD_INFO(C, H,     4, Result, SetEnabledProgramVerification, (bool enabled),                                                                                                    hos::Version_10_0_0) \
        AMS_SF_METHOD_INFO(C, H, 65000, void,   AtmosphereHasLaunchedProgram,  (sf::Out<bool> out, ncm::ProgramId program_id))                                                                                         \
        AMS_SF_METHOD_INFO(C, H, 65001, Result, AtmosphereGetProgramInfo,      (sf::Out<ProgramInfo> out_program_info, sf::Out<cfg::OverrideStatus> out_status, const ncm::ProgramLocation &loc))                      \
        AMS_SF_METHOD_INFO(C, H, 65002, Result, AtmospherePinProgram,          (sf::Out<PinId> out_id, const ncm::ProgramLocation &loc, const cfg::OverrideStatus &override_status))

    AMS_SF_DEFINE_INTERFACE(IProcessManagerInterface, AMS_LDR_I_PROCESS_MANAGER_INTERFACE_INTERFACE_INFO)

}
