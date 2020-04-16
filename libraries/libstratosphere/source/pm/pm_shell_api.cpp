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

namespace ams::pm::shell {

    /* Shell API. */
    Result WEAK_SYMBOL LaunchProgram(os::ProcessId *out, const ncm::ProgramLocation &loc, u32 launch_flags) {
        static_assert(sizeof(ncm::ProgramLocation) == sizeof(NcmProgramLocation));
        static_assert(alignof(ncm::ProgramLocation) == alignof(NcmProgramLocation));
        return pmshellLaunchProgram(launch_flags, reinterpret_cast<const NcmProgramLocation *>(&loc), reinterpret_cast<u64 *>(out));
    }

    Result TerminateProcess(os::ProcessId process_id) {
        return ::pmshellTerminateProcess(static_cast<u64>(process_id));
    }

    Result GetProcessEventEvent(os::SystemEvent *out) {
        ::Event evt;
        R_TRY(::pmshellGetProcessEventHandle(std::addressof(evt)));
        out->Attach(evt.revent, true, svc::InvalidHandle, false, os::EventClearMode_ManualClear);
        return ResultSuccess();
    }

    Result GetProcessEventInfo(ProcessEventInfo *out) {
        static_assert(sizeof(*out) == sizeof(::PmProcessEventInfo));
        return ::pmshellGetProcessEventInfo(reinterpret_cast<::PmProcessEventInfo *>(out));
    }

    Result GetApplicationProcessIdForShell(os::ProcessId *out) {
        static_assert(sizeof(*out) == sizeof(u64));
        return ::pmshellGetApplicationProcessIdForShell(reinterpret_cast<u64 *>(out));
    }

    Result BoostSystemMemoryResourceLimit(u64 size) {
        return ::pmshellBoostSystemMemoryResourceLimit(size);
    }

    Result EnableApplicationExtraThread() {
        return ::pmshellEnableApplicationExtraThread();
    }

}
