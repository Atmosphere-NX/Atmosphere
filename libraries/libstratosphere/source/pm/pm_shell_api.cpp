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

namespace ams::pm::shell {

    /* Shell API. */
    #if defined(ATMOSPHERE_OS_HORIZON)
    Result WEAK_SYMBOL LaunchProgram(os::ProcessId *out, const ncm::ProgramLocation &loc, u32 launch_flags) {
        static_assert(sizeof(ncm::ProgramLocation) == sizeof(NcmProgramLocation));
        static_assert(alignof(ncm::ProgramLocation) == alignof(NcmProgramLocation));
        R_RETURN(pmshellLaunchProgram(launch_flags, reinterpret_cast<const NcmProgramLocation *>(std::addressof(loc)), reinterpret_cast<u64 *>(out)));
    }

    Result TerminateProcess(os::ProcessId process_id) {
        R_RETURN(::pmshellTerminateProcess(static_cast<u64>(process_id)));
    }

    Result GetProcessEventEvent(os::SystemEvent *out) {
        ::Event evt;
        R_TRY(::pmshellGetProcessEventHandle(std::addressof(evt)));
        out->Attach(evt.revent, true, svc::InvalidHandle, false, os::EventClearMode_ManualClear);
        R_SUCCEED();
    }

    Result GetProcessEventInfo(ProcessEventInfo *out) {
        static_assert(sizeof(*out) == sizeof(::PmProcessEventInfo));
        R_RETURN(::pmshellGetProcessEventInfo(reinterpret_cast<::PmProcessEventInfo *>(out)));
    }

    Result GetApplicationProcessIdForShell(os::ProcessId *out) {
        static_assert(sizeof(*out) == sizeof(u64));
        R_RETURN(::pmshellGetApplicationProcessIdForShell(reinterpret_cast<u64 *>(out)));
    }

    Result BoostSystemMemoryResourceLimit(u64 size) {
        R_RETURN(::pmshellBoostSystemMemoryResourceLimit(size));
    }

    Result BoostApplicationThreadResourceLimit() {
        R_RETURN(::pmshellBoostApplicationThreadResourceLimit());
    }

    Result BoostSystemThreadResourceLimit() {
        R_RETURN(::pmshellBoostSystemThreadResourceLimit());
    }
    #endif

}
