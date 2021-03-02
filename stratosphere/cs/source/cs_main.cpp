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

    void *__libnx_alloc(size_t size);
    void *__libnx_aligned_alloc(size_t alignment, size_t size);
    void __libnx_free(void *mem);
}

namespace ams {

    ncm::ProgramId CurrentProgramId = ncm::SystemProgramId::Cs;

}

using namespace ams;

#define AMS_CS_SERVER_USE_FATAL_ERROR 1

#if AMS_CS_SERVER_USE_FATAL_ERROR

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

namespace ams::cs {

    namespace {

        alignas(os::ThreadStackAlignment) u8 g_shell_stack[4_KB];
        alignas(os::ThreadStackAlignment) u8 g_runner_stack[4_KB];

        alignas(os::MemoryPageSize) u8 g_heap_memory[32_KB];

        alignas(0x40) u8 g_htcs_buffer[1_KB];

        lmem::HeapHandle g_heap_handle;

        void *Allocate(size_t size) {
            void *mem = lmem::AllocateFromExpHeap(g_heap_handle, size);
            return mem;
        }

        void Deallocate(void *p, size_t size) {
            lmem::FreeToExpHeap(g_heap_handle, p);
        }

        void InitializeHeap() {
            g_heap_handle = lmem::CreateExpHeap(g_heap_memory, sizeof(g_heap_memory), lmem::CreateOption_ThreadSafe);
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

    cs::InitializeHeap();
}

void __appInit(void) {
    hos::InitializeForStratosphere();

    fs::SetAllocator(cs::Allocate, cs::Deallocate);

    sm::DoWithSession([&]() {
        R_ABORT_UNLESS(fsInitialize());
        lr::Initialize();
        R_ABORT_UNLESS(ldr::InitializeForShell());
        R_ABORT_UNLESS(pgl::Initialize());
        /* TODO: Other services? */
    });

    ams::CheckApiVersion();
}

void __appExit(void) {
    /* TODO: Other services? */
    pgl::Finalize();
    ldr::FinalizeForShell();
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

namespace ams::cs {

    namespace {

        constinit ::ams::cs::CommandProcessor g_command_processor;

        constinit ::ams::scs::ShellServer g_shell_server;
        constinit ::ams::scs::ShellServer g_runner_server;

        constinit sf::UnmanagedServiceObject<htc::tenv::IServiceManager, htc::tenv::ServiceManager> g_tenv_service_manager;

    }

}

int main(int argc, char **argv)
{
    using namespace ams::cs;

    /* Set thread name. */
    os::SetThreadNamePointer(os::GetCurrentThread(), AMS_GET_SYSTEM_THREAD_NAME(cs, Main));
    AMS_ASSERT(os::GetThreadPriority(os::GetCurrentThread()) == AMS_GET_SYSTEM_THREAD_PRIORITY(cs, Main));

    /* Initialize htcs. */
    constexpr auto HtcsSocketCountMax = 6;
    const size_t buffer_size = htcs::GetWorkingMemorySize(2 * HtcsSocketCountMax);
    AMS_ABORT_UNLESS(sizeof(g_htcs_buffer) >= buffer_size);
    htcs::InitializeForSystem(g_htcs_buffer, buffer_size, HtcsSocketCountMax);

    /* Initialize audio server. */
    cs::InitializeAudioServer();

    /* Initialize remote video server. */
    cs::InitializeRemoteVideoServer();

    /* Initialize hid server. */
    cs::InitializeHidServer();

    /* Initialize target io server. */
    cs::InitializeTargetIoServer();

    /* Initialize command processor. */
    g_command_processor.Initialize();

    /* Setup scs. */
    scs::InitializeShell();

    /* Setup target environment service. */
    scs::InitializeTenvServiceManager();

    /* Initialize the shell servers. */
    g_shell_server.Initialize("iywys@$cs", g_shell_stack, sizeof(g_shell_stack), std::addressof(g_command_processor));
    g_shell_server.Start();

    g_runner_server.Initialize("iywys@$csForRunnerTools", g_runner_stack, sizeof(g_runner_stack), std::addressof(g_command_processor));
    g_runner_server.Start();

    /* Register htc:tenv. */
    R_ABORT_UNLESS(scs::GetServerManager()->RegisterObjectForServer(g_tenv_service_manager.GetShared(), htc::tenv::ServiceName, scs::SessionCount[scs::Port_HtcTenv]));

    /* Start the scs ipc server. */
    scs::StartServer();

    return 0;
}
