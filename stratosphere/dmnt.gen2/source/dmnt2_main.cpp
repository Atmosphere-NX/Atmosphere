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
#include "dmnt2_debug_log.hpp"
#include "dmnt2_gdb_server.hpp"

namespace ams {

    namespace dmnt {

        namespace {

            alignas(0x40) constinit u8 g_htcs_buffer[4_KB];

        }

    }

    namespace init {

        void InitializeSystemModule() {
            /* Initialize our connection to sm. */
            R_ABORT_UNLESS(sm::Initialize());

            /* Verify that we can sanely execute. */
            ams::CheckApiVersion();
        }

        void FinalizeSystemModule() { /* ... */ }

        void Startup() { /* ... */ }

    }

    void Main() {
        /* Set thread name. */
        os::SetThreadNamePointer(os::GetCurrentThread(), AMS_GET_SYSTEM_THREAD_NAME(dmnt, Main));
        AMS_ASSERT(os::GetThreadPriority(os::GetCurrentThread()) == AMS_GET_SYSTEM_THREAD_PRIORITY(dmnt, Main));

        /* Initialize htcs. */
        constexpr auto HtcsSocketCountMax = 8;
        const size_t buffer_size = htcs::GetWorkingMemorySize(HtcsSocketCountMax);
        AMS_ABORT_UNLESS(sizeof(dmnt::g_htcs_buffer) >= buffer_size);
        htcs::InitializeForSystem(dmnt::g_htcs_buffer, buffer_size, HtcsSocketCountMax);

        /* Initialize debug log thread. */
        dmnt::InitializeDebugLog();

        /* Start GdbServer. */
        dmnt::InitializeGdbServer();

        /* TODO */
        while (true) {
            os::SleepThread(TimeSpan::FromDays(1));
        }
    }

}
