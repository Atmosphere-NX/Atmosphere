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

namespace ams::pm::shell {

    /* Shell API. */
    Result LaunchProgram(os::ProcessId *out, const ncm::ProgramLocation &loc, u32 launch_flags);
    Result TerminateProcess(os::ProcessId process_id);
    Result GetProcessEventEvent(os::SystemEvent *out);
    Result GetProcessEventInfo(ProcessEventInfo *out);
    Result GetApplicationProcessIdForShell(os::ProcessId *out);
    Result BoostSystemMemoryResourceLimit(u64 size);
    Result EnableApplicationExtraThread();

}
