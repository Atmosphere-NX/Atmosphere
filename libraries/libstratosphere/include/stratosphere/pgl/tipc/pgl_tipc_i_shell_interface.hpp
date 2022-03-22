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

#pragma once
#include <vapours.hpp>
#include <stratosphere/os.hpp>
#include <stratosphere/pm.hpp>
#include <stratosphere/tipc.hpp>
#include <stratosphere/pgl/pgl_types.hpp>
#include <stratosphere/pgl/tipc/pgl_tipc_i_event_observer.hpp>

#define AMS_PGL_TIPC_I_SHELL_INTERFACE_INTERFACE_INFO(C, H)                                                                                                                                                         \
    AMS_TIPC_METHOD_INFO(C, H,  0, Result, LaunchProgram,                         (ams::tipc::Out<os::ProcessId> out, const ncm::ProgramLocation loc, u32 pm_flags, u8 pgl_flags), (out, loc, pm_flags, pgl_flags)) \
    AMS_TIPC_METHOD_INFO(C, H,  1, Result, TerminateProcess,                      (os::ProcessId process_id),                                                                      (process_id))                    \
    AMS_TIPC_METHOD_INFO(C, H,  2, Result, LaunchProgramFromHost,                 (ams::tipc::Out<os::ProcessId> out, const ams::tipc::InBuffer content_path, u32 pm_flags),       (out, content_path, pm_flags))   \
    AMS_TIPC_METHOD_INFO(C, H,  4, Result, GetHostContentMetaInfo,                (ams::tipc::Out<pgl::ContentMetaInfo> out, const ams::tipc::InBuffer content_path),              (out, content_path))             \
    AMS_TIPC_METHOD_INFO(C, H,  5, Result, GetApplicationProcessId,               (ams::tipc::Out<os::ProcessId> out),                                                             (out))                           \
    AMS_TIPC_METHOD_INFO(C, H,  6, Result, BoostSystemMemoryResourceLimit,        (u64 size),                                                                                      (size))                          \
    AMS_TIPC_METHOD_INFO(C, H,  7, Result, IsProcessTracked,                      (ams::tipc::Out<bool> out, os::ProcessId process_id),                                            (out, process_id))               \
    AMS_TIPC_METHOD_INFO(C, H,  8, Result, EnableApplicationCrashReport,          (bool enabled),                                                                                  (enabled))                       \
    AMS_TIPC_METHOD_INFO(C, H,  9, Result, IsApplicationCrashReportEnabled,       (ams::tipc::Out<bool> out),                                                                      (out))                           \
    AMS_TIPC_METHOD_INFO(C, H, 10, Result, EnableApplicationAllThreadDumpOnCrash, (bool enabled),                                                                                  (enabled))                       \
    AMS_TIPC_METHOD_INFO(C, H, 20, Result, GetShellEventObserver,                 (ams::tipc::OutMoveHandle out),                                                                  (out))

AMS_TIPC_DEFINE_INTERFACE(ams::pgl::tipc, IShellInterface, AMS_PGL_TIPC_I_SHELL_INTERFACE_INTERFACE_INFO);
