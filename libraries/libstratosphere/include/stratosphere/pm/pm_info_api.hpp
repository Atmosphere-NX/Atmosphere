/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

#include "pm_types.hpp"
#include "../ncm/ncm_types.hpp"

namespace ams::pm::info {

    /* Information API. */
    Result GetProgramId(ncm::ProgramId *out_program_id, os::ProcessId process_id);
    Result GetProcessId(os::ProcessId *out_process_id, ncm::ProgramId program_id);
    Result HasLaunchedProgram(bool *out, ncm::ProgramId program_id);

    Result GetProcessInfo(ncm::ProgramLocation *out_loc, cfg::OverrideStatus *out_status, os::ProcessId process_id);

    /* Information convenience API. */
    bool HasLaunchedProgram(ncm::ProgramId program_id);

    Result IsHblProcessId(bool *out, os::ProcessId process_id);
    Result IsHblProgramId(bool *out, ncm::ProgramId program_id);

}
