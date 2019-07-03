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

#include "pm_shell_service.hpp"
#include "impl/pm_process_manager.hpp"

namespace sts::pm::shell {

    /* Overrides for libstratosphere pm::shell commands. */
    Result LaunchTitle(u64 *out_process_id, const ncm::TitleLocation &loc, u32 launch_flags) {
        return impl::LaunchTitle(out_process_id, loc, launch_flags);
    }

    /* Service command implementations. */
    Result ShellServiceBase::LaunchTitle(Out<u64> out_process_id, ncm::TitleLocation loc, u32 flags) {
        return pm::shell::LaunchTitle(out_process_id.GetPointer(), loc, flags);
    }

    Result ShellServiceBase::TerminateProcess(u64 process_id) {
        return impl::TerminateProcess(process_id);
    }

    Result ShellServiceBase::TerminateTitle(ncm::TitleId title_id) {
        return impl::TerminateTitle(title_id);
    }

    void ShellServiceBase::GetProcessEventHandle(Out<CopiedHandle> out) {
        R_ASSERT(impl::GetProcessEventHandle(out.GetHandlePointer()));
    }

    void ShellServiceBase::GetProcessEventInfo(Out<ProcessEventInfo> out) {
        R_ASSERT(impl::GetProcessEventInfo(out.GetPointer()));
    }

    Result ShellServiceBase::CleanupProcess(u64 process_id) {
        return impl::CleanupProcess(process_id);
    }

    Result ShellServiceBase::ClearExceptionOccurred(u64 process_id) {
        return impl::ClearExceptionOccurred(process_id);
    }

    void ShellServiceBase::NotifyBootFinished() {
        R_ASSERT(impl::NotifyBootFinished());
    }

    Result ShellServiceBase::GetApplicationProcessIdForShell(Out<u64> out) {
        return impl::GetApplicationProcessId(out.GetPointer());
    }

    Result ShellServiceBase::BoostSystemMemoryResourceLimit(u64 boost_size) {
        return impl::BoostSystemMemoryResourceLimit(boost_size);
    }

    Result ShellServiceBase::BoostApplicationThreadResourceLimit() {
        return impl::BoostApplicationThreadResourceLimit();
    }

    void ShellServiceBase::GetBootFinishedEventHandle(Out<CopiedHandle> out) {
        R_ASSERT(impl::GetBootFinishedEventHandle(out.GetHandlePointer()));
    }

}
