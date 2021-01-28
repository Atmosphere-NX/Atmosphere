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

    ncm::ProgramId CurrentProgramId = ncm::SystemProgramId::Pgl;

}

using namespace ams;

void __libnx_exception_handler(ThreadExceptionDump *ctx) {
    ams::CrashHandler(ctx);
}

namespace ams::pgl {

    namespace {

        /* NOTE: Nintendo reserves only 0x2000 bytes for this heap, which is used "mostly" to allocate shell event observers. */
        /* However, we would like very much for homebrew sysmodules to be able to subscribe to events if they so choose */
        /* And so we will use a larger heap (32 KB). */
        /* We should have a smaller memory footprint than N in the end, regardless. */
        constinit u8 g_heap_memory[32_KB];
        lmem::HeapHandle g_server_heap_handle;
        constinit ams::sf::ExpHeapAllocator g_server_allocator;

        void *Allocate(size_t size) {
            return lmem::AllocateFromExpHeap(g_server_heap_handle, size);
        }

        void Deallocate(void *p, size_t size) {
            return lmem::FreeToExpHeap(g_server_heap_handle, p);
        }

        void InitializeHeap() {
            g_server_heap_handle = lmem::CreateExpHeap(g_heap_memory, sizeof(g_heap_memory), lmem::CreateOption_ThreadSafe);
            g_server_allocator.Attach(g_server_heap_handle);
        }

    }

    namespace {

        /* pgl. */
        enum PortIndex {
            PortIndex_Shell,
            PortIndex_Count,
        };

        constexpr sm::ServiceName ShellServiceName = sm::ServiceName::Encode("pgl");
        constexpr size_t          ShellMaxSessions = 8; /* Official maximum is 8. */

        using ServerManager = ams::sf::hipc::ServerManager<PortIndex_Count>;

        ServerManager g_server_manager;

        constinit ams::sf::UnmanagedServiceObject<pgl::sf::IShellInterface, pgl::srv::ShellInterface> g_shell_interface(std::addressof(g_server_allocator));

        void RegisterServiceSession() {
            R_ABORT_UNLESS(g_server_manager.RegisterObjectForServer(g_shell_interface.GetShared(), ShellServiceName, ShellMaxSessions));
        }

        void LoopProcess() {
            g_server_manager.LoopProcess();
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

    ams::pgl::InitializeHeap();
}

void __appInit(void) {
    hos::InitializeForStratosphere();

    fs::SetAllocator(pgl::Allocate, pgl::Deallocate);

    sm::DoWithSession([&]() {
        R_ABORT_UNLESS(setInitialize());
        R_ABORT_UNLESS(setsysInitialize());
        R_ABORT_UNLESS(pmshellInitialize());
        R_ABORT_UNLESS(ldrShellInitialize());
        R_ABORT_UNLESS(lrInitialize());
        R_ABORT_UNLESS(fsInitialize());
    });

    ams::CheckApiVersion();
}

void __appExit(void) {
    fsExit();
    lrExit();
    ldrShellExit();
    pmshellExit();
    setsysExit();
    setExit();
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
    return pgl::Allocate(size);
}

void operator delete(void *p) {
    return pgl::Deallocate(p, 0);
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
    /* Disable auto-abort in fs operations. */
    fs::SetEnabledAutoAbort(false);

    /* Set thread name. */
    os::SetThreadNamePointer(os::GetCurrentThread(), AMS_GET_SYSTEM_THREAD_NAME(pgl, Main));
    AMS_ASSERT(os::GetThreadPriority(os::GetCurrentThread()) == AMS_GET_SYSTEM_THREAD_PRIORITY(pgl, Main));

    /* Register the pgl service. */
    pgl::RegisterServiceSession();

    /* Initialize the server library. */
    pgl::srv::Initialize();

    /* Loop forever, servicing our services. */
    pgl::LoopProcess();

    /* Cleanup */
    return 0;
}

