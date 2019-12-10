/*
 * Copyright (c) 2019 Adubbz
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

#include "impl/ncm_content_manager.hpp"
#include "lr_manager_service.hpp"
#include "ncm_content_manager_service.hpp"

extern "C" {
    extern u32 __start__;

    u32 __nx_applet_type = AppletType_None;

    #define INNER_HEAP_SIZE 0x100000
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

    ncm::ProgramId CurrentProgramId = ncm::ProgramId::Ncm;

    namespace result {

        bool CallFatalOnResultAssertion = false;

    }

}

using namespace ams;

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
    hos::SetVersionForLibnx();

    sm::DoWithSession([&]() {
        R_ASSERT(fsInitialize());
    });

    ams::CheckApiVersion();
}

void __appExit(void) {
    /* Cleanup services. */
    fsdevUnmountAll();
    fsExit();
}

namespace {

    struct NcmServerOptions {
        static constexpr size_t PointerBufferSize = 0x400;
        static constexpr size_t MaxDomains = 0;
        static constexpr size_t MaxDomainObjects = 0;
    };

    constexpr sm::ServiceName NcmServiceName = sm::ServiceName::Encode("ncm");
    constexpr size_t          NcmMaxSessions = 16;
    constexpr size_t          NcmNumServers = 1;

    constexpr sm::ServiceName LrServiceName = sm::ServiceName::Encode("lr");
    constexpr size_t          LrMaxSessions = 16;
    constexpr size_t          LrNumServers = 1;

    sf::hipc::ServerManager<NcmNumServers, NcmServerOptions, NcmMaxSessions> g_ncm_server_manager;
    sf::hipc::ServerManager<LrNumServers, NcmServerOptions, LrMaxSessions> g_lr_server_manager;
}

void ContentManagerServerMain(void* arg) {
    /* Create services. */
    R_ASSERT(g_ncm_server_manager.RegisterServer<ncm::ContentManagerService>(NcmServiceName, NcmMaxSessions));

    /* Loop forever, servicing our services. */
    g_ncm_server_manager.LoopProcess();
}

void LocationResolverServerMain(void* arg) {
    /* Create services. */
    R_ASSERT(g_lr_server_manager.RegisterServer<lr::LocationResolverManagerService>(LrServiceName, LrMaxSessions));

    /* Loop forever, servicing our services. */
    g_lr_server_manager.LoopProcess();
}

int main(int argc, char **argv)
{
    /* Initialize content manager implementation. */
    R_ASSERT(ams::ncm::impl::InitializeContentManager());

    static os::Thread s_content_manager_thread;
    static os::Thread s_location_resolver_thread;

    R_ASSERT(s_content_manager_thread.Initialize(&ContentManagerServerMain, nullptr, 0x4000, 0x15));
    R_ASSERT(s_content_manager_thread.Start());

    R_ASSERT(s_location_resolver_thread.Initialize(&LocationResolverServerMain, nullptr, 0x4000, 0x15));
    R_ASSERT(s_location_resolver_thread.Start());

    s_content_manager_thread.Join();
    s_location_resolver_thread.Join();

    ams::ncm::impl::FinalizeContentManager();
    
    return 0;
}
