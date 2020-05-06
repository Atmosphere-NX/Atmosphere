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
#include "../os/os_common_types.hpp"

namespace ams::pm {

    enum class BootMode {
        Normal      = 0,
        Maintenance = 1,
        SafeMode    = 2,
    };

    enum ResourceLimitGroup {
        ResourceLimitGroup_System      = 0,
        ResourceLimitGroup_Application = 1,
        ResourceLimitGroup_Applet      = 2,
        ResourceLimitGroup_Count,
    };

    enum LaunchFlags : u32 {
        LaunchFlags_None                = 0,
        LaunchFlags_SignalOnExit        = (1 << 0),
        LaunchFlags_SignalOnStart       = (1 << 1),
        LaunchFlags_SignalOnException   = (1 << 2),
        LaunchFlags_SignalOnDebugEvent  = (1 << 3),
        LaunchFlags_StartSuspended      = (1 << 4),
        LaunchFlags_DisableAslr         = (1 << 5),
    };

    enum LaunchFlagsDeprecated : u32 {
        LaunchFlagsDeprecated_None                = 0,
        LaunchFlagsDeprecated_SignalOnExit        = (1 << 0),
        LaunchFlagsDeprecated_StartSuspended      = (1 << 1),
        LaunchFlagsDeprecated_SignalOnException   = (1 << 2),
        LaunchFlagsDeprecated_DisableAslr         = (1 << 3),
        LaunchFlagsDeprecated_SignalOnDebugEvent  = (1 << 4),
        LaunchFlagsDeprecated_SignalOnStart       = (1 << 5),
    };

    constexpr inline u32 LaunchFlagsMask = (1 << 6) - 1;

    enum class ProcessEvent : u32 {
        None           = 0,
        Exited         = 1,
        Started        = 2,
        Exception      = 3,
        DebugRunning   = 4,
        DebugBreak     = 5,
    };

    enum class ProcessEventDeprecated : u32 {
        None           = 0,
        Exception      = 1,
        Exited         = 2,
        DebugRunning   = 3,
        DebugBreak     = 4,
        Started        = 5,
    };

    inline u32 GetProcessEventValue(ProcessEvent event) {
        if (hos::GetVersion() >= hos::Version_5_0_0) {
            return static_cast<u32>(event);
        }
        switch (event) {
            case ProcessEvent::None:
                return static_cast<u32>(ProcessEventDeprecated::None);
            case ProcessEvent::Exited:
                return static_cast<u32>(ProcessEventDeprecated::Exited);
            case ProcessEvent::Started:
                return static_cast<u32>(ProcessEventDeprecated::Started);
            case ProcessEvent::Exception:
                return static_cast<u32>(ProcessEventDeprecated::Exception);
            case ProcessEvent::DebugRunning:
                return static_cast<u32>(ProcessEventDeprecated::DebugRunning);
            case ProcessEvent::DebugBreak:
                return static_cast<u32>(ProcessEventDeprecated::DebugBreak);
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    struct ProcessEventInfo {
        u32 event;
        os::ProcessId process_id;
    };
    static_assert(sizeof(ProcessEventInfo) == 0x10 && util::is_pod<ProcessEventInfo>::value, "ProcessEventInfo definition!");

}
