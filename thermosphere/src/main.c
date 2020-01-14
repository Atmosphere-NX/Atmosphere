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

extern const u8 __start__[];

static void loadKernelViaSemihosting(void)
{
    size_t len = 1<<20; // max len
    uintptr_t buf = (uintptr_t)__start__ + (1<<20);
    long handle = -1, ret;

    DEBUG("Loading kernel via semihosted file I/O... ");
    handle = semihosting_file_open("test_kernel.bin", FOPEN_MODE_RB);
    if (handle < 0) {
        PANIC("failed to open file (%ld)!\n", handle);
    }

    if ((ret = semihosting_file_read(handle, &len, buf)) < 0) {
        PANIC("failed to read file (%ld)!\n", ret);
    }

    DEBUG("OK!\n");
    semihosting_file_close(handle);
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
    debugUnpauseCores(BIT(0));
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
            if (semihosting_connection_supported()) {
                loadKernelViaSemihosting();
            } else {
                PANIC("Kernel not loaded!\n");
            }
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
