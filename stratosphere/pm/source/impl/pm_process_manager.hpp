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
#include <switch.h>
#include <stratosphere.hpp>
#include <stratosphere/ldr.hpp>
#include <stratosphere/pm.hpp>

namespace sts::pm::impl {

    /* Initialization. */
    Result InitializeProcessManager();

    /* Process Management. */
    Result LaunchTitle(u64 *out_process_id, const ncm::TitleLocation &loc, u32 flags);
    Result StartProcess(u64 process_id);
    Result TerminateProcess(u64 process_id);
    Result TerminateTitle(ncm::TitleId title_id);
    Result GetProcessEventHandle(Handle *out);
    Result GetProcessEventInfo(ProcessEventInfo *out);
    Result CleanupProcess(u64 process_id);
    Result ClearExceptionOccurred(u64 process_id);

    /* Information Getters. */
    Result GetModuleIdList(u32 *out_count, u8 *out_buf, size_t max_out_count, u64 unused);
    Result GetExceptionProcessIdList(u32 *out_count, u64 *out_process_ids, size_t max_out_count);
    Result GetProcessId(u64 *out, ncm::TitleId title_id);
    Result GetTitleId(ncm::TitleId *out, u64 process_id);
    Result GetApplicationProcessId(u64 *out_process_id);
    Result AtmosphereGetProcessInfo(Handle *out_process_handle, ncm::TitleLocation *out_loc, u64 process_id);

    /* Hook API. */
    Result HookToCreateProcess(Handle *out_hook, ncm::TitleId title_id);
    Result HookToCreateApplicationProcess(Handle *out_hook);
    Result ClearHook(u32 which);

    /* Boot API. */
    Result NotifyBootFinished();
    Result GetBootFinishedEventHandle(Handle *out);

    /* Resource Limit API. */
    Result BoostSystemMemoryResourceLimit(u64 boost_size);
    Result BoostApplicationThreadResourceLimit();
    Result AtmosphereGetCurrentLimitInfo(u64 *out_cur_val, u64 *out_lim_val, u32 group, u32 resource);

}
