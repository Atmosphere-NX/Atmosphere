/*
 * Copyright (c) 2018 Atmosphère-NX
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
    auto auto_lock = Registration::GetProcessListUniqueLock();
    
    auto proc = Registration::GetProcess(pid);
    if (proc != nullptr) {
        return svcTerminateProcess(proc->handle);
    } else {
        return 0x20F;
    }
}

Result ShellService::TerminateTitleId(u64 tid) {
    auto auto_lock = Registration::GetProcessListUniqueLock();
    
    auto proc = Registration::GetProcessByTitleId(tid);
    if (proc != NULL) {
        return svcTerminateProcess(proc->handle);
    } else {
        return 0x20F;
    }
}

void ShellService::GetProcessWaitEvent(Out<CopiedHandle> event) {
    event.SetValue(Registration::GetProcessEventHandle());
}

void ShellService::GetProcessEventType(Out<u64> type, Out<u64> pid) {
    Registration::GetProcessEventType(pid.GetPointer(), type.GetPointer());
}

Result ShellService::FinalizeExitedProcess(u64 pid) {
    auto auto_lock = Registration::GetProcessListUniqueLock();
    
    auto proc = Registration::GetProcess(pid);
    if (proc == NULL) {
        return 0x20F;
    } else if (proc->state != ProcessState_Exited) {
        return 0x60F;
    } else {
        Registration::FinalizeExitedProcess(proc);
        return 0x0;
    }
}

Result ShellService::ClearProcessNotificationFlag(u64 pid) {
    auto auto_lock = Registration::GetProcessListUniqueLock();
    
    auto proc = Registration::GetProcess(pid);
    if (proc != NULL) {
        proc->flags &= ~PROCESSFLAGS_CRASHED;
        return 0x0;
    } else {
        return 0x20F;
    }
}

void ShellService::NotifyBootFinished() {
    if (!g_has_boot_finished) {
        g_has_boot_finished = true;
        EmbeddedBoot2::Main();
    }
}

Result ShellService::GetApplicationProcessId(Out<u64> pid) {
    auto auto_lock = Registration::GetProcessListUniqueLock();
    
    std::shared_ptr<Registration::Process> app_proc;
    if (Registration::HasApplicationProcess(&app_proc)) {
        pid.SetValue(app_proc->pid);
        return 0;
    }
    return 0x20F;
}

Result ShellService::BoostSystemMemoryResourceLimit(u64 sysmem_size) {
    return ResourceLimitUtils::BoostSystemMemoryResourceLimit(sysmem_size);
}
