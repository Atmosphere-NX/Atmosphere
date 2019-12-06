/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include "dmnt_service.hpp"
#include "cheat/dmnt_cheat_service.hpp"

extern "C" {
    extern u32 __start__;

    u32 __nx_applet_type = AppletType_None;
    u32 __nx_fs_num_sessions = 1;
    u32 __nx_fsdev_direntry_cache_size = 1;

    /* TODO: Evaluate how much this can be reduced by. */
    #define INNER_HEAP_SIZE 0x20000
    size_t nx_inner_heap_size = INNER_HEAP_SIZE;
    char   nx_inner_heap[INNER_HEAP_SIZE];

    void __libnx_initheap(void);
    void __appInit(void);
    void __appExit(void);
}

namespace ams {

    ncm::ProgramId CurrentProgramId = ncm::ProgramId::Dmnt;

    namespace result {

        bool CallFatalOnResultAssertion = true;

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
    hos::SetVersionForLibnx();

    sm::DoWithSession([&]() {
        R_ASSERT(pmdmntInitialize());
        R_ASSERT(pminfoInitialize());
        R_ASSERT(ldrDmntInitialize());
        /* TODO: We provide this on every sysver via ro. Do we need a shim? */
        if (hos::GetVersion() >= hos::Version_300) {
            R_ASSERT(roDmntInitialize());
        }
        R_ASSERT(nsdevInitialize());
        R_ASSERT(lrInitialize());
        R_ASSERT(setInitialize());
        R_ASSERT(setsysInitialize());
        R_ASSERT(hidInitialize());
        R_ASSERT(fsInitialize());
    });

    R_ASSERT(fsdevMountSdmc());

    ams::CheckApiVersion();
}

void __appExit(void) {
    /* Cleanup services. */
    fsdevUnmountAll();
    fsExit();
    hidExit();
    setsysExit();
    setExit();
    lrExit();
    nsdevExit();
    roDmntExit();
    ldrDmntExit();
    pminfoExit();
    pmdmntExit();
}

namespace {

    using ServerOptions = sf::hipc::DefaultServerManagerOptions;

    constexpr sm::ServiceName DebugMonitorServiceName = sm::ServiceName::Encode("dmnt:-");
    constexpr size_t          DebugMonitorMaxSessions = 4;

    constexpr sm::ServiceName CheatServiceName = sm::ServiceName::Encode("dmnt:cht");
    constexpr size_t          CheatMaxSessions = 2;

    /* dmnt:-, dmnt:cht. */
    constexpr size_t NumServers  = 2;
    constexpr size_t NumSessions = DebugMonitorMaxSessions + CheatMaxSessions;

    sf::hipc::ServerManager<NumServers, ServerOptions, NumSessions> g_server_manager;

    void LoopServerThread(void *arg) {
        g_server_manager.LoopProcess();
    }

    constexpr size_t TotalThreads = DebugMonitorMaxSessions + 1;
    static_assert(TotalThreads >= 1, "TotalThreads");
    constexpr size_t NumExtraThreads = TotalThreads - 1;
    constexpr size_t ThreadStackSize = 0x4000;
    alignas(os::MemoryPageSize) u8 g_extra_thread_stacks[NumExtraThreads][ThreadStackSize];

    os::Thread g_extra_threads[NumExtraThreads];

}

int main(int argc, char **argv)
{
    /* Create services. */
    /* TODO: Implement rest of dmnt:- in ams.tma development branch. */
    /* R_ASSERT((g_server_manager.RegisterServer<dmnt::cheat::CheatService>(DebugMonitorServiceName, DebugMonitorMaxSessions))); */
    R_ASSERT((g_server_manager.RegisterServer<dmnt::cheat::CheatService>(CheatServiceName, CheatMaxSessions)));

    /* Loop forever, servicing our services. */
    /* Nintendo loops four threads processing on the manager -- we'll loop an extra fifth for our cheat service. */
    {

        /* Initialize threads. */
        if constexpr (NumExtraThreads > 0) {
            const u32 priority = os::GetCurrentThreadPriority();
            for (size_t i = 0; i < NumExtraThreads; i++) {
                R_ASSERT(g_extra_threads[i].Initialize(LoopServerThread, nullptr, g_extra_thread_stacks[i], ThreadStackSize, priority));
            }
        }

        /* Start extra threads. */
        if constexpr (NumExtraThreads > 0) {
            for (size_t i = 0; i < NumExtraThreads; i++) {
                R_ASSERT(g_extra_threads[i].Start());
            }
        }

        /* Loop this thread. */
        LoopServerThread(nullptr);

        /* Wait for extra threads to finish. */
        if constexpr (NumExtraThreads > 0) {
            for (size_t i = 0; i < NumExtraThreads; i++) {
                R_ASSERT(g_extra_threads[i].Join());
            }
        }
    }

    return 0;
}

