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
#include "dmnt_service.hpp"
#include "cheat/dmnt_cheat_service.hpp"
#include "cheat/impl/dmnt_cheat_api.hpp"

extern "C" {
    extern u32 __start__;

    u32 __nx_applet_type = AppletType_None;
    u32 __nx_fs_num_sessions = 1;

    /* TODO: Evaluate how much this can be reduced by. */
    #define INNER_HEAP_SIZE 0x20000
    size_t nx_inner_heap_size = INNER_HEAP_SIZE;
    char   nx_inner_heap[INNER_HEAP_SIZE];

    void __libnx_initheap(void);
    void __appInit(void);
    void __appExit(void);
}

namespace ams {

    ncm::ProgramId CurrentProgramId = ncm::SystemProgramId::Dmnt;

    namespace result {

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

    sm::DoWithSession([&]() {
        R_ABORT_UNLESS(pmdmntInitialize());
        R_ABORT_UNLESS(pminfoInitialize());
        R_ABORT_UNLESS(ldrDmntInitialize());
        /* TODO: We provide this on every sysver via ro. Do we need a shim? */
        if (hos::GetVersion() >= hos::Version_3_0_0) {
            R_ABORT_UNLESS(roDmntInitialize());
        }
        R_ABORT_UNLESS(nsdevInitialize());
        lr::Initialize();
        R_ABORT_UNLESS(setInitialize());
        R_ABORT_UNLESS(setsysInitialize());
        R_ABORT_UNLESS(hidInitialize());
        R_ABORT_UNLESS(fsInitialize());
    });

    R_ABORT_UNLESS(fs::MountSdCard("sdmc"));

    ams::CheckApiVersion();
}

void __appExit(void) {
    /* Cleanup services. */
    fsExit();
    hidExit();
    setsysExit();
    setExit();
    lr::Finalize();
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

    os::ThreadType g_extra_threads[NumExtraThreads];

}

int main(int argc, char **argv)
{
    /* Set thread name. */
    os::SetThreadNamePointer(os::GetCurrentThread(), AMS_GET_SYSTEM_THREAD_NAME(dmnt, Main));
    AMS_ASSERT(os::GetThreadPriority(os::GetCurrentThread()) == AMS_GET_SYSTEM_THREAD_PRIORITY(dmnt, Main));

    /* Initialize the cheat manager. */
    ams::dmnt::cheat::impl::InitializeCheatManager();

    /* Create services. */
    /* TODO: Implement rest of dmnt:- in ams.tma development branch. */
    /* R_ABORT_UNLESS((g_server_manager.RegisterServer<dmnt::cheat::CheatService>(DebugMonitorServiceName, DebugMonitorMaxSessions))); */
    R_ABORT_UNLESS((g_server_manager.RegisterServer<dmnt::cheat::impl::ICheatInterface, dmnt::cheat::CheatService>(CheatServiceName, CheatMaxSessions)));

    /* Loop forever, servicing our services. */
    /* Nintendo loops four threads processing on the manager -- we'll loop an extra fifth for our cheat service. */
    {

        /* Initialize threads. */
        if constexpr (NumExtraThreads > 0) {
            static_assert(AMS_GET_SYSTEM_THREAD_PRIORITY(dmnt, Main) == AMS_GET_SYSTEM_THREAD_PRIORITY(dmnt, Ipc));
            for (size_t i = 0; i < NumExtraThreads; i++) {
                R_ABORT_UNLESS(os::CreateThread(std::addressof(g_extra_threads[i]), LoopServerThread, nullptr, g_extra_thread_stacks[i], ThreadStackSize, AMS_GET_SYSTEM_THREAD_PRIORITY(dmnt, Ipc)));
                os::SetThreadNamePointer(std::addressof(g_extra_threads[i]), AMS_GET_SYSTEM_THREAD_NAME(dmnt, Ipc));
            }
        }

        /* Start extra threads. */
        if constexpr (NumExtraThreads > 0) {
            for (size_t i = 0; i < NumExtraThreads; i++) {
                os::StartThread(std::addressof(g_extra_threads[i]));
            }
        }

        /* Loop this thread. */
        LoopServerThread(nullptr);

        /* Wait for extra threads to finish. */
        if constexpr (NumExtraThreads > 0) {
            for (size_t i = 0; i < NumExtraThreads; i++) {
                os::WaitThread(std::addressof(g_extra_threads[i]));
            }
        }
    }

    return 0;
}

