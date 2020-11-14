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
#pragma once
#include <stratosphere.hpp>

namespace ams::pm {

    class ShellService final {
        public:
            /* Actual command implementations. */
            Result LaunchProgram(sf::Out<os::ProcessId> out_process_id, const ncm::ProgramLocation &loc, u32 flags);
            Result TerminateProcess(os::ProcessId process_id);
            Result TerminateProgram(ncm::ProgramId program_id);
            void   GetProcessEventHandle(sf::OutCopyHandle out);
            void   GetProcessEventInfo(sf::Out<ProcessEventInfo> out);
            Result CleanupProcess(os::ProcessId process_id);
            Result ClearExceptionOccurred(os::ProcessId process_id);
            void   NotifyBootFinished();
            Result GetApplicationProcessIdForShell(sf::Out<os::ProcessId> out);
            Result BoostSystemMemoryResourceLimit(u64 boost_size);
            Result BoostApplicationThreadResourceLimit();
            void   GetBootFinishedEventHandle(sf::OutCopyHandle out);
    };
    static_assert(pm::impl::IsIShellInterface<ShellService>);

}
