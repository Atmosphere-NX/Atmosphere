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
#include <stratosphere.hpp>

namespace ams::pm::impl {

    /* Initialization. */
    Result InitializeProcessManager();

    /* Process Management. */
    Result LaunchProgram(os::ProcessId *out_process_id, const ncm::ProgramLocation &loc, u32 flags);
    Result StartProcess(os::ProcessId process_id);
    Result TerminateProcess(os::ProcessId process_id);
    Result TerminateProgram(ncm::ProgramId program_id);
    Result GetProcessEventHandle(os::NativeHandle *out);
    Result GetProcessEventInfo(ProcessEventInfo *out);
    Result CleanupProcess(os::ProcessId process_id);
    Result ClearExceptionOccurred(os::ProcessId process_id);

    /* Information Getters. */
    Result GetModuleIdList(u32 *out_count, u8 *out_buf, size_t max_out_count, u64 unused);
    Result GetExceptionProcessIdList(u32 *out_count, os::ProcessId *out_process_ids, size_t max_out_count);
    Result GetProcessId(os::ProcessId *out, ncm::ProgramId program_id);
    Result GetProgramId(ncm::ProgramId *out, os::ProcessId process_id);
    Result GetApplicationProcessId(os::ProcessId *out_process_id);
    Result AtmosphereGetProcessInfo(os::NativeHandle *out_process_handle, ncm::ProgramLocation *out_loc, cfg::OverrideStatus *out_status, os::ProcessId process_id);

    /* Hook API. */
    Result HookToCreateProcess(os::NativeHandle *out_hook, ncm::ProgramId program_id);
    Result HookToCreateApplicationProcess(os::NativeHandle *out_hook);
    Result ClearHook(u32 which);

    /* Boot API. */
    Result NotifyBootFinished();
    Result GetBootFinishedEventHandle(os::NativeHandle *out);

    /* Resource Limit API. */
    Result BoostSystemMemoryResourceLimit(u64 boost_size);
    Result BoostApplicationThreadResourceLimit();
    Result AtmosphereGetCurrentLimitInfo(s64 *out_cur_val, s64 *out_lim_val, u32 group, u32 resource);

}
