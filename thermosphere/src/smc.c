#include <string.h>
#include "smc.h"
#include "synchronization.h"

// Currently in exception_vectors.s:
extern const u32 doSmcIndirectCallImpl[];
extern const u32 doSmcIndirectCallImplSmcInstructionOffset, doSmcIndirectCallImplSize;

void doSmcIndirectCall(ExceptionStackFrame *frame, u32 smcId)
{
    u32 codebuf[doSmcIndirectCallImplSize]; // note: potential VLA
    memcpy(codebuf, doSmcIndirectCallImpl, doSmcIndirectCallImplSize);
    codebuf[doSmcIndirectCallImplSmcInstructionOffset/4] |= smcId << 5;

    __dsb_sy();
    __isb();

    ((void (*)(ExceptionStackFrame *))codebuf)(frame);
}

void handleSmcTrap(ExceptionStackFrame *frame, ExceptionSyndromeRegister esr)
{
    // TODO: Arm PSCI 0.2+ stuff
    u32 smcId = esr.iss;

    doSmcIndirectCall(frame, smcId);
    skipFaultingInstruction(frame, 4);
}
