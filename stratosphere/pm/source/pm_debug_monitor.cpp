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
#include "pm_debug_monitor.hpp"


Result DebugMonitorService::GetUnknownStub(Out<u32> count, OutBuffer<u8> out_buf, u64 in_unk) {
    /* This command seems stubbed. */
    if (out_buf.num_elements >> 31) {
        return 0xC0F;
    }
    count.SetValue(0);
    return 0x0;
}

Result DebugMonitorService::GetDebugProcessIds(Out<u32> count, OutBuffer<u64> out_pids) {
    if (out_pids.num_elements >> 31) {
        return 0xC0F;
    }
    return Registration::GetDebugProcessIds(out_pids.buffer, out_pids.num_elements, count.GetPointer());
}

Result DebugMonitorService::LaunchDebugProcess(u64 pid) {
    return Registration::LaunchDebugProcess(pid);
}

Result DebugMonitorService::GetTitleProcessId(Out<u64> pid, u64 tid) {
    auto auto_lock = Registration::GetProcessListUniqueLock();
    
    std::shared_ptr<Registration::Process> proc = Registration::GetProcessByTitleId(tid);
    if (proc != nullptr) {
        pid.SetValue(proc->pid);
        return 0;
    }
    return 0x20F;
}

Result DebugMonitorService::EnableDebugForTitleId(Out<CopiedHandle> event, u64 tid) {
    return Registration::EnableDebugForTitleId(tid, event.GetHandlePointer());
}

Result DebugMonitorService::GetApplicationProcessId(Out<u64> pid) {
    auto auto_lock = Registration::GetProcessListUniqueLock();
    
    std::shared_ptr<Registration::Process> app_proc;
    if (Registration::HasApplicationProcess(&app_proc)) {
        pid.SetValue(app_proc->pid);
        return 0x0;
    }
    return 0x20F;
}

Result DebugMonitorService::EnableDebugForApplication(Out<CopiedHandle> event) {
    return Registration::EnableDebugForApplication(event.GetHandlePointer());
}


Result DebugMonitorService::DisableDebug(u32 which) {
    return Registration::DisableDebug(which);
}

Result DebugMonitorService::AtmosphereGetProcessHandle(Out<CopiedHandle> proc_hand, u64 pid) {
    auto proc = Registration::GetProcess(pid);
    if(proc != nullptr) {
        proc_hand.SetValue(proc->handle);
        return 0;
    }
    return 0x20F;
}

Result DebugMonitorService::AtmosphereGetCurrentLimitInfo(Out<u64> cur_val, Out<u64> lim_val, u32 category, u32 resource) {
    Result rc;
    if(category > ResourceLimitUtils::ResourceLimitCategory::ResourceLimitCategory_Applet) {
        return 0xF001;
    }

    Handle limit_h = ResourceLimitUtils::GetResourceLimitHandleByCategory((ResourceLimitUtils::ResourceLimitCategory) category);

    rc = svcGetResourceLimitCurrentValue(cur_val.GetPointer(), limit_h, (LimitableResource) resource);
    if(R_FAILED(rc)) {
        return rc;
    }

    rc = svcGetResourceLimitLimitValue(lim_val.GetPointer(), limit_h, (LimitableResource) resource);
    if(R_FAILED(rc)) {
        return rc;
    }

    return 0;
}
