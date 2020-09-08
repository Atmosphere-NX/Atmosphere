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
#include "ro_debug_monitor_service.hpp"
#include "ro_ro_service.hpp"

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
}

namespace ams {

    ncm::ProgramId CurrentProgramId = ncm::SystemProgramId::Ro;

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
    hos::InitializeForStratosphere();

    sm::DoWithSession([&]() {
        R_ABORT_UNLESS(setsysInitialize());
        R_ABORT_UNLESS(fsInitialize());
        spl::Initialize();
        if (hos::GetVersion() < hos::Version_3_0_0) {
            R_ABORT_UNLESS(pminfoInitialize());
        }
    });

    R_ABORT_UNLESS(fs::MountSdCard("sdmc"));

    ams::CheckApiVersion();
}

void __appExit(void) {
    fsExit();
    if (hos::GetVersion() < hos::Version_3_0_0) {
        pminfoExit();
    }

    setsysExit();
}

namespace {

    /* ldr:ro, ro:dmnt, ro:1. */
    /* TODO: Consider max sessions enforcement? */
    constexpr size_t NumServers  = 3;
    sf::hipc::ServerManager<NumServers> g_server_manager;

    constexpr sm::ServiceName DebugMonitorServiceName = sm::ServiceName::Encode("ro:dmnt");
    constexpr size_t          DebugMonitorMaxSessions = 2;

    /* NOTE: Official code passes 32 for ldr:ro max sessions. We will pass 2, because that's the actual limit. */
    constexpr sm::ServiceName UserServiceName = sm::ServiceName::Encode("ldr:ro");
    constexpr size_t          UserMaxSessions = 2;

    constexpr sm::ServiceName JitPluginServiceName = sm::ServiceName::Encode("ro:1");
    constexpr size_t          JitPluginMaxSessions = 2;

}

int main(int argc, char **argv)
{
    /* Set thread name. */
    os::SetThreadNamePointer(os::GetCurrentThread(), AMS_GET_SYSTEM_THREAD_NAME(ro, Main));
    AMS_ASSERT(os::GetThreadPriority(os::GetCurrentThread()) == AMS_GET_SYSTEM_THREAD_PRIORITY(ro, Main));

    /* Initialize Debug config. */
    {
        ON_SCOPE_EXIT { spl::Finalize(); };

        ro::SetDevelopmentHardware(spl::IsDevelopment());
        ro::SetDevelopmentFunctionEnabled(spl::IsDevelopmentFunctionEnabled());
    }

    /* Create services. */
    R_ABORT_UNLESS((g_server_manager.RegisterServer<ro::impl::IDebugMonitorInterface, ro::DebugMonitorService>(DebugMonitorServiceName, DebugMonitorMaxSessions)));

    R_ABORT_UNLESS((g_server_manager.RegisterServer<ro::impl::IRoInterface, ro::RoUserService>(UserServiceName, UserMaxSessions)));
    if (hos::GetVersion() >= hos::Version_7_0_0) {
        R_ABORT_UNLESS((g_server_manager.RegisterServer<ro::impl::IRoInterface, ro::RoJitPluginService>(JitPluginServiceName, JitPluginMaxSessions)));
    }

    /* Loop forever, servicing our services. */
    g_server_manager.LoopProcess();

    /* Cleanup */
    return 0;
}

