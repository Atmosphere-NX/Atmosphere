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

    #define INNER_HEAP_SIZE 0x4000
    size_t nx_inner_heap_size = INNER_HEAP_SIZE;
    char   nx_inner_heap[INNER_HEAP_SIZE];

    void __libnx_initheap(void);
    void __appInit(void);
    void __appExit(void);

    /* Exception handling. */
    alignas(16) u8 __nx_exception_stack[ams::os::MemoryPageSize];
    u64 __nx_exception_stack_size = sizeof(__nx_exception_stack);
    void __libnx_exception_handler(ThreadExceptionDump *ctx);
}

namespace ams {

    ncm::ProgramId CurrentProgramId = ncm::SystemProgramId::Pgl;

    namespace result {

        bool CallFatalOnResultAssertion = false;

    }

}

using namespace ams;

void __libnx_exception_handler(ThreadExceptionDump *ctx) {
    ams::CrashHandler(ctx);
}

namespace ams::pgl {

    namespace {

        /* pgl. */
        constexpr size_t NumServers  = 1;
        ams::sf::hipc::ServerManager<NumServers> g_server_manager;

        constexpr sm::ServiceName ShellServiceName = sm::ServiceName::Encode("pgl");
        constexpr size_t          ShellMaxSessions = 8; /* Official maximum is 8. */

        constinit pgl::srv::ShellInterface g_shell_interface;

        void RegisterServiceSession() {
            R_ABORT_UNLESS((g_server_manager.RegisterServer<pgl::sf::IShellInterface, pgl::srv::ShellInterface>(ShellServiceName, ShellMaxSessions, ams::sf::GetSharedPointerTo<pgl::sf::IShellInterface>(g_shell_interface))));
        }

        void LoopProcess() {
            g_server_manager.LoopProcess();
        }

        /* NOTE: Nintendo reserves only 0x2000 bytes for this heap, which is used "mostly" to allocate shell event observers. */
        /* However, we would like very much for homebrew sysmodules to be able to subscribe to events if they so choose */
        /* And so we will use a larger heap (32 KB). */
        /* We should have a smaller memory footprint than N in the end, regardless. */
        u8 g_heap_memory[32_KB];
        TYPED_STORAGE(ams::sf::ExpHeapMemoryResource) g_heap_memory_resource;

        void *Allocate(size_t size) {
            return lmem::AllocateFromExpHeap(GetReference(g_heap_memory_resource).GetHandle(), size);
        }

        void Deallocate(void *p, size_t size) {
            return lmem::FreeToExpHeap(GetReference(g_heap_memory_resource).GetHandle(), p);
        }

        void InitializeHeap() {
            auto heap_handle = lmem::CreateExpHeap(g_heap_memory, sizeof(g_heap_memory), lmem::CreateOption_ThreadSafe);
            new (GetPointer(g_heap_memory_resource)) ams::sf::ExpHeapMemoryResource(heap_handle);
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
    pgl::srv::Initialize(std::addressof(pgl::g_shell_interface), GetPointer(pgl::g_heap_memory_resource));

    /* Loop forever, servicing our services. */
    pgl::LoopProcess();

    /* Cleanup */
    return 0;
}

