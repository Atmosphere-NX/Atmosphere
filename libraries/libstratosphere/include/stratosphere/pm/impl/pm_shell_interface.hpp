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
#include <vapours.hpp>
#include <stratosphere/pm/pm_types.hpp>
#include <stratosphere/sf.hpp>

namespace ams::pm::impl {

    #define AMS_PM_I_SHELL_INTERFACE_INTERFACE_INFO(C, H)                                                                                                                                 \
        AMS_SF_METHOD_INFO(C, H, 0, Result, LaunchProgram,                       (sf::Out<os::ProcessId> out_process_id, const ncm::ProgramLocation &loc, u32 flags))                     \
        AMS_SF_METHOD_INFO(C, H, 1, Result, TerminateProcess,                    (os::ProcessId process_id))                                                                              \
        AMS_SF_METHOD_INFO(C, H, 2, Result, TerminateProgram,                    (ncm::ProgramId program_id))                                                                             \
        AMS_SF_METHOD_INFO(C, H, 3, void,   GetProcessEventHandle,               (sf::OutCopyHandle out))                                                                                 \
        AMS_SF_METHOD_INFO(C, H, 4, void,   GetProcessEventInfo,                 (sf::Out<ProcessEventInfo> out))                                                                         \
        AMS_SF_METHOD_INFO(C, H, 5, void,   NotifyBootFinished,                  ())                                                                                                      \
        AMS_SF_METHOD_INFO(C, H, 6, Result, GetApplicationProcessIdForShell,     (sf::Out<os::ProcessId> out))                                                                            \
        AMS_SF_METHOD_INFO(C, H, 7, Result, BoostSystemMemoryResourceLimit,      (u64 boost_size))                                                                                        \
        AMS_SF_METHOD_INFO(C, H, 8, Result, BoostApplicationThreadResourceLimit, (),                                                                                  hos::Version_7_0_0) \
        AMS_SF_METHOD_INFO(C, H, 9, void,   GetBootFinishedEventHandle,          (sf::OutCopyHandle out),                                                             hos::Version_8_0_0)

    AMS_SF_DEFINE_INTERFACE(IShellInterface, AMS_PM_I_SHELL_INTERFACE_INTERFACE_INFO)

    #define AMS_PM_I_DEPRECATED_SHELL_INTERFACE_INTERFACE_INFO(C, H)                                                                                                                  \
        AMS_SF_METHOD_INFO(C, H, 0, Result, LaunchProgram,                   (sf::Out<os::ProcessId> out_process_id, const ncm::ProgramLocation &loc, u32 flags))                     \
        AMS_SF_METHOD_INFO(C, H, 1, Result, TerminateProcess,                (os::ProcessId process_id))                                                                              \
        AMS_SF_METHOD_INFO(C, H, 2, Result, TerminateProgram,                (ncm::ProgramId program_id))                                                                             \
        AMS_SF_METHOD_INFO(C, H, 3, void,   GetProcessEventHandle,           (sf::OutCopyHandle out))                                                                                 \
        AMS_SF_METHOD_INFO(C, H, 4, void,   GetProcessEventInfo,             (sf::Out<ProcessEventInfo> out))                                                                         \
        AMS_SF_METHOD_INFO(C, H, 5, Result, CleanupProcess,                  (os::ProcessId process_id))                                                                              \
        AMS_SF_METHOD_INFO(C, H, 6, Result, ClearExceptionOccurred,          (os::ProcessId process_id))                                                                              \
        AMS_SF_METHOD_INFO(C, H, 7, void,   NotifyBootFinished,              ())                                                                                                      \
        AMS_SF_METHOD_INFO(C, H, 8, Result, GetApplicationProcessIdForShell, (sf::Out<os::ProcessId> out))                                                                            \
        AMS_SF_METHOD_INFO(C, H, 9, Result, BoostSystemMemoryResourceLimit,  (u64 boost_size),                                                                    hos::Version_4_0_0)

    AMS_SF_DEFINE_INTERFACE(IDeprecatedShellInterface, AMS_PM_I_DEPRECATED_SHELL_INTERFACE_INTERFACE_INFO)

}
