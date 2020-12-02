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
#include "lm_log_service.hpp"
#include "lm_log_getter.hpp"
#include "lm_threads.hpp"

extern "C" {
    extern u32 __start__;

    u32 __nx_applet_type = AppletType_None;
    u32 __nx_fs_num_sessions = 1;
    
    #define INNER_HEAP_SIZE 0x8000
    size_t nx_inner_heap_size = INNER_HEAP_SIZE;
    char nx_inner_heap[INNER_HEAP_SIZE];

    void __libnx_initheap(void);
    void __appInit(void);
    void __appExit(void);
}

namespace ams {

    ncm::ProgramId CurrentProgramId = ncm::SystemProgramId::LogManager;

    namespace result {

        /* Fatal is launched after we are launched, so disable this. */
        bool CallFatalOnResultAssertion = false;

    }

}

using namespace ams;

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

    /* Initialize services. */
    sm::DoWithSession([]() {
        R_ABORT_UNLESS(setsysInitialize());
        R_ABORT_UNLESS(pscmInitialize());
        R_ABORT_UNLESS(fsInitialize());
    });

    ams::CheckApiVersion();
}

void __appExit(void) {
    /* Cleanup services. */
    fsExit();
    pscmExit();
    setsysExit();
}

namespace {

    /* TODO: these domain/domain object amounts work fine, but which ones does N actually use? */
    struct ServerOptions {
        static constexpr size_t PointerBufferSize = 0x400;
        static constexpr size_t MaxDomains = 0x40;
        static constexpr size_t MaxDomainObjects = 0x200;
    };

    constexpr sm::ServiceName LogServiceName = sm::ServiceName::Encode("lm");
    constexpr size_t LogServiceMaxSessions = 30;

    constexpr sm::ServiceName LogGetterServiceName = sm::ServiceName::Encode("lm:get");
    constexpr size_t LogGetterServiceMaxSessions = 1;

    /* lm, lm:get */
    constexpr size_t NumServers = 2;
    constexpr size_t MaxSessions = LogServiceMaxSessions + LogGetterServiceMaxSessions;
    sf::hipc::ServerManager<NumServers, ServerOptions, MaxSessions> g_server_manager;

    psc::PmModule g_pm_module;
    os::WaitableHolderType g_pm_module_waitable_holder;

}

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
                if(R_SUCCEEDED(g_pm_module.GetRequest(std::addressof(cur_state), std::addressof(pm_flags)))) {
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

    /* Register lm. */
    R_ABORT_UNLESS((g_server_manager.RegisterServer<lm::impl::ILogService, lm::LogService>(LogServiceName, LogServiceMaxSessions)));
    
    /* Register lm:get. */
    R_ABORT_UNLESS((g_server_manager.RegisterServer<lm::impl::ILogGetter, lm::LogGetter>(LogGetterServiceName, LogGetterServiceMaxSessions)));
    
    /* Loop forever, servicing our services. */
    StartAndLoopProcess();

    /* Dispose threads. */
    lm::DisposeFlushThread();
    lm::DisposeHtcsThread();
 
    return 0;
}