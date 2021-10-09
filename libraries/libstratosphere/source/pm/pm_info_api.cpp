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
#include "pm_ams.h"

namespace ams::pm::info {

    /* Information API. */
    Result GetProgramId(ncm::ProgramId *out_program_id, os::ProcessId process_id) {
        return pminfoGetProgramId(reinterpret_cast<u64 *>(out_program_id), static_cast<u64>(process_id));
    }

    Result GetProcessId(os::ProcessId *out_process_id, ncm::ProgramId program_id) {
        return pminfoAtmosphereGetProcessId(reinterpret_cast<u64 *>(out_process_id), static_cast<u64>(program_id));
    }

    Result GetProcessInfo(ncm::ProgramLocation *out_loc, cfg::OverrideStatus *out_status, os::ProcessId process_id) {
        *out_loc = {};
        *out_status = {};
        static_assert(sizeof(*out_status) == sizeof(CfgOverrideStatus));
        return pminfoAtmosphereGetProcessInfo(reinterpret_cast<NcmProgramLocation *>(out_loc), reinterpret_cast<CfgOverrideStatus *>(out_status), static_cast<u64>(process_id));
    }

    bool HasLaunchedBootProgram(ncm::ProgramId program_id) {
        bool has_launched = false;
        R_ABORT_UNLESS(HasLaunchedBootProgram(std::addressof(has_launched), program_id));
        return has_launched;
    }


    Result IsHblProcessId(bool *out, os::ProcessId process_id) {
        ncm::ProgramLocation loc;
        cfg::OverrideStatus override_status;
        R_TRY(GetProcessInfo(std::addressof(loc), std::addressof(override_status), process_id));

        *out = override_status.IsHbl();
        return ResultSuccess();
    }

    Result IsHblProgramId(bool *out, ncm::ProgramId program_id) {
        os::ProcessId process_id;
        R_TRY(GetProcessId(std::addressof(process_id), program_id));

        return IsHblProcessId(out, process_id);
    }

}
