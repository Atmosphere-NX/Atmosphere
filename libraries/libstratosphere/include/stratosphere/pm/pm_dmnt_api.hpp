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

#include <stratosphere/ldr.hpp>
#include <stratosphere/pm/pm_types.hpp>
#include <stratosphere/ncm/ncm_program_location.hpp>

namespace ams::pm::dmnt {

    /* Debug Monitor API. */
    Result StartProcess(os::ProcessId process_id);
    Result GetProgramId(ncm::ProgramId *out_program_id, os::ProcessId process_id);
    Result GetProcessId(os::ProcessId *out_process_id, const ncm::ProgramId program_id);
    Result GetApplicationProcessId(os::ProcessId *out_process_id);
    Result HookToCreateApplicationProcess(os::NativeHandle *out_handle);
    Result HookToCreateProcess(os::NativeHandle *out_handle, const ncm::ProgramId program_id);
    Result AtmosphereGetProcessInfo(os::NativeHandle *out_handle, ncm::ProgramLocation *out_loc, cfg::OverrideStatus *out_status, os::ProcessId process_id);

    #if defined(ATMOSPHERE_OS_HORIZON)
    Result AtmosphereGetCurrentLimitInfo(u64 *out_current_value, u64 *out_limit_value, ResourceLimitGroup group, svc::LimitableResource resource);
    #endif

}
