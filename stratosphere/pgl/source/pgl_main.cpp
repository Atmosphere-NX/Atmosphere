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

    namespace init {

        void InitializeSystemModule() {
            /* Initialize heap. */
            pgl::srv::InitializeHeap();

            /* Initialize our connection to sm. */
            R_ABORT_UNLESS(sm::Initialize());

            /* Initialize fs. */
            fs::InitializeForSystem();
            fs::SetAllocator(pgl::srv::Allocate, pgl::srv::Deallocate);
            fs::SetEnabledAutoAbort(false);

            /* Initialize other services we need. */
            R_ABORT_UNLESS(setInitialize());
            R_ABORT_UNLESS(setsysInitialize());
            R_ABORT_UNLESS(pmshellInitialize());
            R_ABORT_UNLESS(ldrShellInitialize());
            R_ABORT_UNLESS(lrInitialize());

            /* Verify that we can sanely execute. */
            ams::CheckApiVersion();
        }

        void FinalizeSystemModule() { /* ... */ }

        void Startup() { /* ... */ }

    }

    void Main() {
        /* Set thread name. */
        os::SetThreadNamePointer(os::GetCurrentThread(), AMS_GET_SYSTEM_THREAD_NAME(pgl, Main));
        AMS_ASSERT(os::GetThreadPriority(os::GetCurrentThread()) == AMS_GET_SYSTEM_THREAD_PRIORITY(pgl, Main));

        /* Initialize and start the server. */
        pgl::srv::StartServer();
    }

}
