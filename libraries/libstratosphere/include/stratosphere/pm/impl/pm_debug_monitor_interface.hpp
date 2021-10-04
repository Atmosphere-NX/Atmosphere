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
#include <stratosphere/pm/pm_types.hpp>
#include <stratosphere/sf.hpp>

#define AMS_PM_I_DEBUG_MONITOR_INTERFACE_INTERFACE_INFO(C, H)                                                                                                                                                                                                                                            \
    AMS_SF_METHOD_INFO(C, H,     0, Result, GetExceptionProcessIdList,      (sf::Out<u32> out_count, const sf::OutArray<os::ProcessId> &out_process_ids),                                                                     (out_count, out_process_ids))                                              \
    AMS_SF_METHOD_INFO(C, H,     1, Result, StartProcess,                   (os::ProcessId process_id),                                                                                                                       (process_id))                                                              \
    AMS_SF_METHOD_INFO(C, H,     2, Result, GetProcessId,                   (sf::Out<os::ProcessId> out, ncm::ProgramId program_id),                                                                                          (out, program_id))                                                         \
    AMS_SF_METHOD_INFO(C, H,     3, Result, HookToCreateProcess,            (sf::OutCopyHandle out_hook, ncm::ProgramId program_id),                                                                                          (out_hook, program_id))                                                    \
    AMS_SF_METHOD_INFO(C, H,     4, Result, GetApplicationProcessId,        (sf::Out<os::ProcessId> out),                                                                                                                     (out))                                                                     \
    AMS_SF_METHOD_INFO(C, H,     5, Result, HookToCreateApplicationProcess, (sf::OutCopyHandle out_hook),                                                                                                                     (out_hook))                                                                \
    AMS_SF_METHOD_INFO(C, H,     6, Result, ClearHook,                      (u32 which),                                                                                                                                      (which),                                               hos::Version_6_0_0) \
    AMS_SF_METHOD_INFO(C, H, 65000, Result, AtmosphereGetProcessInfo,       (sf::OutCopyHandle out_process_handle, sf::Out<ncm::ProgramLocation> out_loc, sf::Out<cfg::OverrideStatus> out_status, os::ProcessId process_id), (out_process_handle, out_loc, out_status, process_id))                     \
    AMS_SF_METHOD_INFO(C, H, 65001, Result, AtmosphereGetCurrentLimitInfo,  (sf::Out<s64> out_cur_val, sf::Out<s64> out_lim_val, u32 group, u32 resource),                                                                    (out_cur_val, out_lim_val, group, resource))

AMS_SF_DEFINE_INTERFACE(ams::pm::impl, IDebugMonitorInterface, AMS_PM_I_DEBUG_MONITOR_INTERFACE_INTERFACE_INFO)

#define AMS_PM_I_DEPRECATED_DEBUG_MONITOR_INTERFACE_INTERFACE_INFO(C, H)                                                                                                                                                                                                             \
    AMS_SF_METHOD_INFO(C, H,     0, Result, GetModuleIdList,                (sf::Out<u32> out_count, const sf::OutBuffer &out_buf, u64 unused),                                                                               (out_count, out_buf, unused))                          \
    AMS_SF_METHOD_INFO(C, H,     1, Result, GetExceptionProcessIdList,      (sf::Out<u32> out_count, const sf::OutArray<os::ProcessId> &out_process_ids),                                                                     (out_count, out_process_ids))                          \
    AMS_SF_METHOD_INFO(C, H,     2, Result, StartProcess,                   (os::ProcessId process_id),                                                                                                                       (process_id))                                          \
    AMS_SF_METHOD_INFO(C, H,     3, Result, GetProcessId,                   (sf::Out<os::ProcessId> out, ncm::ProgramId program_id),                                                                                          (out, program_id))                                     \
    AMS_SF_METHOD_INFO(C, H,     4, Result, HookToCreateProcess,            (sf::OutCopyHandle out_hook, ncm::ProgramId program_id),                                                                                          (out_hook, program_id))                                \
    AMS_SF_METHOD_INFO(C, H,     5, Result, GetApplicationProcessId,        (sf::Out<os::ProcessId> out),                                                                                                                     (out))                                                 \
    AMS_SF_METHOD_INFO(C, H,     6, Result, HookToCreateApplicationProcess, (sf::OutCopyHandle out_hook),                                                                                                                     (out_hook))                                            \
    AMS_SF_METHOD_INFO(C, H, 65000, Result, AtmosphereGetProcessInfo,       (sf::OutCopyHandle out_process_handle, sf::Out<ncm::ProgramLocation> out_loc, sf::Out<cfg::OverrideStatus> out_status, os::ProcessId process_id), (out_process_handle, out_loc, out_status, process_id)) \
    AMS_SF_METHOD_INFO(C, H, 65001, Result, AtmosphereGetCurrentLimitInfo,  (sf::Out<s64> out_cur_val, sf::Out<s64> out_lim_val, u32 group, u32 resource),                                                                    (out_cur_val, out_lim_val, group, resource))

AMS_SF_DEFINE_INTERFACE(ams::pm::impl, IDeprecatedDebugMonitorInterface, AMS_PM_I_DEPRECATED_DEBUG_MONITOR_INTERFACE_INTERFACE_INFO)
