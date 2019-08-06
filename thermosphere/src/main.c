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

extern const u8 __start__[];

static void loadKernelViaSemihosting(void)
{
    size_t len = 1<<20; // max len
    uintptr_t buf = (uintptr_t)__start__ + (1<<20);
    long handle = -1, ret;

    DEBUG("Loading kernel via semihosted file I/O... ");
    handle = semihosting_file_open("test_kernel.bin", FOPEN_MODE_RB);
    if (handle < 0) {
        DEBUG("failed to open file (%ld)!\n", handle);
        panic();
    }

    if ((ret = semihosting_file_read(handle, &len, buf)) < 0) {
        DEBUG("failed to read file (%ld)!\n", ret);
        panic();
    }

    DEBUG("OK!\n");
    semihosting_file_close(handle);
    currentCoreCtx->kernelEntrypoint = buf;
}

void main(ExceptionStackFrame *frame)
{
    enableTraps();
    enableBreakpointsAndWatchpoints();

    initBreakpoints();
    initWatchpoints();

    if (currentCoreCtx->isBootCore) {
        uartInit(115200);
        DEBUG("EL2: core %u reached main first!\n", currentCoreCtx->coreId);
        if (currentCoreCtx->kernelEntrypoint == 0) {
            if (semihosting_connection_supported()) {
                loadKernelViaSemihosting();
                //panic();
            } else {
                DEBUG("Kernel not loaded!\n");
                panic();
            }
        }
    }
    else {
        DEBUG("EL2: core %u reached main!\n", currentCoreCtx->coreId);
        //DEBUG("Test 0x%08llx %016llx\n", get_physical_address_el1_stage12(0x08010000ull), GET_SYSREG(par_el1));
    }

    // Set up exception frame: init regs to 0, set spsr, elr, etc.
    memset(frame, 0, sizeof(ExceptionStackFrame));
    frame->spsr_el2 = (0xF << 6) | (1 << 2) | 1; // EL1h+DAIF
    frame->elr_el2  = currentCoreCtx->kernelEntrypoint;
    frame->x[0]     = currentCoreCtx->kernelArgument;

    // Test
    singleStepSetNextState(frame, SingleStepState_ActivePending);
}
