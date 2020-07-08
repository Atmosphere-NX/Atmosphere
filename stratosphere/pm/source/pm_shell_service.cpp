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
#include "pm_shell_service.hpp"
#include "impl/pm_process_manager.hpp"

namespace ams::pm {

    /* Overrides for libstratosphere pm::shell commands. */
    namespace shell {

        Result LaunchProgram(os::ProcessId *out_process_id, const ncm::ProgramLocation &loc, u32 launch_flags) {
            return impl::LaunchProgram(out_process_id, loc, launch_flags);
        }

    }

    /* Service command implementations. */
    Result ShellService::LaunchProgram(sf::Out<os::ProcessId> out_process_id, const ncm::ProgramLocation &loc, u32 flags) {
        return pm::shell::LaunchProgram(out_process_id.GetPointer(), loc, flags);
    }

    Result ShellService::TerminateProcess(os::ProcessId process_id) {
        return impl::TerminateProcess(process_id);
    }

    Result ShellService::TerminateProgram(ncm::ProgramId program_id) {
        return impl::TerminateProgram(program_id);
    }

    void ShellService::GetProcessEventHandle(sf::OutCopyHandle out) {
        R_ABORT_UNLESS(impl::GetProcessEventHandle(out.GetHandlePointer()));
    }

    void ShellService::GetProcessEventInfo(sf::Out<ProcessEventInfo> out) {
        R_ABORT_UNLESS(impl::GetProcessEventInfo(out.GetPointer()));
    }

    Result ShellService::CleanupProcess(os::ProcessId process_id) {
        return impl::CleanupProcess(process_id);
    }

    Result ShellService::ClearExceptionOccurred(os::ProcessId process_id) {
        return impl::ClearExceptionOccurred(process_id);
    }

    void ShellService::NotifyBootFinished() {
        R_ABORT_UNLESS(impl::NotifyBootFinished());
    }

    Result ShellService::GetApplicationProcessIdForShell(sf::Out<os::ProcessId> out) {
        return impl::GetApplicationProcessId(out.GetPointer());
    }

    Result ShellService::BoostSystemMemoryResourceLimit(u64 boost_size) {
        return impl::BoostSystemMemoryResourceLimit(boost_size);
    }

    Result ShellService::BoostApplicationThreadResourceLimit() {
        return impl::BoostApplicationThreadResourceLimit();
    }

    void ShellService::GetBootFinishedEventHandle(sf::OutCopyHandle out) {
        R_ABORT_UNLESS(impl::GetBootFinishedEventHandle(out.GetHandlePointer()));
    }

}
