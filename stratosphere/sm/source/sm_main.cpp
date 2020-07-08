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
#include "sm_user_service.hpp"
#include "sm_manager_service.hpp"
#include "sm_debug_monitor_service.hpp"
#include "impl/sm_service_manager.hpp"

extern "C" {
    extern u32 __start__;

    u32 __nx_applet_type = AppletType_None;

    #define INNER_HEAP_SIZE 0x4000
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

    ncm::ProgramId CurrentProgramId = ncm::SystemProgramId::Sm;

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

    /* We must do no service setup here, because we are sm. */
}

void __appExit(void) {
    /* Nothing to clean up, because we're sm. */
}

namespace {

    /* sm:m, sm:, sm:dmnt. */
    constexpr size_t NumServers  = 3;
    sf::hipc::ServerManager<NumServers> g_server_manager;

}

int main(int argc, char **argv)
{
    /* Set thread name. */
    os::SetThreadNamePointer(os::GetCurrentThread(), AMS_GET_SYSTEM_THREAD_NAME(sm, Main));
    AMS_ASSERT(os::GetThreadPriority(os::GetCurrentThread()) == AMS_GET_SYSTEM_THREAD_PRIORITY(sm, Main));

    /* NOTE: These handles are manually managed, but we don't save references to them to close on exit. */
    /* This is fine, because if SM crashes we have much bigger issues. */

    /* Create sm:, (and thus allow things to register to it). */
    {
        Handle sm_h;
        R_ABORT_UNLESS(svcManageNamedPort(&sm_h, "sm:", 0x40));
        g_server_manager.RegisterServer<sm::impl::IUserInterface, sm::UserService>(sm_h);
    }

    /* Create sm:m manually. */
    {
        Handle smm_h;
        R_ABORT_UNLESS(sm::impl::RegisterServiceForSelf(&smm_h, sm::ServiceName::Encode("sm:m"), 1));
        g_server_manager.RegisterServer<sm::impl::IManagerInterface, sm::ManagerService>(smm_h);
    }

    /*===== ATMOSPHERE EXTENSION =====*/
    /* Create sm:dmnt manually. */
    {
        Handle smdmnt_h;
        R_ABORT_UNLESS(sm::impl::RegisterServiceForSelf(&smdmnt_h, sm::ServiceName::Encode("sm:dmnt"), 1));
        g_server_manager.RegisterServer<sm::impl::IDebugMonitorInterface, sm::DebugMonitorService>(smdmnt_h);
    }

    /*================================*/

    /* Loop forever, servicing our services. */
    g_server_manager.LoopProcess();

    /* Cleanup. */
    return 0;
}
