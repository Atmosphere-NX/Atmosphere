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
#include <stratosphere.hpp>
#include "fssrv_program_info.hpp"

namespace ams::fssrv::impl {

    namespace {

        constinit os::SdkMutex g_mutex;
        constinit bool g_initialized = false;

        constinit u64 g_initial_process_id_min = 0;
        constinit u64 g_initial_process_id_max = 0;

        constinit u64 g_current_process_id = 0;

        ALWAYS_INLINE void InitializeInitialAndCurrentProcessId() {
            if (AMS_UNLIKELY(!g_initialized)) {
                std::scoped_lock lk(g_mutex);
                if (AMS_LIKELY(!g_initialized)) {
                    /* Get initial process id range. */
                    R_ABORT_UNLESS(svc::GetSystemInfo(std::addressof(g_initial_process_id_min), svc::SystemInfoType_InitialProcessIdRange, svc::InvalidHandle, svc::InitialProcessIdRangeInfo_Minimum));
                    R_ABORT_UNLESS(svc::GetSystemInfo(std::addressof(g_initial_process_id_max), svc::SystemInfoType_InitialProcessIdRange, svc::InvalidHandle, svc::InitialProcessIdRangeInfo_Maximum));

                    AMS_ABORT_UNLESS(0 < g_initial_process_id_min);
                    AMS_ABORT_UNLESS(g_initial_process_id_min <= g_initial_process_id_max);

                    /* Get current procss id. */
                    R_ABORT_UNLESS(svc::GetProcessId(std::addressof(g_current_process_id), svc::PseudoHandle::CurrentProcess));

                    /* Set initialized. */
                    g_initialized = true;
                }
            }
        }

    }

    bool IsInitialProgram(u64 process_id) {
        /* Initialize/sanity check. */
        InitializeInitialAndCurrentProcessId();
        AMS_ABORT_UNLESS(g_initial_process_id_min > 0);

        /* Check process id in range. */
        return g_initial_process_id_min <= process_id && process_id <= g_initial_process_id_max;
    }

    bool IsCurrentProcess(u64 process_id) {
        /* Initialize. */
        InitializeInitialAndCurrentProcessId();

        return process_id == g_current_process_id;
    }

}
