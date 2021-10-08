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

namespace ams {

    namespace lm::srv {

        void StartLogServerProxy();
        void StopLogServerProxy();

        void InitializeFlushThread();
        void FinalizeFlushThread();

        void InitializeIpcServer();
        void LoopIpcServer();
        void FinalizeIpcServer();

    }

    namespace init {

        void InitializeSystemModule() {
            /* Initialize our connection to sm. */
            R_ABORT_UNLESS(sm::Initialize());

            /* Initialize services we need. */
            R_ABORT_UNLESS(::setsysInitialize());
            R_ABORT_UNLESS(::pscmInitialize());

            /* Verify that we can sanely execute. */
            ams::CheckApiVersion();
        }

        void FinalizeSystemModule() { /* ... */ }

        void Startup() { /* ... */ }

    }

    void Main() {
        /* Check thread priority. */
        AMS_ASSERT(os::GetThreadPriority(os::GetCurrentThread()) == AMS_GET_SYSTEM_THREAD_PRIORITY(LogManager, MainThread));

        /* Set thread name. */
        os::ChangeThreadPriority(os::GetCurrentThread(), AMS_GET_SYSTEM_THREAD_PRIORITY(lm, IpcServer));
        os::SetThreadNamePointer(os::GetCurrentThread(), AMS_GET_SYSTEM_THREAD_NAME(lm, IpcServer));

        /* Start log server proxy. */
        lm::srv::StartLogServerProxy();

        /* Initialize flush thread. */
        lm::srv::InitializeFlushThread();

        /* Process IPC server. */
        lm::srv::InitializeIpcServer();
        lm::srv::LoopIpcServer();
        lm::srv::FinalizeIpcServer();

        /* Finalize flush thread. */
        lm::srv::FinalizeFlushThread();

        /* Stop log server proxy. */
        lm::srv::StopLogServerProxy();
    }

}
