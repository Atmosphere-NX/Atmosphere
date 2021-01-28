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
#include "fatal_service.hpp"
#include "fatal_config.hpp"
#include "fatal_repair.hpp"
#include "fatal_font.hpp"

extern "C" {
    extern u32 __start__;

    u32 __nx_applet_type = AppletType_None;
    u32 __nx_fs_num_sessions = 1;

    #define INNER_HEAP_SIZE 0x0
    size_t nx_inner_heap_size = INNER_HEAP_SIZE;
    char   nx_inner_heap[INNER_HEAP_SIZE];

    u32 __nx_nv_transfermem_size = 0x40000;
    ViLayerFlags __nx_vi_stray_layer_flags = (ViLayerFlags)0;

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

    ncm::ProgramId CurrentProgramId = ncm::SystemProgramId::Fatal;

}

using namespace ams;

namespace ams::fatal {

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
            g_fs_heap_handle = lmem::CreateExpHeap(g_fs_heap_memory, sizeof(g_fs_heap_memory), lmem::CreateOption_ThreadSafe);
        }

    }

}

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

void __appInit(void) {
    hos::InitializeForStratosphere();

    fatal::InitializeFsHeap();
    fs::SetAllocator(fatal::AllocateForFs, fatal::DeallocateForFs);

    sm::DoWithSession([&]() {
        R_ABORT_UNLESS(setInitialize());
        R_ABORT_UNLESS(setsysInitialize());
        R_ABORT_UNLESS(pminfoInitialize());
        R_ABORT_UNLESS(i2cInitialize());
        R_ABORT_UNLESS(bpcInitialize());

        if (hos::GetVersion() >= hos::Version_8_0_0) {
            R_ABORT_UNLESS(clkrstInitialize());
        } else {
            R_ABORT_UNLESS(pcvInitialize());
        }

        R_ABORT_UNLESS(lblInitialize());
        R_ABORT_UNLESS(psmInitialize());
        R_ABORT_UNLESS(spsmInitialize());
        R_ABORT_UNLESS(plInitialize(::PlServiceType_User));
        gpio::Initialize();
        R_ABORT_UNLESS(fsInitialize());
    });

    R_ABORT_UNLESS(fs::MountSdCard("sdmc"));

    /* fatal cannot throw fatal, so don't do: ams::CheckApiVersion(); */
}

void __appExit(void) {
    /* Cleanup services. */
    fsExit();
    plExit();
    gpio::Finalize();
    spsmExit();
    psmExit();
    lblExit();
    if (hos::GetVersion() >= hos::Version_8_0_0) {
        clkrstExit();
    } else {
        pcvExit();
    }
    bpcExit();
    i2cExit();
    pminfoExit();
    setsysExit();
    setExit();
}

namespace {

    using ServerOptions = sf::hipc::DefaultServerManagerOptions;

    constexpr sm::ServiceName UserServiceName = sm::ServiceName::Encode("fatal:u");
    constexpr size_t          UserMaxSessions = 4;

    constexpr sm::ServiceName PrivateServiceName = sm::ServiceName::Encode("fatal:p");
    constexpr size_t          PrivateMaxSessions = 4;

    /* fatal:u, fatal:p. */
    constexpr size_t NumServers  = 2;
    constexpr size_t NumSessions = UserMaxSessions + PrivateMaxSessions;

    sf::hipc::ServerManager<NumServers, ServerOptions, NumSessions> g_server_manager;

    constinit sf::UnmanagedServiceObject<fatal::impl::IService, fatal::srv::Service> g_user_service_object;
    constinit sf::UnmanagedServiceObject<fatal::impl::IPrivateService, fatal::srv::Service> g_private_service_object;

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
    /* Disable auto-abort in fs operations. */
    fs::SetEnabledAutoAbort(false);

    /* Set thread name. */
    os::SetThreadNamePointer(os::GetCurrentThread(), AMS_GET_SYSTEM_THREAD_NAME(fatal, Main));
    AMS_ASSERT(os::GetThreadPriority(os::GetCurrentThread()) == AMS_GET_SYSTEM_THREAD_PRIORITY(fatal, Main));

    /* Load shared font. */
    R_ABORT_UNLESS(fatal::srv::font::InitializeSharedFont());

    /* Check whether we should throw fatal due to repair process. */
    fatal::srv::CheckRepairStatus();

    /* Create services. */
    R_ABORT_UNLESS(g_server_manager.RegisterObjectForServer(g_user_service_object.GetShared(),    UserServiceName, UserMaxSessions));
    R_ABORT_UNLESS(g_server_manager.RegisterObjectForServer(g_private_service_object.GetShared(), PrivateServiceName, PrivateMaxSessions));

    /* Add dirty event holder. */
    auto *dirty_event_holder = ams::fatal::srv::GetFatalDirtyWaitableHolder();
    g_server_manager.AddUserWaitableHolder(dirty_event_holder);

    /* Loop forever, servicing our services. */
    /* Because fatal has a user wait holder, we need to specify how to process manually. */
    while (auto *signaled_holder = g_server_manager.WaitSignaled()) {
        if (signaled_holder == dirty_event_holder) {
            /* Dirty event holder was signaled. */
            fatal::srv::OnFatalDirtyEvent();
            g_server_manager.AddUserWaitableHolder(signaled_holder);
        } else {
            /* A server/session was signaled. Have the manager handle it. */
            R_ABORT_UNLESS(g_server_manager.Process(signaled_holder));
        }
    }

    return 0;
}

