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
#include <stratosphere.hpp>

#include "pm_ams.h"

namespace ams::pm::dmnt {

    /* Debug Monitor API. */
    Result StartProcess(os::ProcessId process_id) {
        return pmdmntStartProcess(static_cast<u64>(process_id));
    }

    Result GetProcessId(os::ProcessId *out_process_id, const ncm::ProgramId program_id) {
        return pmdmntGetProcessId(reinterpret_cast<u64 *>(out_process_id), static_cast<u64>(program_id));
    }

    Result GetApplicationProcessId(os::ProcessId *out_process_id) {
        return pmdmntGetApplicationProcessId(reinterpret_cast<u64 *>(out_process_id));
    }

    Result HookToCreateApplicationProcess(Handle *out_handle) {
        Event evt;
        R_TRY(pmdmntHookToCreateApplicationProcess(&evt));
        *out_handle = evt.revent;
        return ResultSuccess();
    }

    Result AtmosphereGetProcessInfo(Handle *out_handle, ncm::ProgramLocation *out_loc, cfg::OverrideStatus *out_status, os::ProcessId process_id) {
        *out_handle = INVALID_HANDLE;
        *out_loc = {};
        *out_status = {};
        static_assert(sizeof(*out_status) == sizeof(CfgOverrideStatus));
        return pmdmntAtmosphereGetProcessInfo(out_handle, reinterpret_cast<NcmProgramLocation *>(out_loc), reinterpret_cast<CfgOverrideStatus *>(out_status), static_cast<u64>(process_id));
    }

    Result AtmosphereGetCurrentLimitInfo(u64 *out_current_value, u64 *out_limit_value, ResourceLimitGroup group, LimitableResource resource) {
        *out_current_value = 0;
        *out_limit_value   = 0;
        return pmdmntAtmosphereGetCurrentLimitInfo(out_current_value, out_limit_value, group, resource);
    }

}
