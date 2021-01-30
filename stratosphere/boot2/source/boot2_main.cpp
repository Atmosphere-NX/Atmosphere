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

    /* Exception handling. */
    alignas(16) u8 __nx_exception_stack[ams::os::MemoryPageSize];
    u64 __nx_exception_stack_size = sizeof(__nx_exception_stack);
    void __libnx_exception_handler(ThreadExceptionDump *ctx);

    void *__libnx_alloc(size_t size);
    void *__libnx_aligned_alloc(size_t alignment, size_t size);
    void __libnx_free(void *mem);
}

namespace ams {

    ncm::ProgramId CurrentProgramId = ncm::SystemProgramId::Boot2;

}

using namespace ams;

void __libnx_exception_handler(ThreadExceptionDump *ctx) {
    ams::CrashHandler(ctx);
}

void __libnx_initheap(void) {
	void*  addr = nx_inner_heap;
	size_t size = nx_inner_heap_size;

	/* Newlib */
	extern char* fake_heap_start;
	extern char* fake_heap_end;

	fake_heap_start = (char*)addr;
	fake_heap_end   = (char*)addr + size;
}

namespace {


    constinit u8 g_fs_heap_memory[2_KB];
    lmem::HeapHandle g_fs_heap_handle;

    void *AllocateForFs(size_t size) {
        return lmem::AllocateFromExpHeap(g_fs_heap_handle, size);
    }

    void DeallocateForFs(void *p, size_t size) {
        return lmem::FreeToExpHeap(g_fs_heap_handle, p);
    }

    void InitializeFsHeap() {
        g_fs_heap_handle = lmem::CreateExpHeap(g_fs_heap_memory, sizeof(g_fs_heap_memory), lmem::CreateOption_None);
    }

}

void __appInit(void) {
    hos::InitializeForStratosphere();

    InitializeFsHeap();
    fs::SetAllocator(AllocateForFs, DeallocateForFs);

    /* Initialize services we need. */
    sm::DoWithSession([&]() {
        R_ABORT_UNLESS(fsInitialize());
        R_ABORT_UNLESS(pmbmInitialize());
        R_ABORT_UNLESS(pminfoInitialize());
        R_ABORT_UNLESS(pmshellInitialize());
        R_ABORT_UNLESS(setsysInitialize());
        gpio::Initialize();
    });

    /* Mount the SD card. */
    R_ABORT_UNLESS(fs::MountSdCard("sdmc"));

    ams::CheckApiVersion();
}

void __appExit(void) {
    fs::Unmount("sdmc");
    gpio::Finalize();
    setsysExit();
    pmshellExit();
    pminfoExit();
    pmbmExit();
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
    os::SetThreadNamePointer(os::GetCurrentThread(), AMS_GET_SYSTEM_THREAD_NAME(boot2, Main));
    AMS_ASSERT(os::GetThreadPriority(os::GetCurrentThread()) == AMS_GET_SYSTEM_THREAD_PRIORITY(boot2, Main));

    /* Launch all programs off of SYSTEM/the SD. */
    boot2::LaunchPostSdCardBootPrograms();

    return 0;
}

