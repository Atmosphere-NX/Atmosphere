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

#include <limits>
#include "pm_debug_monitor_service.hpp"
#include "impl/pm_process_manager.hpp"

namespace sts::pm::dmnt {

    /* Actual command implementations. */
    Result DebugMonitorServiceBase::GetModuleIdList(Out<u32> out_count, OutBuffer<u8> out_buf, u64 unused) {
        if (out_buf.num_elements > std::numeric_limits<s32>::max()) {
            return ResultPmInvalidSize;
        }
        return impl::GetModuleIdList(out_count.GetPointer(), out_buf.buffer, out_buf.num_elements,  unused);
    }

    Result DebugMonitorServiceBase::GetExceptionProcessIdList(Out<u32> out_count, OutBuffer<u64> out_process_ids) {
        if (out_process_ids.num_elements > std::numeric_limits<s32>::max()) {
            return ResultPmInvalidSize;
        }
        return impl::GetExceptionProcessIdList(out_count.GetPointer(), out_process_ids.buffer, out_process_ids.num_elements);
    }

    Result DebugMonitorServiceBase::StartProcess(u64 process_id) {
        return impl::StartProcess(process_id);
    }

    Result DebugMonitorServiceBase::GetProcessId(Out<u64> out, ncm::TitleId title_id) {
        return impl::GetProcessId(out.GetPointer(), title_id);
    }

    Result DebugMonitorServiceBase::HookToCreateProcess(Out<CopiedHandle> out_hook, ncm::TitleId title_id) {
        return impl::HookToCreateProcess(out_hook.GetHandlePointer(), title_id);
    }

    Result DebugMonitorServiceBase::GetApplicationProcessId(Out<u64> out) {
        return impl::GetApplicationProcessId(out.GetPointer());
    }

    Result DebugMonitorServiceBase::HookToCreateApplicationProcess(Out<CopiedHandle> out_hook) {
        return impl::HookToCreateApplicationProcess(out_hook.GetHandlePointer());
    }

    Result DebugMonitorServiceBase::ClearHook(u32 which) {
        return impl::ClearHook(which);
    }

    /* Atmosphere extension commands. */
    Result DebugMonitorServiceBase::AtmosphereGetProcessInfo(Out<CopiedHandle> out_process_handle, Out<ncm::TitleLocation> out_loc, u64 process_id) {
        return impl::AtmosphereGetProcessInfo(out_process_handle.GetHandlePointer(), out_loc.GetPointer(), process_id);
    }

    Result DebugMonitorServiceBase::AtmosphereGetCurrentLimitInfo(Out<u64> out_cur_val, Out<u64> out_lim_val, u32 group, u32 resource) {
        return impl::AtmosphereGetCurrentLimitInfo(out_cur_val.GetPointer(), out_lim_val.GetPointer(), group, resource);
    }

}
