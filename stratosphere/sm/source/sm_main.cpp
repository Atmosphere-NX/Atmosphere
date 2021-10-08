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
#include "sm_tipc_server.hpp"

namespace ams {

    namespace init {

        void InitializeSystemModule() { /* ... */ }

        void FinalizeSystemModule() { /* ... */ }

        void Startup() { /* ... */ }

    }

    void NORETURN Exit(int rc) {
        AMS_UNUSED(rc);
        AMS_ABORT("Exit called by immortal process");
    }

    void Main() {
        /* Set thread name. */
        os::SetThreadNamePointer(os::GetCurrentThread(), AMS_GET_SYSTEM_THREAD_NAME(sm, Main));
        AMS_ASSERT(os::GetThreadPriority(os::GetCurrentThread()) == AMS_GET_SYSTEM_THREAD_PRIORITY(sm, Main));

        /* Initialize the server. */
        sm::InitializeTipcServer();

        /* Loop forever, processing our services. */
        sm::LoopProcessTipcServer();

        /* This can never be reached. */
        AMS_ASSUME(false);
    }

}
