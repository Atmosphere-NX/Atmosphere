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

    class DebugMonitorService final {
        public:
            /* Actual command implementations. */
            Result GetModuleIdList(sf::Out<u32> out_count, const sf::OutBuffer &out_buf, u64 unused);
            Result GetExceptionProcessIdList(sf::Out<u32> out_count, const sf::OutArray<os::ProcessId> &out_process_ids);
            Result StartProcess(os::ProcessId process_id);
            Result GetProcessId(sf::Out<os::ProcessId> out, ncm::ProgramId program_id);
            Result HookToCreateProcess(sf::OutCopyHandle out_hook, ncm::ProgramId program_id);
            Result GetApplicationProcessId(sf::Out<os::ProcessId> out);
            Result HookToCreateApplicationProcess(sf::OutCopyHandle out_hook);
            Result ClearHook(u32 which);

            /* Atmosphere extension commands. */
            Result AtmosphereGetProcessInfo(sf::OutCopyHandle out_process_handle, sf::Out<ncm::ProgramLocation> out_loc, sf::Out<cfg::OverrideStatus> out_status, os::ProcessId process_id);
            Result AtmosphereGetCurrentLimitInfo(sf::Out<s64> out_cur_val, sf::Out<s64> out_lim_val, u32 group, u32 resource);
    };
    static_assert(pm::impl::IsIDebugMonitorInterface<DebugMonitorService>);

}
