/*
 * Copyright (c) 2019 Atmosphère-NX
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

#include "sysreg_traps.h"
#include "sysreg.h"
#include "arm.h"
#include "debug_log.h"
#include "software_breakpoints.h"

static void doSystemRegisterRwImpl(u64 *val, u32 iss)
{
    u32 op0  = (iss >> 20) & 3;
    u32 op2  = (iss >> 17) & 7;
    u32 op1  = (iss >> 14) & 7;
    u32 CRn  = (iss >> 10) & 15;
    //u32 Rt   = (iss >>  5) & 31;
    u32 CRm  = (iss >>  1) & 15;
    u32 dir  = iss & 1;

    u32 codebuf[] = {
        0,              // TBD
        0xD65F03C0,     // ret
    };

    codebuf[0] = dir ? MAKE_MRS_FROM_FIELDS(op0, op1, CRn, CRm, op2, 0) : MAKE_MSR_FROM_FIELDS(op0, op1, CRn, CRm, op2, 0);

    flush_dcache_range(codebuf, (u8 *)codebuf + sizeof(codebuf));
    invalidate_icache_all();

    *val = ((u64 (*)(u64))codebuf)(*val);
}

void doSystemRegisterRead(ExceptionStackFrame *frame, u32 iss, u32 reg)
{
    u64 val = 0;

    iss &= ~((0x1F << 5) | 1);

    // Hooks go here:
    switch (iss) {
        default:
            break;
    }

    doSystemRegisterRwImpl(&val, iss | 1);
    frame->x[reg] = val;

    skipFaultingInstruction(frame, 4);
}

void doSystemRegisterWrite(ExceptionStackFrame *frame, u32 iss, u32 reg)
{
    u64 val = 0;
    iss &= ~((0x1F << 5) | 1);

    val = frame->x[reg];

    bool reevalSoftwareBreakpoints = false;

    // Hooks go here:
    switch (iss) {
        case ENCODE_SYSREG_ISS(TTBR0_EL1):
        case ENCODE_SYSREG_ISS(TTBR1_EL1):
        case ENCODE_SYSREG_ISS(TCR_EL1):
        case ENCODE_SYSREG_ISS(SCTLR_EL1):
            reevalSoftwareBreakpoints = true;
            break;
        default:
            break;
    }

    if (reevalSoftwareBreakpoints) {
        revertAllSoftwareBreakpoints();
    }

    doSystemRegisterRwImpl(&val, iss);

    if (reevalSoftwareBreakpoints) {
        __dsb_sy();
        __isb();
        applyAllSoftwareBreakpoints();
    }

    skipFaultingInstruction(frame, 4);
}

void handleMsrMrsTrap(ExceptionStackFrame *frame, ExceptionSyndromeRegister esr)
{
    u32 iss = esr.iss;
    u32 reg = (iss >> 5) & 31;
    bool isRead = (iss & 1) != 0;

    if (isRead) {
        doSystemRegisterRead(frame, iss, reg);
    } else {
        doSystemRegisterWrite(frame, iss, reg);
    }
}

static bool evaluateMcrMrcCondition(u64 spsr, u32 condition, bool condValid)
{
    if (!condValid) {
        // Only T32 instructions can do that
        u32 it = spsrGetT32ItFlags(spsr);
        return it == 0 || spsrEvaluateConditionCode(spsr, it >> 4);
    } else {
        return spsrEvaluateConditionCode(spsr, condition);
    }
}

void handleSysregAccessA32Stub(ExceptionStackFrame *frame, ExceptionSyndromeRegister esr)
{
    // A32 stub: Skip instruction, read 0 if necessary (there are debug regs at EL0)

    if (esr.iss & 1 && evaluateMcrMrcCondition(frame->spsr_el2, (esr.iss >> 20) & 0xF, (esr.iss & BIT(24)) != 0)) {
        frame->x[(esr.iss >> 5) & 0x1F] = 0;
    }
    skipFaultingInstruction(frame, esr.il == 0 ? 2 : 4);
}
