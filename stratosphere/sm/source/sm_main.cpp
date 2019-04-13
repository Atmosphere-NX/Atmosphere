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

#include "sm_manager_service.hpp"
#include "sm_user_service.hpp"
#include "sm_dmnt_service.hpp"
#include "sm_registration.hpp"

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
    u64 __stratosphere_title_id = TitleId_Sm;
    void __libstratosphere_exception_handler(AtmosphereFatalErrorContext *ctx);
}

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




int main(int argc, char **argv)
{
    consoleDebugInit(debugDevice_SVC);
    
    /* TODO: What's a good timeout value to use here? */
    auto server_manager = new WaitableManager(1);

    /* Create sm:, (and thus allow things to register to it). */
    server_manager->AddWaitable(new ManagedPortServer<UserService>("sm:", 0x40));
        
    /* Create sm:m manually. */
    Handle smm_h;
    if (R_FAILED(Registration::RegisterServiceForSelf(smEncodeName("sm:m"), 1, false, &smm_h))) {
        /* TODO: Panic. */
        while (1) { }
    }
    
    server_manager->AddWaitable(new ExistingPortServer<ManagerService>(smm_h, 1));
    
    /*===== ATMOSPHERE EXTENSION =====*/
    /* Create sm:dmnt manually. */
    Handle smdmnt_h;
    if (R_FAILED(Registration::RegisterServiceForSelf(smEncodeName("sm:dmnt"), 1, false, &smdmnt_h))) {
        /* TODO: Panic. */
        while (1) { }
    }
    
    server_manager->AddWaitable(new ExistingPortServer<DmntService>(smm_h, 1));;
    /*================================*/
        
    /* Loop forever, servicing our services. */
    server_manager->Process();
    
    /* Cleanup. */
    delete server_manager;
	return 0;
}

