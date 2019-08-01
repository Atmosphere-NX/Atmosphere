#include <string.h>
#include "smc.h"
#include "synchronization.h"
#include "core_ctx.h"
#include "arm.h"

// Currently in exception_vectors.s:
extern const u32 doSmcIndirectCallImpl[];
extern const u32 doSmcIndirectCallImplSmcInstructionOffset, doSmcIndirectCallImplSize;

// start.s
void start2(u64 contextId);

void doSmcIndirectCall(ExceptionStackFrame *frame, u32 smcId)
{
    u32 codebuf[doSmcIndirectCallImplSize / 4]; // note: potential VLA
    memcpy(codebuf, doSmcIndirectCallImpl, doSmcIndirectCallImplSize);
    codebuf[doSmcIndirectCallImplSmcInstructionOffset / 4] |= smcId << 5;

    flush_dcache_range(codebuf, codebuf + doSmcIndirectCallImplSize/4);
    invalidate_icache_all();

    ((void (*)(ExceptionStackFrame *))codebuf)(frame);
}

static void doCpuOnHook(ExceptionStackFrame *frame, u32 smcId)
{
    // Note: preserve contexId which is passed by EL3, we'll save it later
    u32 cpuId = (u32)frame->x[1]; // note: full affinity mask. Start.s checks that Aff1,2,3 are 0.
    uintptr_t ep = frame->x[2];
    // frame->x[3] is contextId
    if (cpuId < 4) {
        g_coreCtxs[cpuId].kernelEntrypoint = ep;
        frame->x[2] = (uintptr_t)start2;
    }
}

void handleSmcTrap(ExceptionStackFrame *frame, ExceptionSyndromeRegister esr)
{
    // TODO: Arm PSCI 0.2+ stuff
    u32 smcId = esr.iss;

    // Note: don't differenciate PSCI calls by smc immediate since HOS uses #1 for that
    // Note: funcId is actually u32 according to Arm PSCI. Not sure what to do if x0>>32 != 0.
    // NN doesn't truncate, Arm TF does.
    // Note: clear NN ABI-breaking bits
    u32 funcId = frame->x[0] & ~0xFF00;
    switch (funcId) {
        case 0xC4000001:
            // CPU_SUSPEND
            // TODO
            break;
        case 0x84000002:
            // CPU_OFF
            // TODO
            break;
        case 0xC4000003:
            doCpuOnHook(frame, smcId);
            break;
        case 0x84000008:
            // SYSTEM_OFF, not implemented by Nintendo
            // TODO
            break;
        case 0x84000009:
            // SYSTEM_RESET, not implemented by Nintendo
            // TODO
            break;

        default:
            // Other unimportant functions we don't care about
            break;
    }
    doSmcIndirectCall(frame, smcId);
    skipFaultingInstruction(frame, 4);
}
