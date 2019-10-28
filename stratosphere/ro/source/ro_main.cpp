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
#include "ro_debug_monitor.hpp"
#include "ro_service.hpp"

extern "C" {
    extern u32 __start__;

    u32 __nx_applet_type = AppletType_None;
    u32 __nx_fs_num_sessions = 1;
    u32 __nx_fsdev_direntry_cache_size = 1;

    #define INNER_HEAP_SIZE 0x4000
    size_t nx_inner_heap_size = INNER_HEAP_SIZE;
    char   nx_inner_heap[INNER_HEAP_SIZE];

    void __libnx_initheap(void);
    void __appInit(void);
    void __appExit(void);
}

namespace ams {

    ncm::ProgramId CurrentProgramId = ncm::ProgramId::Ro;

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
        R_ASSERT(setsysInitialize());
        R_ASSERT(fsInitialize());
        R_ASSERT(splInitialize());
        if (hos::GetVersion() < hos::Version_300) {
            R_ASSERT(pminfoInitialize());
        }
    });

    R_ASSERT(fsdevMountSdmc());

    ams::CheckApiVersion();
}

void __appExit(void) {
    fsdevUnmountAll();
    fsExit();
    if (hos::GetVersion() < hos::Version_300) {
        pminfoExit();
    }
    setsysExit();
}

/* Helpers to create RO objects. */
static constexpr auto MakeRoServiceForSelf = []() { return std::make_shared<ro::Service>(ro::ModuleType::ForSelf); };
static constexpr auto MakeRoServiceForOthers = []() { return std::make_shared<ro::Service>(ro::ModuleType::ForOthers); };

namespace {

    /* ldr:ro, ro:dmnt, ro:1. */
    /* TODO: Consider max sessions enforcement? */
    constexpr size_t NumServers  = 3;
    sf::hipc::ServerManager<NumServers> g_server_manager;

    constexpr sm::ServiceName DebugMonitorServiceName = sm::ServiceName::Encode("ro:dmnt");
    constexpr size_t          DebugMonitorMaxSessions = 2;

    /* NOTE: Official code passes 32 for ldr:ro max sessions. We will pass 2, because that's the actual limit. */
    constexpr sm::ServiceName ForSelfServiceName = sm::ServiceName::Encode("ldr:ro");
    constexpr size_t          ForSelfMaxSessions = 2;

    constexpr sm::ServiceName ForOthersServiceName = sm::ServiceName::Encode("ro:1");
    constexpr size_t          ForOthersMaxSessions = 2;

}

int main(int argc, char **argv)
{
    /* Initialize Debug config. */
    {
        ON_SCOPE_EXIT { splExit(); };

        ro::SetDevelopmentHardware(spl::IsDevelopmentHardware());
        ro::SetDevelopmentFunctionEnabled(spl::IsDevelopmentFunctionEnabled());
    }

    /* Create services. */
    R_ASSERT((g_server_manager.RegisterServer<ro::DebugMonitorService>(DebugMonitorServiceName, DebugMonitorMaxSessions)));

    R_ASSERT((g_server_manager.RegisterServer<ro::Service, +MakeRoServiceForSelf>(ForSelfServiceName, ForSelfMaxSessions)));
    if (hos::GetVersion() >= hos::Version_700) {
        R_ASSERT((g_server_manager.RegisterServer<ro::Service, +MakeRoServiceForOthers>(ForOthersServiceName, ForOthersMaxSessions)));
    }

    /* Loop forever, servicing our services. */
    g_server_manager.LoopProcess();

    /* Cleanup */
    return 0;
}

