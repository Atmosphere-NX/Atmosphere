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
 
#include <switch.h>
#include <stratosphere.hpp>
#include "pm_registration.hpp"
#include "pm_resource_limits.hpp"
#include "pm_shell.hpp"
#include "pm_boot2.hpp"

static bool g_has_boot_finished = false;

Result ShellService::LaunchProcess(Out<u64> pid, Registration::TidSid tid_sid, u32 launch_flags) {
    return Registration::LaunchProcessByTidSid(tid_sid, launch_flags, pid.GetPointer());
}

Result ShellService::TerminateProcessId(u64 pid) {
    std::scoped_lock<ProcessList &> lk(Registration::GetProcessList());
    
    auto proc = Registration::GetProcess(pid);
    if (proc != nullptr) {
        return svcTerminateProcess(proc->handle);
    } else {
        return ResultPmProcessNotFound;
    }
}

Result ShellService::TerminateTitleId(u64 tid) {
    std::scoped_lock<ProcessList &> lk(Registration::GetProcessList());
    
    auto proc = Registration::GetProcessByTitleId(tid);
    if (proc != NULL) {
        return svcTerminateProcess(proc->handle);
    } else {
        return ResultPmProcessNotFound;
    }
}

void ShellService::GetProcessWaitEvent(Out<CopiedHandle> event) {
    event.SetValue(Registration::GetProcessEventHandle());
}

void ShellService::GetProcessEventType(Out<u64> type, Out<u64> pid) {
    Registration::GetProcessEventType(pid.GetPointer(), type.GetPointer());
}

Result ShellService::FinalizeExitedProcess(u64 pid) {
    std::scoped_lock<ProcessList &> lk(Registration::GetProcessList());
    
    auto proc = Registration::GetProcess(pid);
    if (proc == NULL) {
        return ResultPmProcessNotFound;
    } else if (proc->state != ProcessState_Exited) {
        return ResultPmNotExited;
    } else {
        Registration::FinalizeExitedProcess(proc);
        return ResultSuccess;
    }
}

Result ShellService::ClearProcessNotificationFlag(u64 pid) {
    std::scoped_lock<ProcessList &> lk(Registration::GetProcessList());
    
    auto proc = Registration::GetProcess(pid);
    if (proc != NULL) {
        proc->flags &= ~PROCESSFLAGS_CRASHED;
        return ResultSuccess;
    } else {
        return ResultPmProcessNotFound;
    }
}

void ShellService::NotifyBootFinished() {
    if (!g_has_boot_finished) {
        g_has_boot_finished = true;
        EmbeddedBoot2::Main();
    }
}

Result ShellService::GetApplicationProcessId(Out<u64> pid) {
    std::scoped_lock<ProcessList &> lk(Registration::GetProcessList());
    
    std::shared_ptr<Registration::Process> app_proc;
    if (Registration::HasApplicationProcess(&app_proc)) {
        pid.SetValue(app_proc->pid);
        return ResultSuccess;
    }
    return ResultPmProcessNotFound;
}

Result ShellService::BoostSystemMemoryResourceLimit(u64 sysmem_size) {
    return ResourceLimitUtils::BoostSystemMemoryResourceLimit(sysmem_size);
}

Result ShellService::BoostSystemThreadsResourceLimit() {
    /* Starting in 7.0.0, Nintendo reduces the number of system threads from 0x260 to 0x60, */
    /* Until this command is called to double that amount to 0xC0. */
    /* We will simply not reduce the number of system threads available for no reason. */
    return ResultSuccess;
}


Result ShellService::GetUnimplementedEventHandle(Out<CopiedHandle> event) {
    /* In 8.0.0, Nintendo added this command which should return an event handle. */
    /* In addition, they also added code to create a new event in the global PM constructor. */
    /* However, nothing signals this event, and this command currently does std::abort();. */
    /* We will oblige. */
    std::abort();
    
    /* TODO: Return an event handle, once N makes this command a real thing in the future. */
    /* TODO: return ResultSuccess; */
}
