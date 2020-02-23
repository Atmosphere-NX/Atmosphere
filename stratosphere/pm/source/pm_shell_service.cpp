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
#include "pm_shell_service.hpp"
#include "impl/pm_process_manager.hpp"

namespace ams::pm::shell {

    /* Overrides for libstratosphere pm::shell commands. */
    Result LaunchProgram(os::ProcessId *out_process_id, const ncm::ProgramLocation &loc, u32 launch_flags) {
        return impl::LaunchProgram(out_process_id, loc, launch_flags);
    }

    /* Service command implementations. */
    Result ShellServiceBase::LaunchProgram(sf::Out<os::ProcessId> out_process_id, const ncm::ProgramLocation &loc, u32 flags) {
        return pm::shell::LaunchProgram(out_process_id.GetPointer(), loc, flags);
    }

    Result ShellServiceBase::TerminateProcess(os::ProcessId process_id) {
        return impl::TerminateProcess(process_id);
    }

    Result ShellServiceBase::TerminateProgram(ncm::ProgramId program_id) {
        return impl::TerminateProgram(program_id);
    }

    void ShellServiceBase::GetProcessEventHandle(sf::OutCopyHandle out) {
        R_ABORT_UNLESS(impl::GetProcessEventHandle(out.GetHandlePointer()));
    }

    void ShellServiceBase::GetProcessEventInfo(sf::Out<ProcessEventInfo> out) {
        R_ABORT_UNLESS(impl::GetProcessEventInfo(out.GetPointer()));
    }

    Result ShellServiceBase::CleanupProcess(os::ProcessId process_id) {
        return impl::CleanupProcess(process_id);
    }

    Result ShellServiceBase::ClearExceptionOccurred(os::ProcessId process_id) {
        return impl::ClearExceptionOccurred(process_id);
    }

    void ShellServiceBase::NotifyBootFinished() {
        R_ABORT_UNLESS(impl::NotifyBootFinished());
    }

    Result ShellServiceBase::GetApplicationProcessIdForShell(sf::Out<os::ProcessId> out) {
        return impl::GetApplicationProcessId(out.GetPointer());
    }

    Result ShellServiceBase::BoostSystemMemoryResourceLimit(u64 boost_size) {
        return impl::BoostSystemMemoryResourceLimit(boost_size);
    }

    Result ShellServiceBase::BoostApplicationThreadResourceLimit() {
        return impl::BoostApplicationThreadResourceLimit();
    }

    void ShellServiceBase::GetBootFinishedEventHandle(sf::OutCopyHandle out) {
        R_ABORT_UNLESS(impl::GetBootFinishedEventHandle(out.GetHandlePointer()));
    }

}
