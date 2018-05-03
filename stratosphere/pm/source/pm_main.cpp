#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <malloc.h>

#include <switch.h>
#include <stratosphere.hpp>

#include "pm_boot_mode.hpp"
#include "pm_process_track.hpp"

extern "C" {
    extern u32 __start__;

    u32 __nx_applet_type = AppletType_None;

    #define INNER_HEAP_SIZE 0x200000
    size_t nx_inner_heap_size = INNER_HEAP_SIZE;
    char   nx_inner_heap[INNER_HEAP_SIZE];
    
    void __libnx_initheap(void);
    void __appInit(void);
    void __appExit(void);
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

    rc = smInitialize();
    if (R_FAILED(rc)) {
        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_SM));
    }
    
    rc = fsInitialize();
    if (R_FAILED(rc)) {
        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_FS));
    }
        
    rc = lrInitialize();
    if (R_FAILED(rc))  {
        fatalSimple(0xCAFE << 4 | 1);
    }
    
    rc = fsprInitialize();
    if (R_FAILED(rc))  {
        fatalSimple(0xCAFE << 4 | 2);
    }
    
    rc = splInitialize();
    if (R_FAILED(rc))  {
        fatalSimple(0xCAFE << 4 | 3);
    }
    
    rc = ldrPmInitialize();
    if (R_FAILED(rc))  {
        fatalSimple(0xCAFE << 4 | 4);
    }
}

void __appExit(void) {
    /* Cleanup services. */
    fsdevUnmountAll();
    ldrPmExit();
    splExit();
    fsprExit();
    lrExit();
    fsExit();
    smExit();
}

int main(int argc, char **argv)
{
    Thread process_track_thread = {0};
    consoleDebugInit(debugDevice_SVC);
    
    /* Initialize and spawn the Process Tracking thread. */
    ProcessTracking::Initialize();
    if (R_FAILED(threadCreate(&process_track_thread, &ProcessTracking::MainLoop, NULL, 0x4000, 0x15, 0))) {
        /* TODO: Panic. */
    }
    if (R_FAILED(threadStart(&process_track_thread))) {
        /* TODO: Panic. */
    }
    
    
    /* TODO: What's a good timeout value to use here? */
    WaitableManager *server_manager = new WaitableManager(U64_MAX);
        
    /* TODO: Create services. */
    server_manager->add_waitable(new ServiceServer<BootModeService>("pm:bm", 4));
    
    /* Loop forever, servicing our services. */
    server_manager->process();
    
    /* Cleanup. */
    delete server_manager;
	return 0;
}

