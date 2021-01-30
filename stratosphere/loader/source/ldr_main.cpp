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
#include "ldr_development_manager.hpp"
#include "ldr_loader_service.hpp"

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

    ncm::ProgramId CurrentProgramId = ncm::SystemProgramId::Loader;

}

using namespace ams;

void __libnx_exception_handler(ThreadExceptionDump *ctx) {
    ams::CrashHandler(ctx);
}

namespace ams::ldr {

    namespace {

        constinit u8 g_heap_memory[16_KB];
        lmem::HeapHandle g_server_heap_handle;
        constinit ams::sf::ExpHeapAllocator g_server_allocator;

        void *Allocate(size_t size) {
            return lmem::AllocateFromExpHeap(g_server_heap_handle, size);
        }

        void Deallocate(void *p, size_t size) {
            return lmem::FreeToExpHeap(g_server_heap_handle, p);
        }

        void InitializeHeap() {
            g_server_heap_handle = lmem::CreateExpHeap(g_heap_memory, sizeof(g_heap_memory), lmem::CreateOption_None);
            g_server_allocator.Attach(g_server_heap_handle);
        }

    }

    namespace {

        struct ServerOptions {
            static constexpr size_t PointerBufferSize = 0x400;
            static constexpr size_t MaxDomains = 0;
            static constexpr size_t MaxDomainObjects = 0;
        };

        /* ldr:pm, ldr:shel, ldr:dmnt. */
        enum PortIndex {
            PortIndex_ProcessManager,
            PortIndex_Shell,
            PortIndex_DebugMonitor,
            PortIndex_Count,
        };

        constexpr sm::ServiceName ProcessManagerServiceName = sm::ServiceName::Encode("ldr:pm");
        constexpr size_t          ProcessManagerMaxSessions = 1;

        constexpr sm::ServiceName ShellServiceName = sm::ServiceName::Encode("ldr:shel");
        constexpr size_t          ShellMaxSessions = 3;

        constexpr sm::ServiceName DebugMonitorServiceName = sm::ServiceName::Encode("ldr:dmnt");
        constexpr size_t          DebugMonitorMaxSessions = 3;

        constinit sf::UnmanagedServiceObject<impl::IProcessManagerInterface, LoaderService> g_pm_service;
        constinit sf::UnmanagedServiceObject<impl::IShellInterface, LoaderService> g_shell_service;
        constinit sf::UnmanagedServiceObject<impl::IDebugMonitorInterface, LoaderService> g_dmnt_service;

        constexpr size_t MaxSessions = ProcessManagerMaxSessions + ShellMaxSessions + DebugMonitorMaxSessions + 1;

        using ServerManager = ams::sf::hipc::ServerManager<PortIndex_Count, ServerOptions, MaxSessions>;

        ServerManager g_server_manager;

        void RegisterServiceSessions() {
            R_ABORT_UNLESS(g_server_manager.RegisterObjectForServer(g_pm_service.GetShared(), ProcessManagerServiceName, ProcessManagerMaxSessions));
            R_ABORT_UNLESS(g_server_manager.RegisterObjectForServer(g_shell_service.GetShared(), ShellServiceName, ShellMaxSessions));
            R_ABORT_UNLESS(g_server_manager.RegisterObjectForServer(g_dmnt_service.GetShared(), DebugMonitorServiceName, DebugMonitorMaxSessions));
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

    ams::ldr::InitializeHeap();
}

void __appInit(void) {
    hos::InitializeForStratosphere();

    fs::SetAllocator(ldr::Allocate, ldr::Deallocate);

    /* Initialize services we need. */
    sm::DoWithSession([&]() {
        R_ABORT_UNLESS(fsInitialize());
        lr::Initialize();
        R_ABORT_UNLESS(fsldrInitialize());
        spl::Initialize();
    });

    ams::CheckApiVersion();
}

void __appExit(void) {
    /* Cleanup services. */
    spl::Finalize();
    fsldrExit();
    lr::Finalize();
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
    return ldr::Allocate(size);
}

void operator delete(void *p) {
    return ldr::Deallocate(p, 0);
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
    os::SetThreadNamePointer(os::GetCurrentThread(), AMS_GET_SYSTEM_THREAD_NAME(ldr, Main));
    AMS_ASSERT(os::GetThreadPriority(os::GetCurrentThread()) == AMS_GET_SYSTEM_THREAD_PRIORITY(ldr, Main));

    /* Configure development. */
    /* NOTE: Nintendo really does call the getter function three times instead of caching the value. */
    ldr::SetDevelopmentForAcidProductionCheck(spl::IsDevelopment());
    ldr::SetDevelopmentForAntiDowngradeCheck(spl::IsDevelopment());
    ldr::SetDevelopmentForAcidSignatureCheck(spl::IsDevelopment());

    /* Register the loader services. */
    ldr::RegisterServiceSessions();

    /* Loop forever, servicing our services. */
    ldr::LoopProcess();

	return 0;
}

