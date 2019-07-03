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

#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <malloc.h>

#include <switch.h>
#include <stratosphere.hpp>

#include "sm_user_service.hpp"
#include "sm_manager_service.hpp"
#include "sm_dmnt_service.hpp"

#include "impl/sm_service_manager.hpp"

extern "C" {
    extern u32 __start__;

    u32 __nx_applet_type = AppletType_None;

    #define INNER_HEAP_SIZE 0x20000
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

sts::ncm::TitleId __stratosphere_title_id = sts::ncm::TitleId::Sm;

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

    /* We must do no service setup here, because we are sm. */
}

void __appExit(void) {
    /* Nothing to clean up, because we're sm. */
}

using namespace sts;

int main(int argc, char **argv)
{
    /* Create service waitable manager. */
    static auto s_server_manager = WaitableManager(1);

    /* Create sm:, (and thus allow things to register to it). */
    s_server_manager.AddWaitable(new ManagedPortServer<sm::UserService>("sm:", 0x40));

    /* Create sm:m manually. */
    Handle smm_h;
    R_ASSERT(sm::impl::RegisterServiceForSelf(&smm_h, sm::ServiceName::Encode("sm:m"), 1));
    s_server_manager.AddWaitable(new ExistingPortServer<sm::ManagerService>(smm_h, 1));

    /*===== ATMOSPHERE EXTENSION =====*/
    /* Create sm:dmnt manually. */
    Handle smdmnt_h;
    R_ASSERT(sm::impl::RegisterServiceForSelf(&smdmnt_h, sm::ServiceName::Encode("sm:dmnt"), 1));
    s_server_manager.AddWaitable(new ExistingPortServer<sm::DmntService>(smm_h, 1));;

    /*================================*/

    /* Loop forever, servicing our services. */
    s_server_manager.Process();

    /* Cleanup. */
    return 0;
}
