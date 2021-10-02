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
#include <stratosphere.hpp>

namespace ams::cfg {

    namespace {

        constinit os::SdkMutex g_lock;
        constinit bool g_got_privileged_process_status = false;
        constinit os::ProcessId g_min_initial_process_id = os::InvalidProcessId, g_max_initial_process_id = os::InvalidProcessId;
        constinit os::ProcessId g_cur_process_id = os::InvalidProcessId;

        ALWAYS_INLINE void EnsurePrivilegedProcessStatusCached() {
            if (AMS_LIKELY(g_got_privileged_process_status)) {
                return;
            }

            std::scoped_lock lk(g_lock);

            if (AMS_LIKELY(!g_got_privileged_process_status)) {
                R_ABORT_UNLESS(svc::GetSystemInfo(std::addressof(g_min_initial_process_id.value), svc::SystemInfoType_InitialProcessIdRange, svc::InvalidHandle, svc::InitialProcessIdRangeInfo_Minimum));
                R_ABORT_UNLESS(svc::GetSystemInfo(std::addressof(g_max_initial_process_id.value), svc::SystemInfoType_InitialProcessIdRange, svc::InvalidHandle, svc::InitialProcessIdRangeInfo_Maximum));
                g_cur_process_id = os::GetCurrentProcessId();

                g_got_privileged_process_status = true;
            }
        }

    }

    bool IsInitialProcess() {
        /* Cache initial process range and extents. */
        EnsurePrivilegedProcessStatusCached();

        /* Determine if we're Initial. */
        return g_min_initial_process_id <= g_cur_process_id && g_cur_process_id <= g_max_initial_process_id;
    }

    void GetInitialProcessRange(os::ProcessId *out_min, os::ProcessId *out_max) {
        /* Cache initial process range and extents. */
        EnsurePrivilegedProcessStatusCached();

        /* Set output. */
        *out_min = g_min_initial_process_id;
        *out_max = g_max_initial_process_id;
    }

}
