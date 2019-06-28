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

/* Represents modern DebugMonitorService (5.0.0+) */
class DebugMonitorService : public IServiceObject {
    private:
        enum class CommandId {
            GetDebugProcessIds            = 0,
            LaunchDebugProcess            = 1,
            GetTitleProcessId             = 2,
            EnableDebugForTitleId         = 3,
            GetApplicationProcessId       = 4,
            EnableDebugForApplication     = 5,

            DisableDebug                  = 6,

            AtmosphereGetProcessInfo      = 65000,
            AtmosphereGetCurrentLimitInfo = 65001,
        };
    protected:
        /* Actual commands. */
        Result GetUnknownStub(Out<u32> count, OutBuffer<u8> out_buf, u64 in_unk);
        Result GetDebugProcessIds(Out<u32> count, OutBuffer<u64> out_pids);
        Result LaunchDebugProcess(u64 pid);
        Result GetTitleProcessId(Out<u64> pid, u64 tid);
        Result EnableDebugForTitleId(Out<CopiedHandle> event, u64 tid);
        Result GetApplicationProcessId(Out<u64> pid);
        Result EnableDebugForApplication(Out<CopiedHandle> event);
        Result DisableDebug(u32 which);

        /* Atmosphere commands. */
        Result AtmosphereGetProcessInfo(Out<CopiedHandle> proc_hand, Out<Registration::TidSid> tid_sid, u64 pid);
        Result AtmosphereGetCurrentLimitInfo(Out<u64> cur_val, Out<u64> lim_val, u32 category, u32 resource);
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            /* 5.0.0-* */
            MAKE_SERVICE_COMMAND_META(DebugMonitorService, GetDebugProcessIds),
            MAKE_SERVICE_COMMAND_META(DebugMonitorService, LaunchDebugProcess),
            MAKE_SERVICE_COMMAND_META(DebugMonitorService, GetTitleProcessId),
            MAKE_SERVICE_COMMAND_META(DebugMonitorService, EnableDebugForTitleId),
            MAKE_SERVICE_COMMAND_META(DebugMonitorService, GetApplicationProcessId),
            MAKE_SERVICE_COMMAND_META(DebugMonitorService, EnableDebugForApplication),

            /* 6.0.0-* */
            MAKE_SERVICE_COMMAND_META(DebugMonitorService, DisableDebug, FirmwareVersion_600),

            /* Atmosphere extensions. */
            MAKE_SERVICE_COMMAND_META(DebugMonitorService, AtmosphereGetProcessInfo),
            MAKE_SERVICE_COMMAND_META(DebugMonitorService, AtmosphereGetCurrentLimitInfo),
        };
};

/* Represents deprecated DebugMonitorService (1.0.0-4.1.0). */
class DebugMonitorServiceDeprecated : public DebugMonitorService {
    private:
        enum class CommandId {
            GetUnknownStub                = 0,
            GetDebugProcessIds            = 1,
            LaunchDebugProcess            = 2,
            GetTitleProcessId             = 3,
            EnableDebugForTitleId         = 4,
            GetApplicationProcessId       = 5,
            EnableDebugForApplication     = 6,

            AtmosphereGetProcessInfo      = 65000,
            AtmosphereGetCurrentLimitInfo = 65001,
        };
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            /* 1.0.0-4.1.0 */
            MAKE_SERVICE_COMMAND_META(DebugMonitorServiceDeprecated, GetUnknownStub),
            MAKE_SERVICE_COMMAND_META(DebugMonitorServiceDeprecated, GetDebugProcessIds),
            MAKE_SERVICE_COMMAND_META(DebugMonitorServiceDeprecated, LaunchDebugProcess),
            MAKE_SERVICE_COMMAND_META(DebugMonitorServiceDeprecated, GetTitleProcessId),
            MAKE_SERVICE_COMMAND_META(DebugMonitorServiceDeprecated, EnableDebugForTitleId),
            MAKE_SERVICE_COMMAND_META(DebugMonitorServiceDeprecated, GetApplicationProcessId),
            MAKE_SERVICE_COMMAND_META(DebugMonitorServiceDeprecated, EnableDebugForApplication),

            /* Atmosphere extensions. */
            MAKE_SERVICE_COMMAND_META(DebugMonitorServiceDeprecated, AtmosphereGetProcessInfo),
            MAKE_SERVICE_COMMAND_META(DebugMonitorServiceDeprecated, AtmosphereGetCurrentLimitInfo),
        };
};
