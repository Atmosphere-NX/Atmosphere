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

void thermosphereMain(ExceptionStackFrame *frame)
{
    enableTraps();
    enableBreakpointsAndWatchpoints();
    timerInit();
    initIrq();

    if (currentCoreCtx->isBootCore) {
        uartInit(DEFAULT_UART, BAUD_115200, 0);
        uartSetInterruptStatus(DEFAULT_UART, DIRECTION_READ, true);

        DEBUG("EL2: core %u reached main first!\n", currentCoreCtx->coreId);
    }

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
    frame->cntvct_el0   = GET_SYSREG(cntvct_el0);
}
