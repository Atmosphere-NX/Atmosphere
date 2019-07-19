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
#include <atmosphere.h>
#include <stratosphere.hpp>

#include "fatal_service.hpp"
#include "fatal_config.hpp"
#include "fatal_repair.hpp"
#include "fatal_font.hpp"

extern "C" {
    extern u32 __start__;

    u32 __nx_applet_type = AppletType_None;

    #define INNER_HEAP_SIZE 0x2A0000
    size_t nx_inner_heap_size = INNER_HEAP_SIZE;
    char   nx_inner_heap[INNER_HEAP_SIZE];

    u32 __nx_nv_transfermem_size = 0x40000;
    ViLayerFlags __nx_vi_stray_layer_flags = (ViLayerFlags)0;

    void __libnx_initheap(void);
    void __appInit(void);
    void __appExit(void);

    /* Exception handling. */
    alignas(16) u8 __nx_exception_stack[0x1000];
    u64 __nx_exception_stack_size = sizeof(__nx_exception_stack);
    void __libnx_exception_handler(ThreadExceptionDump *ctx);
    void __libstratosphere_exception_handler(AtmosphereFatalErrorContext *ctx);
}

sts::ncm::TitleId __stratosphere_title_id = sts::ncm::TitleId::Fatal;

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
        R_ASSERT(setInitialize());
        R_ASSERT(setsysInitialize());
        R_ASSERT(pminfoInitialize());
        R_ASSERT(i2cInitialize());
        R_ASSERT(bpcInitialize());

        if (GetRuntimeFirmwareVersion() >= FirmwareVersion_800) {
            R_ASSERT(clkrstInitialize());
        } else {
            R_ASSERT(pcvInitialize());
        }

        R_ASSERT(lblInitialize());
        R_ASSERT(psmInitialize());
        R_ASSERT(spsmInitialize());
        R_ASSERT(plInitialize());
        R_ASSERT(gpioInitialize());
        R_ASSERT(fsInitialize());
    });

    R_ASSERT(fsdevMountSdmc());

    /* fatal cannot throw fatal, so don't do: CheckAtmosphereVersion(CURRENT_ATMOSPHERE_VERSION); */
}

void __appExit(void) {
    /* Cleanup services. */
    fsdevUnmountAll();
    fsExit();
    plExit();
    gpioExit();
    spsmExit();
    psmExit();
    lblExit();
    if (GetRuntimeFirmwareVersion() >= FirmwareVersion_800) {
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

int main(int argc, char **argv)
{
    /* Load shared font. */
    R_ASSERT(sts::fatal::srv::font::InitializeSharedFont());

    /* Check whether we should throw fatal due to repair process. */
    sts::fatal::srv::CheckRepairStatus();

    /* Create waitable manager. */
    static auto s_server_manager = WaitableManager(1);

    /* Create services. */
    s_server_manager.AddWaitable(new ServiceServer<sts::fatal::srv::PrivateService>("fatal:p", 4));
    s_server_manager.AddWaitable(new ServiceServer<sts::fatal::srv::UserService>("fatal:u", 4));
    s_server_manager.AddWaitable(sts::fatal::srv::GetFatalDirtyEvent());

    /* Loop forever, servicing our services. */
    s_server_manager.Process();

    return 0;
}

