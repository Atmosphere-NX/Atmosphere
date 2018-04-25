#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <malloc.h>

#include <switch.h>
#include <stratosphere.hpp>

#define PMC_BASE 0x7000E400

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

    /* Initialize services we need (TODO: SPL, NCM) */
    rc = smInitialize();
    if (R_FAILED(rc))
        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_SM));
    
    rc = fsInitialize();
    if (R_FAILED(rc))
        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_FS));
    
    rc = pmshellInitialize();
    if (R_FAILED(rc))
        fatalSimple(0xCAFE << 4 | 1);
    
    fsdevInit();
}

void __appExit(void) {
    /* Cleanup services. */
    fsdevExit();
    pmshellExit(); 
    fsExit();
    smExit();
}

int main(int argc, char **argv)
{
    consoleDebugInit(debugDevice_SVC);
    
    Result rc;
    
    /* Change GPIO voltage to 1.8v */
    if (kernelAbove200()) {
        /* TODO: svcReadWriteRegister */
    } else {
        u64* pmc_base_vaddr = NULL;
        
        /* Map the PMC registers directly */
        rc = svcQueryIoMapping(pmc_base_vaddr, PMC_BASE, 0x3000);
        if (R_FAILED(rc)) {
            return rc;
        }
        
        /* IO mapping failed */
        if (!pmc_base_vaddr)
            fatalSimple(MAKERESULT(Module_Libnx, LibnxError_IoError));
        
        /* Update PMC_PWR_DET_ENABLE and PMC_PWR_DET_VAL */
        *((u32 *)pmc_base_vaddr + 0x48) |= 0x00A42000;
        *((u32 *)pmc_base_vaddr + 0xE4) &= 0xFF5BDFFF;
    }
    
    /* Wait for changes to take effect */
    svcSleepThread(100000);
    
    /* TODO: Hardware setup, NAND repair, NotifyBootFinished */
    
    rc = 0;
	return rc;
}