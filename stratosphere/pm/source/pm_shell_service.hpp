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

namespace ams::pm::shell {

    class ShellServiceBase : public sf::IServiceObject {
        protected:
            /* Actual command implementations. */
            virtual Result LaunchProgram(sf::Out<os::ProcessId> out_process_id, const ncm::ProgramLocation &loc, u32 flags);
            virtual Result TerminateProcess(os::ProcessId process_id);
            virtual Result TerminateProgram(ncm::ProgramId program_id);
            virtual void   GetProcessEventHandle(sf::OutCopyHandle out);
            virtual void   GetProcessEventInfo(sf::Out<ProcessEventInfo> out);
            virtual Result CleanupProcess(os::ProcessId process_id);
            virtual Result ClearExceptionOccurred(os::ProcessId process_id);
            virtual void   NotifyBootFinished();
            virtual Result GetApplicationProcessIdForShell(sf::Out<os::ProcessId> out);
            virtual Result BoostSystemMemoryResourceLimit(u64 boost_size);
            virtual Result BoostApplicationThreadResourceLimit();
            virtual void   GetBootFinishedEventHandle(sf::OutCopyHandle out);
    };

    /* This represents modern ShellService (5.0.0+). */
    class ShellService final : public ShellServiceBase {
        private:
            enum class CommandId {
                LaunchProgram                         = 0,
                TerminateProcess                    = 1,
                TerminateProgram                      = 2,
                GetProcessEventHandle               = 3,
                GetProcessEventInfo                 = 4,
                NotifyBootFinished                  = 5,
                GetApplicationProcessIdForShell     = 6,
                BoostSystemMemoryResourceLimit      = 7,
                BoostApplicationThreadResourceLimit = 8,
                GetBootFinishedEventHandle          = 9,
            };
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                /* 5.0.0-* */
                MAKE_SERVICE_COMMAND_META(LaunchProgram),
                MAKE_SERVICE_COMMAND_META(TerminateProcess),
                MAKE_SERVICE_COMMAND_META(TerminateProgram),
                MAKE_SERVICE_COMMAND_META(GetProcessEventHandle),
                MAKE_SERVICE_COMMAND_META(GetProcessEventInfo),
                MAKE_SERVICE_COMMAND_META(NotifyBootFinished),
                MAKE_SERVICE_COMMAND_META(GetApplicationProcessIdForShell),
                MAKE_SERVICE_COMMAND_META(BoostSystemMemoryResourceLimit),

                /* 7.0.0-* */
                MAKE_SERVICE_COMMAND_META(BoostApplicationThreadResourceLimit, hos::Version_7_0_0),

                /* 8.0.0-* */
                MAKE_SERVICE_COMMAND_META(GetBootFinishedEventHandle,          hos::Version_8_0_0),
            };
    };

    /* This represents deprecated ShellService (1.0.0-4.1.0). */
    class ShellServiceDeprecated final : public ShellServiceBase {
        private:
            enum class CommandId {
                LaunchProgram                     = 0,
                TerminateProcess                = 1,
                TerminateProgram                  = 2,
                GetProcessEventHandle           = 3,
                GetProcessEventInfo             = 4,
                CleanupProcess                  = 5,
                ClearExceptionOccurred          = 6,
                NotifyBootFinished              = 7,
                GetApplicationProcessIdForShell = 8,
                BoostSystemMemoryResourceLimit  = 9,
            };
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                /* 1.0.0-4.1.0 */
                MAKE_SERVICE_COMMAND_META(LaunchProgram),
                MAKE_SERVICE_COMMAND_META(TerminateProcess),
                MAKE_SERVICE_COMMAND_META(TerminateProgram),
                MAKE_SERVICE_COMMAND_META(GetProcessEventHandle),
                MAKE_SERVICE_COMMAND_META(GetProcessEventInfo),
                MAKE_SERVICE_COMMAND_META(CleanupProcess),
                MAKE_SERVICE_COMMAND_META(ClearExceptionOccurred),
                MAKE_SERVICE_COMMAND_META(NotifyBootFinished),
                MAKE_SERVICE_COMMAND_META(GetApplicationProcessIdForShell),

                /* 4.0.0-4.1.0 */
                MAKE_SERVICE_COMMAND_META(BoostSystemMemoryResourceLimit, hos::Version_4_0_0),
            };
    };

}
