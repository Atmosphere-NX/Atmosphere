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

    #define INNER_HEAP_SIZE 0x240000
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
}

namespace ams {

    ncm::ProgramId CurrentProgramId = ncm::SystemProgramId::Fatal;

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
    hos::InitializeForStratosphere();

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
    R_ABORT_UNLESS((g_server_manager.RegisterServer<fatal::impl::IPrivateService, fatal::srv::PrivateService>(PrivateServiceName, PrivateMaxSessions)));
    R_ABORT_UNLESS((g_server_manager.RegisterServer<fatal::impl::IService, fatal::srv::Service>(UserServiceName, UserMaxSessions)));

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

