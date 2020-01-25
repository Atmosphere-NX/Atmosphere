#include <string.h>

#include "utils.h"
#include "core_ctx.h"
#include "debug_log.h"
#include "platform/uart.h"
#include "semihosting.h"
#include "traps.h"
#include "sysreg.h"
#include "exceptions.h"
#include "single_step.h"
#include "breakpoints.h"
#include "watchpoints.h"
#include "timer.h"
#include "irq.h"
#include "transport_interface.h"
#include "guest_memory.h"

#include "memory_map.h"
#include "mmu.h"

static void loadKernelViaSemihosting(void)
{
    // Note: !! hardcoded addresses !!
    size_t len = 1<<20; // max len
    uintptr_t buf = 0x60000000 + (1<<20);

    long handle = -1, ret;

    u64 *mmuTable = (u64 *)MEMORY_MAP_VA_TTBL;
    mmu_map_block_range(
        1, mmuTable, 0x40000000, 0x40000000, 0x40000000,
        MMU_PTE_BLOCK_XN | MMU_PTE_BLOCK_INNER_SHAREBLE | MMU_PTE_BLOCK_MEMTYPE(MEMORY_MAP_MEMTYPE_NORMAL_UNCACHEABLE)
    );

    __tlb_invalidate_el2_local();
    __dsb_local();

    DEBUG("Loading kernel via semihosted file I/O... ");
    handle = semihosting_file_open("test_kernel.bin", FOPEN_MODE_RB);
    ENSURE2(handle >= 0, "failed to open file (%ld)!\n", handle);

    ret = semihosting_file_read(handle, &len, buf);
    ENSURE2(ret >= 0, "failed to read file (%ld)!\n", ret);

    DEBUG("OK!\n");
    semihosting_file_close(handle);

    mmu_unmap_range(1, mmuTable, 0x40000000, 0x40000000);
    __tlb_invalidate_el2_local();
    __dsb_local();

    currentCoreCtx->kernelEntrypoint = buf;
}



#include "platform/uart.h"
#include "debug_pause.h"
typedef struct TestCtx {
    char buf[512+1];
} TestCtx;

static TestCtx g_testCtx;

size_t testReceiveCallback(TransportInterface *iface, void *p)
{
    TestCtx *ctx = (TestCtx *)p;
    debugPauseCores(BIT(0));
    return transportInterfaceReadDataUntil(iface, ctx->buf, 512, '\r');
}

void testProcessDataCallback(TransportInterface *iface, void *p, size_t sz)
{
    (void)iface;
    (void)sz;
    debugUnpauseCores(BIT(0), BIT(0));
    TestCtx *ctx = (TestCtx *)p;
    DEBUG("EL2 [core %u]: you typed: %s\n", currentCoreCtx->coreId, ctx->buf);
}

void test(void)
{
    TransportInterface *iface = transportInterfaceCreate(
        TRANSPORT_INTERFACE_TYPE_UART,
        DEFAULT_UART,
        DEFAULT_UART_FLAGS,
        testReceiveCallback,
        testProcessDataCallback,
        &g_testCtx
    );
    transportInterfaceSetInterruptAffinity(iface, BIT(1));
}

void thermosphereMain(ExceptionStackFrame *frame, u64 pct)
{
    initIrq();

    if (currentCoreCtx->isBootCore) {
        transportInterfaceInitLayer();
        debugLogInit();
        test();

        DEBUG("EL2: core %u reached main first!\n", currentCoreCtx->coreId);
    }

    enableTraps();
    enableBreakpointsAndWatchpoints();
    timerInit();
    initBreakpoints();
    initWatchpoints();

    if (currentCoreCtx->isBootCore) {
        if (currentCoreCtx->kernelEntrypoint == 0) {
            ENSURE2(semihosting_connection_supported(), "Kernel not loaded!\n");
            loadKernelViaSemihosting();
        }
    }
    else {
        DEBUG("EL2: core %u reached main!\n", currentCoreCtx->coreId);
    }

    setCurrentCoreActive();

    // Set up exception frame: init regs to 0, set spsr, elr, etc.
    memset(frame, 0, sizeof(ExceptionStackFrame));
    frame->spsr_el2     = (0xF << 6) | (1 << 2) | 1; // EL1h+DAIF
    frame->elr_el2      = currentCoreCtx->kernelEntrypoint;
    frame->x[0]         = currentCoreCtx->kernelArgument;
    frame->cntpct_el0   = pct;
}
