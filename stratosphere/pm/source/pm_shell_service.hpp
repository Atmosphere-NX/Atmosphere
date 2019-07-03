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

#pragma once
#include <switch.h>
#include <stratosphere.hpp>
#include <stratosphere/ldr.hpp>
#include <stratosphere/pm.hpp>

namespace sts::pm::shell {

    class ShellServiceBase : public IServiceObject {
        protected:
            /* Actual command implementations. */
            virtual Result LaunchTitle(Out<u64> out_process_id, ncm::TitleLocation loc, u32 flags);
            virtual Result TerminateProcess(u64 process_id);
            virtual Result TerminateTitle(ncm::TitleId title_id);
            virtual void   GetProcessEventHandle(Out<CopiedHandle> out);
            virtual void   GetProcessEventInfo(Out<ProcessEventInfo> out);
            virtual Result CleanupProcess(u64 process_id);
            virtual Result ClearExceptionOccurred(u64 process_id);
            virtual void   NotifyBootFinished();
            virtual Result GetApplicationProcessIdForShell(Out<u64> out);
            virtual Result BoostSystemMemoryResourceLimit(u64 boost_size);
            virtual Result BoostApplicationThreadResourceLimit();
            virtual void   GetBootFinishedEventHandle(Out<CopiedHandle> out);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                /* No entries, because ShellServiceBase is abstract. */
            };
    };

    /* This represents modern ShellService (5.0.0+). */
    class ShellService final : public ShellServiceBase {
        private:
            enum class CommandId {
                LaunchTitle                         = 0,
                TerminateProcess                    = 1,
                TerminateTitle                      = 2,
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
                MAKE_SERVICE_COMMAND_META(ShellService, LaunchTitle),
                MAKE_SERVICE_COMMAND_META(ShellService, TerminateProcess),
                MAKE_SERVICE_COMMAND_META(ShellService, TerminateTitle),
                MAKE_SERVICE_COMMAND_META(ShellService, GetProcessEventHandle),
                MAKE_SERVICE_COMMAND_META(ShellService, GetProcessEventInfo),
                MAKE_SERVICE_COMMAND_META(ShellService, NotifyBootFinished),
                MAKE_SERVICE_COMMAND_META(ShellService, GetApplicationProcessIdForShell),
                MAKE_SERVICE_COMMAND_META(ShellService, BoostSystemMemoryResourceLimit),

                /* 7.0.0-* */
                MAKE_SERVICE_COMMAND_META(ShellService, BoostApplicationThreadResourceLimit, FirmwareVersion_700),

                /* 8.0.0-* */
                MAKE_SERVICE_COMMAND_META(ShellService, GetBootFinishedEventHandle,     FirmwareVersion_800),
            };
    };

    /* This represents deprecated ShellService (1.0.0-4.1.0). */
    class ShellServiceDeprecated final : public ShellServiceBase {
        private:
            enum class CommandId {
                LaunchTitle                     = 0,
                TerminateProcess                = 1,
                TerminateTitle                  = 2,
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
                MAKE_SERVICE_COMMAND_META(ShellServiceDeprecated, LaunchTitle),
                MAKE_SERVICE_COMMAND_META(ShellServiceDeprecated, TerminateProcess),
                MAKE_SERVICE_COMMAND_META(ShellServiceDeprecated, TerminateTitle),
                MAKE_SERVICE_COMMAND_META(ShellServiceDeprecated, GetProcessEventHandle),
                MAKE_SERVICE_COMMAND_META(ShellServiceDeprecated, GetProcessEventInfo),
                MAKE_SERVICE_COMMAND_META(ShellServiceDeprecated, CleanupProcess),
                MAKE_SERVICE_COMMAND_META(ShellServiceDeprecated, ClearExceptionOccurred),
                MAKE_SERVICE_COMMAND_META(ShellServiceDeprecated, NotifyBootFinished),
                MAKE_SERVICE_COMMAND_META(ShellServiceDeprecated, GetApplicationProcessIdForShell),

                /* 4.0.0-4.1.0 */
                MAKE_SERVICE_COMMAND_META(ShellServiceDeprecated, BoostSystemMemoryResourceLimit),
            };
    };

}
