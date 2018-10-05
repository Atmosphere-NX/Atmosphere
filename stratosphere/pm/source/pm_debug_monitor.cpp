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

Result DebugMonitorService::dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size) {
    Result rc = 0xF601;
    if (kernelAbove500()) {
        switch ((DmntCmd_5X)cmd_id) {
            case Dmnt_Cmd_5X_GetDebugProcessIds:
                rc = WrapIpcCommandImpl<&DebugMonitorService::get_debug_process_ids>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                break;
            case Dmnt_Cmd_5X_LaunchDebugProcess:
                rc = WrapIpcCommandImpl<&DebugMonitorService::launch_debug_process>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                break;
            case Dmnt_Cmd_5X_GetTitleProcessId:
                rc = WrapIpcCommandImpl<&DebugMonitorService::get_title_process_id>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                break;
            case Dmnt_Cmd_5X_EnableDebugForTitleId:
                rc = WrapIpcCommandImpl<&DebugMonitorService::enable_debug_for_tid>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                break;
            case Dmnt_Cmd_5X_GetApplicationProcessId:
                rc = WrapIpcCommandImpl<&DebugMonitorService::get_application_process_id>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                break;
            case Dmnt_Cmd_5X_EnableDebugForApplication:
                rc = WrapIpcCommandImpl<&DebugMonitorService::enable_debug_for_application>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                break;
            case Dmnt_Cmd_6X_DisableDebug:
                if (kernelAbove600()) {
                    rc = WrapIpcCommandImpl<&DebugMonitorService::disable_debug>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                }
                break;
            case Dmnt_Cmd_5X_AtmosphereGetProcessHandle:
                rc = WrapIpcCommandImpl<&DebugMonitorService::get_process_handle>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                break;
            case Dmnt_Cmd_5X_AtmosphereGetCurrentLimitInfo:
                rc = WrapIpcCommandImpl<&DebugMonitorService::get_current_limit_info>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                break;
            default:
                break;
        }
    } else {
        switch ((DmntCmd)cmd_id) {
            case Dmnt_Cmd_GetUnknownStub:
                rc = WrapIpcCommandImpl<&DebugMonitorService::get_unknown_stub>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                break;
            case Dmnt_Cmd_GetDebugProcessIds:
                rc = WrapIpcCommandImpl<&DebugMonitorService::get_debug_process_ids>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                break;
            case Dmnt_Cmd_LaunchDebugProcess:
                rc = WrapIpcCommandImpl<&DebugMonitorService::launch_debug_process>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                break;
            case Dmnt_Cmd_GetTitleProcessId:
                rc = WrapIpcCommandImpl<&DebugMonitorService::get_title_process_id>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                break;
            case Dmnt_Cmd_EnableDebugForTitleId:
                rc = WrapIpcCommandImpl<&DebugMonitorService::enable_debug_for_tid>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                break;
            case Dmnt_Cmd_GetApplicationProcessId:
                rc = WrapIpcCommandImpl<&DebugMonitorService::get_application_process_id>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                break;
            case Dmnt_Cmd_EnableDebugForApplication:
                rc = WrapIpcCommandImpl<&DebugMonitorService::enable_debug_for_application>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                break;
            case Dmnt_Cmd_AtmosphereGetProcessHandle:
                rc = WrapIpcCommandImpl<&DebugMonitorService::get_process_handle>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                break;
            case Dmnt_Cmd_AtmosphereGetCurrentLimitInfo:
                rc = WrapIpcCommandImpl<&DebugMonitorService::get_current_limit_info>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                break;
            default:
                break;
        }
    }
    return rc;
}

Result DebugMonitorService::handle_deferred() {
    /* This service is never deferrable. */
    return 0;
}


std::tuple<Result, u32> DebugMonitorService::get_unknown_stub(u64 unknown, OutBuffer<u8> out_unknown) {
    /* This command seem stubbed on retail. */
    if (out_unknown.num_elements >> 31) {
        return {0xC0F, 0};
    }
    return {0x0, 0};
}

std::tuple<Result, u32> DebugMonitorService::get_debug_process_ids(OutBuffer<u64> out_pids) {
    u32 num_out = 0;
    Result rc;
    if (out_pids.num_elements >> 31) {
        return {0xC0F, 0};
    }
    rc = Registration::GetDebugProcessIds(out_pids.buffer, out_pids.num_elements, &num_out);
    return {rc, num_out};
}

std::tuple<Result> DebugMonitorService::launch_debug_process(u64 pid) {
    return {Registration::LaunchDebugProcess(pid)};
}

std::tuple<Result, u64> DebugMonitorService::get_title_process_id(u64 tid) {
    auto auto_lock = Registration::GetProcessListUniqueLock();
    
    std::shared_ptr<Registration::Process> proc = Registration::GetProcessByTitleId(tid);
    if (proc != nullptr) {
        return {0, proc->pid};
    } else {
        return {0x20F, 0};
    }
}

std::tuple<Result, CopiedHandle> DebugMonitorService::enable_debug_for_tid(u64 tid) {
    Handle h = 0;
    Result rc = Registration::EnableDebugForTitleId(tid, &h);
    return {rc, h};
}

std::tuple<Result, u64> DebugMonitorService::get_application_process_id() {
    auto auto_lock = Registration::GetProcessListUniqueLock();
    
    std::shared_ptr<Registration::Process> app_proc;
    if (Registration::HasApplicationProcess(&app_proc)) {
        return {0, app_proc->pid};
    }
    return {0x20F, 0};
}

std::tuple<Result, CopiedHandle> DebugMonitorService::enable_debug_for_application() {
    Handle h = 0;
    Result rc = Registration::EnableDebugForApplication(&h);
    return {rc, h};
}


std::tuple<Result> DebugMonitorService::disable_debug(u32 which) {
    return {Registration::DisableDebug(which)};
}

std::tuple<Result, CopiedHandle> DebugMonitorService::get_process_handle(u64 pid) {
    std::shared_ptr<Registration::Process> proc = Registration::GetProcess(pid);
    if(proc == NULL) {
        return {0x20F, 0};
    }
    return {0, proc->handle};
}

std::tuple<Result, u64, u64> DebugMonitorService::get_current_limit_info(u32 category, u32 resource) {
    if(category > ResourceLimitUtils::ResourceLimitCategory::ResourceLimitCategory_Applet) {
        return {0xf001, 0, 0};
    }

    Handle limit_h = ResourceLimitUtils::GetResourceLimitHandleByCategory((ResourceLimitUtils::ResourceLimitCategory) category);

    uint64_t current_value, limit_value;

    Result r;

    r = svcGetResourceLimitCurrentValue(&current_value, limit_h, (LimitableResource) resource);
    if(R_FAILED(r)) {
        return {r, 0, 0};
    }

    r = svcGetResourceLimitLimitValue(&limit_value, limit_h, (LimitableResource) resource);
    if(R_FAILED(r)) {
        return {r, 0, 0};
    }

    return {0, current_value, limit_value};
}
