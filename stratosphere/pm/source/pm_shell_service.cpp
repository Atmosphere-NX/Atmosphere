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
#include <stratosphere.hpp>
#include "pm_shell_service.hpp"
#include "impl/pm_process_manager.hpp"

namespace ams::pm {

    /* Overrides for libstratosphere pm::shell commands. */
    namespace shell {

        Result LaunchProgram(os::ProcessId *out_process_id, const ncm::ProgramLocation &loc, u32 launch_flags) {
            R_RETURN(impl::LaunchProgram(out_process_id, loc, launch_flags));
        }

    }

    /* Service command implementations. */
    Result ShellService::LaunchProgram(sf::Out<os::ProcessId> out_process_id, const ncm::ProgramLocation &loc, u32 flags) {
        R_RETURN(pm::shell::LaunchProgram(out_process_id.GetPointer(), loc, flags));
    }

    Result ShellService::TerminateProcess(os::ProcessId process_id) {
        R_RETURN(impl::TerminateProcess(process_id));
    }

    Result ShellService::TerminateProgram(ncm::ProgramId program_id) {
        R_RETURN(impl::TerminateProgram(program_id));
    }

    void ShellService::GetProcessEventHandle(sf::OutCopyHandle out) {
        os::NativeHandle event_handle;
        R_ABORT_UNLESS(impl::GetProcessEventHandle(std::addressof(event_handle)));

        out.SetValue(event_handle, false);
    }

    void ShellService::GetProcessEventInfo(sf::Out<ProcessEventInfo> out) {
        R_ABORT_UNLESS(impl::GetProcessEventInfo(out.GetPointer()));
    }

    Result ShellService::CleanupProcess(os::ProcessId process_id) {
        R_RETURN(impl::CleanupProcess(process_id));
    }

    Result ShellService::ClearExceptionOccurred(os::ProcessId process_id) {
        R_RETURN(impl::ClearExceptionOccurred(process_id));
    }

    void ShellService::NotifyBootFinished() {
        R_ABORT_UNLESS(impl::NotifyBootFinished());
    }

    Result ShellService::GetApplicationProcessIdForShell(sf::Out<os::ProcessId> out) {
        R_RETURN(impl::GetApplicationProcessId(out.GetPointer()));
    }

    Result ShellService::BoostSystemMemoryResourceLimit(u64 boost_size) {
        R_RETURN(impl::BoostSystemMemoryResourceLimit(boost_size));
    }

    Result ShellService::BoostApplicationThreadResourceLimit() {
        R_RETURN(impl::BoostApplicationThreadResourceLimit());
    }

    void ShellService::GetBootFinishedEventHandle(sf::OutCopyHandle out) {
        os::NativeHandle event_handle;
        R_ABORT_UNLESS(impl::GetBootFinishedEventHandle(std::addressof(event_handle)));

        out.SetValue(event_handle, false);
    }

    Result ShellService::BoostSystemThreadResourceLimit() {
        R_RETURN(impl::BoostSystemThreadResourceLimit());
    }

}
