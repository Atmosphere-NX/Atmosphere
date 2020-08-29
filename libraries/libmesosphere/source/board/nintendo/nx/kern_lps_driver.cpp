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
#include <mesosphere.hpp>
#include "kern_lps_driver.hpp"
#include "kern_k_sleep_manager.hpp"

namespace ams::kern::board::nintendo::nx::lps {

    namespace {

        constinit bool g_lps_init_done = false;

    }

    void Initialize() {
        if (!g_lps_init_done) {
            MESOSPHERE_UNIMPLEMENTED();

            g_lps_init_done = true;
        }
    }

    Result EnableSuspend() {
        MESOSPHERE_UNIMPLEMENTED();
    }

    void InvokeCpuSleepHandler(uintptr_t arg, uintptr_t entry) {
        MESOSPHERE_ABORT_UNLESS(g_lps_init_done);
        MESOSPHERE_ABORT_UNLESS(GetCurrentCoreId() == 0);

        MESOSPHERE_UNIMPLEMENTED();

        /* Invoke the sleep hander. */
        KSleepManager::CpuSleepHandler(arg, entry);
    }

}