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

#include <switch.h>
#include <atmosphere.h>
#include <stratosphere.hpp>

#include "impl/ncm_content_manager.hpp"
#include "lr_manager_service.hpp"
#include "ncm_content_manager_service.hpp"

extern "C" {
    extern u32 __start__;

    u32 __nx_applet_type = AppletType_None;

    #define INNER_HEAP_SIZE 0x60000
    size_t nx_inner_heap_size = INNER_HEAP_SIZE;
    char   nx_inner_heap[INNER_HEAP_SIZE];
    
    void __libnx_initheap(void);
    void __appInit(void);
    void __appExit(void);

    /* Exception handling. */
    alignas(16) u8 __nx_exception_stack[0x1000];
    u64 __nx_exception_stack_size = sizeof(__nx_exception_stack);
    void __libnx_exception_handler(ThreadExceptionDump *ctx);
    void __libstratosphere_exception_handler(AtmosphereFatalErrorContext *ctx);
}

sts::ncm::TitleId __stratosphere_title_id = sts::ncm::TitleId::Ncm;

void __libnx_exception_handler(ThreadExceptionDump *ctx) {
    StratosphereCrashHandler(ctx);
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
    SetFirmwareVersionForLibnx();

    DoWithSmSession([&]() {
        R_ASSERT(fsInitialize());
    });

    CheckAtmosphereVersion(CURRENT_ATMOSPHERE_VERSION);
}

void __appExit(void) {
    /* Cleanup services. */
    fsdevUnmountAll();
    fsExit();
}

struct ServerOptions {
    static constexpr size_t PointerBufferSize = 0x400;
    static constexpr size_t MaxDomains = 0;
    static constexpr size_t MaxDomainObjects = 0;
};

void ContentManagerServerMain(void* arg) {
    static auto s_server_manager = WaitableManager<ServerOptions>(1);
    
    /* Create services. */
    s_server_manager.AddWaitable(new ServiceServer<sts::ncm::ContentManagerService>("ncm", 0x10));

    /* Loop forever, servicing our services. */
    s_server_manager.Process();
}

void LocationResolverServerMain(void* arg) {
    static auto s_server_manager = WaitableManager<ServerOptions>(1);
    
    /* Create services. */
    s_server_manager.AddWaitable(new ServiceServer<sts::lr::LocationResolverManagerService>("lr", 0x10));

    /* Loop forever, servicing our services. */
    s_server_manager.Process();
}

int main(int argc, char **argv)
{
    /* Initialize content manager implementation. */
    R_ASSERT(sts::ncm::impl::InitializeContentManager());

    static HosThread s_content_manager_thread;
    static HosThread s_location_resolver_thread;

    R_ASSERT(s_content_manager_thread.Initialize(&ContentManagerServerMain, nullptr, 0x4000, 0x15));
    R_ASSERT(s_content_manager_thread.Start());

    R_ASSERT(s_location_resolver_thread.Initialize(&LocationResolverServerMain, nullptr, 0x4000, 0x15));
    R_ASSERT(s_location_resolver_thread.Start());

    s_content_manager_thread.Join();
    s_location_resolver_thread.Join();
    
    return 0;
}