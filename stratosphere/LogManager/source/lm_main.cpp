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

extern "C" {
    extern u32 __start__;

    u32 __nx_applet_type = AppletType_None;
    u32 __nx_fs_num_sessions = 2;

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

using namespace ams;

#define AMS_LM_USE_FATAL_ERROR 1

#if AMS_LM_USE_FATAL_ERROR

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

void __libnx_initheap(void) {
	void*  addr = nx_inner_heap;
	size_t size = nx_inner_heap_size;

	/* Newlib */
	extern char* fake_heap_start;
	extern char* fake_heap_end;

	fake_heap_start = (char*)addr;
	fake_heap_end   = (char*)addr + size;
}

void __appInit(void) {
    hos::InitializeForStratosphere();

    R_ABORT_UNLESS(sm::Initialize());

    R_ABORT_UNLESS(::fsInitialize());
    R_ABORT_UNLESS(::setsysInitialize());
    R_ABORT_UNLESS(::pscmInitialize());

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

namespace ams::lm::srv {

    void StartLogServerProxy();
    void StopLogServerProxy();

    void InitializeFlushThread();
    void FinalizeFlushThread();

    void InitializeIpcServer();
    void LoopIpcServer();
    void FinalizeIpcServer();

}

int main(int argc, char **argv)
{
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

    return 0;
}
