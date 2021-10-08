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
#include "tio_file_server.hpp"

namespace ams {

    namespace tio {

        namespace {

            alignas(0x40) constinit u8 g_fs_heap_buffer[64_KB];
            alignas(0x40) constinit u8 g_htcs_buffer[1_KB];
            lmem::HeapHandle g_fs_heap_handle;

            void *AllocateForFs(size_t size) {
                return lmem::AllocateFromExpHeap(g_fs_heap_handle, size);
            }

            void DeallocateForFs(void *p, size_t size) {
                AMS_UNUSED(size);

                return lmem::FreeToExpHeap(g_fs_heap_handle, p);
            }

            void InitializeFsHeap() {
                /* Setup fs allocator. */
                g_fs_heap_handle = lmem::CreateExpHeap(g_fs_heap_buffer, sizeof(g_fs_heap_buffer), lmem::CreateOption_ThreadSafe);
            }

        }

    }

    namespace init {

        void InitializeSystemModule() {
            /* Initialize heap. */
            tio::InitializeFsHeap();

            /* Initialize our connection to sm. */
            R_ABORT_UNLESS(sm::Initialize());

            /* Initialize fs. */
            fs::InitializeForSystem();
            fs::SetAllocator(tio::AllocateForFs, tio::DeallocateForFs);
            fs::SetEnabledAutoAbort(false);

            /* Verify that we can sanely execute. */
            ams::CheckApiVersion();
        }

        void FinalizeSystemModule() { /* ... */ }

        void Startup() { /* ... */ }

    }

    void Main() {
        /* Set thread name. */
        os::SetThreadNamePointer(os::GetCurrentThread(), AMS_GET_SYSTEM_THREAD_NAME(TioServer, Main));
        AMS_ASSERT(os::GetThreadPriority(os::GetCurrentThread()) == AMS_GET_SYSTEM_THREAD_PRIORITY(TioServer, Main));

        /* Initialize htcs. */
        constexpr auto HtcsSocketCountMax = 2;
        const size_t buffer_size = htcs::GetWorkingMemorySize(HtcsSocketCountMax);
        AMS_ABORT_UNLESS(sizeof(tio::g_htcs_buffer) >= buffer_size);
        htcs::InitializeForSystem(tio::g_htcs_buffer, buffer_size, HtcsSocketCountMax);

        /* Initialize the file server. */
        tio::InitializeFileServer();

        /* Start the file server. */
        tio::StartFileServer();

        /* Wait for the file server to finish. */
        tio::WaitFileServer();
    }

}
