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
#include "dmnt2_transport_layer.hpp"

namespace ams {

    namespace {

        bool IsHtcEnabled() {
            u8 enable_htc = 0;
            settings::fwdbg::GetSettingsItemValue(std::addressof(enable_htc), sizeof(enable_htc), "atmosphere", "enable_htc");
            return enable_htc != 0;
        }

        bool IsStandaloneGdbstubEnabled() {
            u8 enable_gdbstub = 0;
            settings::fwdbg::GetSettingsItemValue(std::addressof(enable_gdbstub), sizeof(enable_gdbstub), "atmosphere", "enable_standalone_gdbstub");
            return enable_gdbstub != 0;
        }

    }

    namespace init {

        void InitializeSystemModule() {
            /* Initialize our connection to sm. */
            R_ABORT_UNLESS(sm::Initialize());

            /* Initialize other services we need. */
            R_ABORT_UNLESS(pmdmntInitialize());

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

        bool use_htcs = false, use_tcp = false;
        {
            R_ABORT_UNLESS(::setsysInitialize());
            ON_SCOPE_EXIT { ::setsysExit(); };

            use_htcs = IsHtcEnabled();
            use_tcp  = IsStandaloneGdbstubEnabled();
        }

        /* Initialize transport layer. */
        if (use_htcs) {
            dmnt::transport::InitializeByHtcs();
        } else if (use_tcp) {
            dmnt::transport::InitializeByTcp();
        } else {
            return;
        }

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
