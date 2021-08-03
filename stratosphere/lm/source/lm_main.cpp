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
#include "lm_log_service.hpp"
#include "lm_log_getter.hpp"
#include "lm_threads.hpp"

extern "C" {
    extern u32 __start__;

    u32 __nx_applet_type = AppletType_None;
    u32 __nx_fs_num_sessions = 1;
    
    #define INNER_HEAP_SIZE 0x0
    size_t nx_inner_heap_size = INNER_HEAP_SIZE; // 0x8000
    char nx_inner_heap[INNER_HEAP_SIZE];

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

    ncm::ProgramId CurrentProgramId = ncm::SystemProgramId::LogManager;

    namespace result {

        /* Fatal is launched after we are launched, so disable this. */
        bool CallFatalOnResultAssertion = false;

    }

}

using namespace ams;

namespace {

    /* lm, lm:get. */
    enum PortIndex {
        PortIndex_LogService,
        PortIndex_LogGetter,
        PortIndex_Count,
    };

    /* TODO: these domain/domain object amounts work fine, but which ones does N actually use? */
    struct ServerOptions {
        static constexpr size_t PointerBufferSize = 0x400;
        static constexpr size_t MaxDomains = 0x40;
        static constexpr size_t MaxDomainObjects = 0x200;
    };

    constexpr sm::ServiceName LogServiceName = sm::ServiceName::Encode("lm");
    constexpr size_t          LogServiceMaxSessions = 30;

    constexpr sm::ServiceName LogGetterServiceName = sm::ServiceName::Encode("lm:get");
    constexpr size_t          LogGetterServiceMaxSessions = 1;

    static constexpr size_t MaxSessions = LogServiceMaxSessions + LogGetterServiceMaxSessions;

    class ServerManager final : public sf::hipc::ServerManager<PortIndex_Count, ServerOptions, MaxSessions> {
        private:
            virtual ams::Result OnNeedsToAccept(int port_index, Server *server) override;
    };

    using Allocator     = sf::ExpHeapAllocator;
    using ObjectFactory = sf::ObjectFactory<sf::ExpHeapAllocator::Policy>;

    alignas(0x40) constinit u8 g_server_allocator_buffer[4_KB];
    lmem::HeapHandle g_server_heap_handle;
    Allocator g_server_allocator;

    ServerManager g_server_manager;

    ams::Result ServerManager::OnNeedsToAccept(int port_index, Server *server) {
        switch (port_index) {
            case PortIndex_LogService:
                return this->AcceptImpl(server, ObjectFactory::CreateSharedEmplaced<lm::impl::ILogService, lm::LogService>(std::addressof(g_server_allocator)));
            case PortIndex_LogGetter:
                return this->AcceptImpl(server, ObjectFactory::CreateSharedEmplaced<lm::impl::ILogGetter, lm::LogGetter>(std::addressof(g_server_allocator)));
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    psc::PmModule g_pm_module;
    os::WaitableHolderType g_pm_module_waitable_holder;

}

void __libnx_exception_handler(ThreadExceptionDump *ctx) {
    ams::CrashHandler(ctx);
}

void __libnx_initheap(void) {
	void  *addr = nx_inner_heap;
	size_t size = nx_inner_heap_size;

	/* Newlib */
	extern char* fake_heap_start;
	extern char* fake_heap_end;

	fake_heap_start = (char*)addr;
	fake_heap_end   = (char*)addr + size;

    /* Setup server allocator. */
    g_server_heap_handle = lmem::CreateExpHeap(g_server_allocator_buffer, sizeof(g_server_allocator_buffer), lmem::CreateOption_None);
}

void __appInit(void) {
    hos::InitializeForStratosphere();

    /* Initialize services. */
    R_ABORT_UNLESS(sm::Initialize());
    R_ABORT_UNLESS(setsysInitialize());
    R_ABORT_UNLESS(pscmInitialize());
    R_ABORT_UNLESS(fsInitialize());
    R_ABORT_UNLESS(time::Initialize());

    ams::CheckApiVersion();
}

void __appExit(void) {
    /* Cleanup services. */
    pscmExit();
    setsysExit();
}

/*
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
*/

extern std::atomic_bool g_disabled_flush;

namespace {

    void StartAndLoopProcess() {
        /* Get our psc:m module to handle requests. */
        const psc::PmModuleId dependencies[] = { psc::PmModuleId_Fs /*, psc::PmModuleId_TmaHostIo*/ };
        R_ABORT_UNLESS(g_pm_module.Initialize(psc::PmModuleId_Lm, dependencies, util::size(dependencies), os::EventClearMode_ManualClear));

        os::InitializeWaitableHolder(std::addressof(g_pm_module_waitable_holder), g_pm_module.GetEventPointer()->GetBase());
        os::SetWaitableHolderUserData(std::addressof(g_pm_module_waitable_holder), static_cast<uintptr_t>(psc::PmModuleId_Lm));

        g_server_manager.AddUserWaitableHolder(std::addressof(g_pm_module_waitable_holder));
        
        /* Invalid initial state values. */
        psc::PmState prev_state = (psc::PmState)6;
        psc::PmState cur_state = (psc::PmState)6;
        psc::PmFlagSet pm_flags;
        while (true) {
            auto *signaled_holder = g_server_manager.WaitSignaled();
            if (signaled_holder != std::addressof(g_pm_module_waitable_holder)) {
                /* Process IPC requests. */
                R_ABORT_UNLESS(g_server_manager.Process(signaled_holder));
            }
            else {
                /* Handle our module. */
                g_pm_module.GetEventPointer()->Clear();
                if (R_SUCCEEDED(g_pm_module.GetRequest(std::addressof(cur_state), std::addressof(pm_flags)))) {
                    if (((prev_state == psc::PmState_ReadyAwakenCritical) && (cur_state == psc::PmState_ReadyAwaken)) || ((prev_state == psc::PmState_ReadyAwaken) && (cur_state == psc::PmState_ReadySleep)) || (cur_state == psc::PmState_ReadyShutdown)) {
                        /* State changed, so allow/disallow flushing. */
                        g_disabled_flush = !g_disabled_flush;
                    }
                    R_ABORT_UNLESS(g_pm_module.Acknowledge(cur_state, ResultSuccess()));
                    prev_state = cur_state;
                }
                g_server_manager.AddUserWaitableHolder(signaled_holder);
            }
        }
    }

}

int main(int argc, char **argv) {
    /* Set thread name. */
    os::SetThreadNamePointer(os::GetCurrentThread(), AMS_GET_SYSTEM_THREAD_NAME(lm, IpcServer));
    AMS_ASSERT(os::GetThreadPriority(os::GetCurrentThread()) == AMS_GET_SYSTEM_THREAD_PRIORITY(lm, IpcServer));

    /* Start Htcs and Flush threads. */
    lm::StartHtcsThread();
    lm::StartFlushThread();

    /* Attach the server allocator. */
    g_server_allocator.Attach(g_server_heap_handle);

    /* Register lm. */
    R_ABORT_UNLESS(g_server_manager.RegisterServer(PortIndex_LogService, LogServiceName, LogServiceMaxSessions));
    
    /* Register lm:get. */
    R_ABORT_UNLESS(g_server_manager.RegisterServer(PortIndex_LogGetter, LogGetterServiceName, LogGetterServiceMaxSessions));
    
    /* Loop forever, servicing our services. */
    StartAndLoopProcess();

    /* Dispose threads. */
    lm::DisposeFlushThread();
    lm::DisposeHtcsThread();
 
    /* Cleanup. */
    return 0;
}