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
#include "pm_debug_monitor_service.hpp"
#include "impl/pm_process_manager.hpp"

namespace ams::pm {

    /* Actual command implementations. */
    Result DebugMonitorService::GetModuleIdList(sf::Out<u32> out_count, const sf::OutBuffer &out_buf, u64 unused) {
        R_UNLESS(out_buf.GetSize() <= std::numeric_limits<s32>::max(), pm::ResultInvalidSize());
        return impl::GetModuleIdList(out_count.GetPointer(), out_buf.GetPointer(), out_buf.GetSize(), unused);
    }

    Result DebugMonitorService::GetExceptionProcessIdList(sf::Out<u32> out_count, const sf::OutArray<os::ProcessId> &out_process_ids) {
        R_UNLESS(out_process_ids.GetSize() <= std::numeric_limits<s32>::max(), pm::ResultInvalidSize());
        return impl::GetExceptionProcessIdList(out_count.GetPointer(), out_process_ids.GetPointer(), out_process_ids.GetSize());
    }

    Result DebugMonitorService::StartProcess(os::ProcessId process_id) {
        return impl::StartProcess(process_id);
    }

    Result DebugMonitorService::GetProcessId(sf::Out<os::ProcessId> out, ncm::ProgramId program_id) {
        return impl::GetProcessId(out.GetPointer(), program_id);
    }

    Result DebugMonitorService::HookToCreateProcess(sf::OutCopyHandle out_hook, ncm::ProgramId program_id) {
        return impl::HookToCreateProcess(out_hook.GetHandlePointer(), program_id);
    }

    Result DebugMonitorService::GetApplicationProcessId(sf::Out<os::ProcessId> out) {
        return impl::GetApplicationProcessId(out.GetPointer());
    }

    Result DebugMonitorService::HookToCreateApplicationProcess(sf::OutCopyHandle out_hook) {
        return impl::HookToCreateApplicationProcess(out_hook.GetHandlePointer());
    }

    Result DebugMonitorService::ClearHook(u32 which) {
        return impl::ClearHook(which);
    }

    /* Atmosphere extension commands. */
    Result DebugMonitorService::AtmosphereGetProcessInfo(sf::OutCopyHandle out_process_handle, sf::Out<ncm::ProgramLocation> out_loc, sf::Out<cfg::OverrideStatus> out_status, os::ProcessId process_id) {
        return impl::AtmosphereGetProcessInfo(out_process_handle.GetHandlePointer(), out_loc.GetPointer(), out_status.GetPointer(), process_id);
    }

    Result DebugMonitorService::AtmosphereGetCurrentLimitInfo(sf::Out<s64> out_cur_val, sf::Out<s64> out_lim_val, u32 group, u32 resource) {
        return impl::AtmosphereGetCurrentLimitInfo(out_cur_val.GetPointer(), out_lim_val.GetPointer(), group, resource);
    }

}
