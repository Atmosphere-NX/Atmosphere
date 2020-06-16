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

        /* Convenience definitions. */
        constexpr os::ProcessId InitialProcessIdMinDeprecated = {0x00};
        constexpr os::ProcessId InitialProcessIdMaxDeprecated = {0x50};

        /* Privileged process globals. */
        os::Mutex g_lock(false);
        bool g_got_privileged_process_status = false;
        os::ProcessId g_min_initial_process_id = os::InvalidProcessId, g_max_initial_process_id = os::InvalidProcessId;
        os::ProcessId g_cur_process_id = os::InvalidProcessId;

        /* SD card helpers. */
        void GetPrivilegedProcessIdRange(os::ProcessId *out_min, os::ProcessId *out_max) {
            os::ProcessId min = os::InvalidProcessId, max = os::InvalidProcessId;
            if (hos::GetVersion() >= hos::Version_5_0_0) {
                /* On 5.0.0+, we can get precise limits from svcGetSystemInfo. */
                R_ABORT_UNLESS(svcGetSystemInfo(reinterpret_cast<u64 *>(&min), SystemInfoType_InitialProcessIdRange, INVALID_HANDLE, InitialProcessIdRangeInfo_Minimum));
                R_ABORT_UNLESS(svcGetSystemInfo(reinterpret_cast<u64 *>(&max), SystemInfoType_InitialProcessIdRange, INVALID_HANDLE, InitialProcessIdRangeInfo_Maximum));
            } else if (hos::GetVersion() >= hos::Version_4_0_0) {
                /* On 4.0.0-4.1.0, we can get the precise limits from normal svcGetInfo. */
                R_ABORT_UNLESS(svcGetInfo(reinterpret_cast<u64 *>(&min), InfoType_InitialProcessIdRange, INVALID_HANDLE, InitialProcessIdRangeInfo_Minimum));
                R_ABORT_UNLESS(svcGetInfo(reinterpret_cast<u64 *>(&max), InfoType_InitialProcessIdRange, INVALID_HANDLE, InitialProcessIdRangeInfo_Maximum));
            } else {
                /* On < 4.0.0, we just use hardcoded extents. */
                min = InitialProcessIdMinDeprecated;
                max = InitialProcessIdMaxDeprecated;
            }

            *out_min = min;
            *out_max = max;
        }

        void GetPrivilegedProcessStatus() {
            GetPrivilegedProcessIdRange(&g_min_initial_process_id, &g_max_initial_process_id);
            g_cur_process_id = os::GetCurrentProcessId();
            g_got_privileged_process_status = true;
        }

    }

    /* Privileged Process utilities. */
    bool IsInitialProcess() {
        std::scoped_lock lk(g_lock);

        /* If we've not detected, do detection. */
        if (!g_got_privileged_process_status) {
            GetPrivilegedProcessStatus();
        }

        /* Determine if we're privileged, and return. */
        return g_min_initial_process_id <= g_cur_process_id && g_cur_process_id <= g_max_initial_process_id;
    }

    void GetInitialProcessRange(os::ProcessId *out_min, os::ProcessId *out_max) {
        std::scoped_lock lk(g_lock);

        /* If we've not detected, do detection. */
        if (!g_got_privileged_process_status) {
            GetPrivilegedProcessStatus();
        }

        *out_min = g_min_initial_process_id;
        *out_max = g_max_initial_process_id;
    }

}
