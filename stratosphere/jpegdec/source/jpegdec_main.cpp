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
#include "jpegdec_memory_management.hpp"

namespace ams {

    namespace init {

        void InitializeSystemModule() {
            /* Initialize heap. */
            jpegdec::InitializeJpegHeap();

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
        os::SetThreadNamePointer(os::GetCurrentThread(), AMS_GET_SYSTEM_THREAD_NAME(jpegdec, Main));

        /* Official jpegdec changes its thread priority to 21 in main. */
        /* This is because older versions of the sysmodule had priority 20 in npdm. */
        os::ChangeThreadPriority(os::GetCurrentThread(), AMS_GET_SYSTEM_THREAD_PRIORITY(jpegdec, Main));
        AMS_ASSERT(os::GetThreadPriority(os::GetCurrentThread()) == AMS_GET_SYSTEM_THREAD_PRIORITY(jpegdec, Main));

        /* Initialize the capsrv library. */
        R_ABORT_UNLESS(capsrv::server::InitializeForDecoderServer());

        /* Service the decoder server. */
        capsrv::server::DecoderControlServerThreadFunction(nullptr);

        /* Finalize the capsrv library. */
        capsrv::server::FinalizeForDecoderServer();
    }

}
