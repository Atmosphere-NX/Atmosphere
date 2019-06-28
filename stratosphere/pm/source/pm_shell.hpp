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

#include "pm_registration.hpp"

/* Represents modern ShellService (5.0.0+) */
class ShellService : public IServiceObject {
    private:
        enum class CommandId {
            LaunchProcess                   = 0,
            TerminateProcessId              = 1,
            TerminateTitleId                = 2,
            GetProcessWaitEvent             = 3,
            GetProcessEventType             = 4,
            NotifyBootFinished              = 5,
            GetApplicationProcessId         = 6,
            BoostSystemMemoryResourceLimit  = 7,
            BoostSystemThreadsResourceLimit = 8,
            GetBootFinishedEvent            = 9,
        };
    protected:
        /* Actual commands. */
        Result LaunchProcess(Out<u64> pid, Registration::TidSid tid_sid, u32 launch_flags);
        Result TerminateProcessId(u64 pid);
        Result TerminateTitleId(u64 tid);
        void GetProcessWaitEvent(Out<CopiedHandle> event);
        void GetProcessEventType(Out<u64> type, Out<u64> pid);
        Result FinalizeExitedProcess(u64 pid);
        Result ClearProcessNotificationFlag(u64 pid);
        void NotifyBootFinished();
        Result GetApplicationProcessId(Out<u64> pid);
        Result BoostSystemMemoryResourceLimit(u64 sysmem_size);
        Result BoostSystemThreadsResourceLimit();
        void GetBootFinishedEvent(Out<CopiedHandle> event);
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            /* 5.0.0-* */
            MAKE_SERVICE_COMMAND_META(ShellService, LaunchProcess),
            MAKE_SERVICE_COMMAND_META(ShellService, TerminateProcessId),
            MAKE_SERVICE_COMMAND_META(ShellService, TerminateTitleId),
            MAKE_SERVICE_COMMAND_META(ShellService, GetProcessWaitEvent),
            MAKE_SERVICE_COMMAND_META(ShellService, GetProcessEventType),
            MAKE_SERVICE_COMMAND_META(ShellService, NotifyBootFinished),
            MAKE_SERVICE_COMMAND_META(ShellService, GetApplicationProcessId),
            MAKE_SERVICE_COMMAND_META(ShellService, BoostSystemMemoryResourceLimit),

            /* 7.0.0-* */
            MAKE_SERVICE_COMMAND_META(ShellService, BoostSystemThreadsResourceLimit, FirmwareVersion_700),

            /* 8.0.0-* */
            MAKE_SERVICE_COMMAND_META(ShellService, GetBootFinishedEvent,            FirmwareVersion_800),
        };
};

/* Represents deprecated ShellService (1.0.0-4.1.0). */
class ShellServiceDeprecated : public ShellService {
    private:
        enum class CommandId {
            LaunchProcess                  = 0,
            TerminateProcessId             = 1,
            TerminateTitleId               = 2,
            GetProcessWaitEvent            = 3,
            GetProcessEventType            = 4,
            FinalizeExitedProcess          = 5,
            ClearProcessNotificationFlag   = 6,
            NotifyBootFinished             = 7,
            GetApplicationProcessId        = 8,
            BoostSystemMemoryResourceLimit = 9,
        };
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            /* 1.0.0- */
            MAKE_SERVICE_COMMAND_META(ShellServiceDeprecated, LaunchProcess),
            MAKE_SERVICE_COMMAND_META(ShellServiceDeprecated, TerminateProcessId),
            MAKE_SERVICE_COMMAND_META(ShellServiceDeprecated, TerminateTitleId),
            MAKE_SERVICE_COMMAND_META(ShellServiceDeprecated, GetProcessWaitEvent),
            MAKE_SERVICE_COMMAND_META(ShellServiceDeprecated, GetProcessEventType),
            MAKE_SERVICE_COMMAND_META(ShellServiceDeprecated, FinalizeExitedProcess),
            MAKE_SERVICE_COMMAND_META(ShellServiceDeprecated, ClearProcessNotificationFlag),
            MAKE_SERVICE_COMMAND_META(ShellServiceDeprecated, NotifyBootFinished),
            MAKE_SERVICE_COMMAND_META(ShellServiceDeprecated, GetApplicationProcessId),

            /* 4.0.0- */
            MAKE_SERVICE_COMMAND_META(ShellServiceDeprecated, BoostSystemMemoryResourceLimit, FirmwareVersion_400),
        };
};
