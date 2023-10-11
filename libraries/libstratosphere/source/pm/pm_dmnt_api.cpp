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
#include <stratosphere.hpp>

#include "pm_ams.os.horizon.h"

namespace ams::pm::dmnt {

    /* Debug Monitor API. */
    #if defined(ATMOSPHERE_OS_HORIZON)
    Result StartProcess(os::ProcessId process_id) {
        R_RETURN(pmdmntStartProcess(static_cast<u64>(process_id)));
    }

    Result GetProgramId(ncm::ProgramId *out_program_id, os::ProcessId process_id) {
        R_RETURN(pmdmntGetProgramId(reinterpret_cast<u64 *>(out_program_id), static_cast<u64>(process_id)));
    }

    Result GetProcessId(os::ProcessId *out_process_id, const ncm::ProgramId program_id) {
        R_RETURN(pmdmntGetProcessId(reinterpret_cast<u64 *>(out_process_id), static_cast<u64>(program_id)));
    }

    Result GetApplicationProcessId(os::ProcessId *out_process_id) {
        R_RETURN(pmdmntGetApplicationProcessId(reinterpret_cast<u64 *>(out_process_id)));
    }

    Result HookToCreateApplicationProcess(os::NativeHandle *out_handle) {
        Event evt;
        R_TRY(pmdmntHookToCreateApplicationProcess(std::addressof(evt)));
        *out_handle = evt.revent;
        R_SUCCEED();
    }

    Result HookToCreateProcess(os::NativeHandle *out_handle, const ncm::ProgramId program_id) {
        Event evt;
        R_TRY(pmdmntHookToCreateProcess(std::addressof(evt), static_cast<u64>(program_id)));
        *out_handle = evt.revent;
        R_SUCCEED();
    }

    Result AtmosphereGetProcessInfo(os::NativeHandle *out_handle, ncm::ProgramLocation *out_loc, cfg::OverrideStatus *out_status, os::ProcessId process_id) {
        *out_handle = os::InvalidNativeHandle;
        *out_loc = {};
        *out_status = {};
        static_assert(sizeof(*out_status) == sizeof(CfgOverrideStatus));
        R_RETURN(pmdmntAtmosphereGetProcessInfo(out_handle, reinterpret_cast<NcmProgramLocation *>(out_loc), reinterpret_cast<CfgOverrideStatus *>(out_status), static_cast<u64>(process_id)));
    }

    Result AtmosphereGetCurrentLimitInfo(u64 *out_current_value, u64 *out_limit_value, ResourceLimitGroup group, svc::LimitableResource resource) {
        *out_current_value = 0;
        *out_limit_value   = 0;
        R_RETURN(pmdmntAtmosphereGetCurrentLimitInfo(out_current_value, out_limit_value, group, resource));
    }
    #endif

}
