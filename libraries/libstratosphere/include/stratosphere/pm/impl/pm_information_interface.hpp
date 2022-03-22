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
#include <stratosphere/pm/pm_types.hpp>
#include <stratosphere/sf.hpp>

#define AMS_PM_I_INFORMATION_INTERFACE_INTERFACE_INFO(C, H)                                                                                                                                                                  \
    AMS_SF_METHOD_INFO(C, H,     0, Result, GetProgramId,                     (sf::Out<ncm::ProgramId> out, os::ProcessId process_id),                                                    (out, process_id))                 \
    AMS_SF_METHOD_INFO(C, H, 65000, Result, AtmosphereGetProcessId,           (sf::Out<os::ProcessId> out, ncm::ProgramId program_id),                                                    (out, program_id))                 \
    AMS_SF_METHOD_INFO(C, H, 65001, Result, AtmosphereHasLaunchedBootProgram, (sf::Out<bool> out, ncm::ProgramId program_id),                                                             (out, program_id))                 \
    AMS_SF_METHOD_INFO(C, H, 65002, Result, AtmosphereGetProcessInfo,         (sf::Out<ncm::ProgramLocation> out_loc, sf::Out<cfg::OverrideStatus> out_status, os::ProcessId process_id), (out_loc, out_status, process_id))

AMS_SF_DEFINE_INTERFACE(ams::pm::impl, IInformationInterface, AMS_PM_I_INFORMATION_INTERFACE_INTERFACE_INFO)
