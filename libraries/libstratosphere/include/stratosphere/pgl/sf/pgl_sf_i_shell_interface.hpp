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
#include <stratosphere/pgl/pgl_types.hpp>
#include <stratosphere/pgl/sf/pgl_sf_i_event_observer.hpp>

#define AMS_PGL_I_SHELL_INTERFACE_INTERFACE_INFO(C, H)                                                                                                                                                                                \
    AMS_SF_METHOD_INFO(C, H,  0, Result, LaunchProgram,                         (ams::sf::Out<os::ProcessId> out, const ncm::ProgramLocation &loc, u32 pm_flags, u8 pgl_flags), (out, loc, pm_flags, pgl_flags))                      \
    AMS_SF_METHOD_INFO(C, H,  1, Result, TerminateProcess,                      (os::ProcessId process_id),                                                                     (process_id))                                         \
    AMS_SF_METHOD_INFO(C, H,  2, Result, LaunchProgramFromHost,                 (ams::sf::Out<os::ProcessId> out, const ams::sf::InBuffer &content_path, u32 pm_flags),         (out, content_path, pm_flags))                        \
    AMS_SF_METHOD_INFO(C, H,  4, Result, GetHostContentMetaInfo,                (ams::sf::Out<pgl::ContentMetaInfo> out, const ams::sf::InBuffer &content_path),                (out, content_path))                                  \
    AMS_SF_METHOD_INFO(C, H,  5, Result, GetApplicationProcessId,               (ams::sf::Out<os::ProcessId> out),                                                              (out))                                                \
    AMS_SF_METHOD_INFO(C, H,  6, Result, BoostSystemMemoryResourceLimit,        (u64 size),                                                                                     (size))                                               \
    AMS_SF_METHOD_INFO(C, H,  7, Result, IsProcessTracked,                      (ams::sf::Out<bool> out, os::ProcessId process_id),                                             (out, process_id))                                    \
    AMS_SF_METHOD_INFO(C, H,  8, Result, EnableApplicationCrashReport,          (bool enabled),                                                                                 (enabled))                                            \
    AMS_SF_METHOD_INFO(C, H,  9, Result, IsApplicationCrashReportEnabled,       (ams::sf::Out<bool> out),                                                                       (out))                                                \
    AMS_SF_METHOD_INFO(C, H, 10, Result, EnableApplicationAllThreadDumpOnCrash, (bool enabled),                                                                                 (enabled))                                            \
    AMS_SF_METHOD_INFO(C, H, 12, Result, TriggerApplicationSnapShotDumper,      (pgl::SnapShotDumpType dump_type, const ams::sf::InBuffer &arg),                                (dump_type, arg))                                     \
    AMS_SF_METHOD_INFO(C, H, 20, Result, GetShellEventObserver,                 (ams::sf::Out<ams::sf::SharedPointer<pgl::sf::IEventObserver>> out),                            (out))                                                \
    AMS_SF_METHOD_INFO(C, H, 21, Result, Command21NotImplemented,               (ams::sf::Out<u64> out, u32 in, const ams::sf::InBuffer &buf1, const ams::sf::InBuffer &buf2),  (out, in, buf1, buf2),           hos::Version_11_0_0)

AMS_SF_DEFINE_INTERFACE(ams::pgl::sf, IShellInterface, AMS_PGL_I_SHELL_INTERFACE_INTERFACE_INFO);
