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
#include "tio_file_server.hpp"

extern "C" {
    extern u32 __start__;

    u32 __nx_applet_type = AppletType_None;
    u32 __nx_fs_num_sessions = 1;

    #define INNER_HEAP_SIZE 0x0
    size_t nx_inner_heap_size = INNER_HEAP_SIZE;
    char   nx_inner_heap[INNER_HEAP_SIZE];

    void __libnx_initheap(void);
    void __appInit(void);
    void __appExit(void);

    void *__libnx_alloc(size_t size);
    void *__libnx_aligned_alloc(size_t alignment, size_t size);
    void __libnx_free(void *mem);
}

namespace ams {

    ncm::ProgramId CurrentProgramId = ncm::SystemProgramId::DevServer;

}

using namespace ams;

#define AMS_TIO_SERVER_USE_FATAL_ERROR 1

#if AMS_TIO_SERVER_USE_FATAL_ERROR

extern "C" {

    /* Exception handling. */
    alignas(16) u8 __nx_exception_stack[ams::os::MemoryPageSize];
    u64 __nx_exception_stack_size = sizeof(__nx_exception_stack);
    void __libnx_exception_handler(ThreadExceptionDump *ctx);

}

void __libnx_exception_handler(ThreadExceptionDump *ctx) {
    ams::CrashHandler(ctx);
}

#endif

namespace ams::tio {

    namespace {

        alignas(0x40) constinit u8 g_fs_heap_buffer[64_KB];
        alignas(0x40) constinit u8 g_htcs_buffer[1_KB];
        lmem::HeapHandle g_fs_heap_handle;

        void *AllocateForFs(size_t size) {
            return lmem::AllocateFromExpHeap(g_fs_heap_handle, size);
        }

        void DeallocateForFs(void *p, size_t size) {
            return lmem::FreeToExpHeap(g_fs_heap_handle, p);
        }

        void InitializeFsHeap() {
            /* Setup fs allocator. */
            g_fs_heap_handle = lmem::CreateExpHeap(g_fs_heap_buffer, sizeof(g_fs_heap_buffer), lmem::CreateOption_ThreadSafe);
        }

    }

}

void __libnx_initheap(void) {
	void*  addr = nx_inner_heap;
	size_t size = nx_inner_heap_size;

	/* Newlib */
	extern char* fake_heap_start;
	extern char* fake_heap_end;

	fake_heap_start = (char*)addr;
	fake_heap_end   = (char*)addr + size;

    ams::tio::InitializeFsHeap();
}

void __appInit(void) {
    hos::InitializeForStratosphere();

    /* Initialize FS heap. */
    fs::SetAllocator(tio::AllocateForFs, tio::DeallocateForFs);

    /* Disable FS auto-abort. */
    fs::SetEnabledAutoAbort(false);

    sm::DoWithSession([&]() {
        R_ABORT_UNLESS(fsInitialize());
    });

    ams::CheckApiVersion();
}

void __appExit(void) {
    fsExit();
}

namespace ams {

    void *Malloc(size_t size) {
        AMS_ABORT("ams::Malloc was called");
    }

    void Free(void *ptr) {
        AMS_ABORT("ams::Free was called");
    }

}

void *operator new(size_t size) {
    AMS_ABORT("operator new(size_t) was called");
}

void operator delete(void *p) {
    AMS_ABORT("operator delete(void *) was called");
}

void *__libnx_alloc(size_t size) {
    AMS_ABORT("__libnx_alloc was called");
}

void *__libnx_aligned_alloc(size_t alignment, size_t size) {
    AMS_ABORT("__libnx_aligned_alloc was called");
}

void __libnx_free(void *mem) {
    AMS_ABORT("__libnx_free was called");
}

int main(int argc, char **argv)
{
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

    return 0;
}
