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
#include <cstdio>
#include <cstring>
#include <malloc.h>

#include <switch.h>
#include <stratosphere/firmware_version.hpp>

#include "creport_crash_report.hpp"


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
    alignas(16) u8 __nx_exception_stack[0x1000];
    u64 __nx_exception_stack_size = sizeof(__nx_exception_stack);
    void __libnx_exception_handler(ThreadExceptionDump *ctx);
    u64 __stratosphere_title_id = TitleId_Creport;
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
    Result rc;
    
    SetFirmwareVersionForLibnx();
    
    DoWithSmSession([&]() {
        rc = fsInitialize();
        if (R_FAILED(rc)) {
            fatalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_FS));
        }
    });
    
    rc = fsdevMountSdmc();
    if (R_FAILED(rc)) {
        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_FS));
    }
}

void __appExit(void) {
    /* Cleanup services. */
    fsdevUnmountAll();
    fsExit();
}

static u64 creport_parse_u64(char *s) {
    /* Official creport uses this custom parsing logic... */
    u64 out_val = 0;
    for (unsigned int i = 0; i < 20 && s[i]; i++) {
        if ('0' <= s[i] && s[i] <= '9') {
            out_val *= 10;
            out_val += (s[i] - '0');
        } else {
            break;
        }
    }
    return out_val;
}

static CrashReport g_Creport;

int main(int argc, char **argv) {
    /* Validate arguments. */
    if (argc < 2) {
        return 0;
    }
    for (int i = 0; i < argc; i++) {
        if (argv[i] == NULL) {
            return 0;
        }
    }
    
    /* Parse crashed PID. */
    u64 crashed_pid = creport_parse_u64(argv[0]);
    
    /* Try to debug the crashed process. */
    g_Creport.BuildReport(crashed_pid, argv[1][0] == '1');
    if (g_Creport.WasSuccessful()) {
        g_Creport.SaveReport();
        
        DoWithSmSession([&]() {
            if (R_SUCCEEDED(nsdevInitialize())) {
                nsdevTerminateProcess(crashed_pid);
                nsdevExit();
            }
        });
        
        /* Don't fatal if we have extra info. */
        if (kernelAbove500()) {
            if (g_Creport.IsApplication()) {
                return 0;
            }
        } else if (argv[1][0] == '1') {
            return 0;
        }
        
        /* Also don't fatal if we're a user break. */
        if (g_Creport.IsUserBreak()) {
            return 0;
        }
        
        FatalContext *ctx = g_Creport.GetFatalContext();
        
        fatalWithContext(g_Creport.GetResult(), FatalType_ErrorScreen, ctx);
    }
    
}