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
#include "synchronization.h"
#include "sysreg.h"

// For a32 mcr/mrc => a64 mrs
u32 convertMcrMrcIss(u32 *outCondition, u32 a32Iss, u32 coproc, u32 el)
{
    // NOTE: MCRR / MRRC do NOT map for the most part and need to be handled separately

    //u32 direction = a32Iss & 1;

    //u32 opc2 = (a32Iss >> 17) & 7;
    u32 opc1 = (a32Iss >> 14) & 7;
    u32 CRn  = (a32Iss >> 10) & 15;
    //u32 Rt   = (a32Iss >>  5) & 31;
    //u32 CRm  = (a32Iss >>  1) & 15;

    bool condValid = (a32Iss & BIT(24)) != 0;
    *outCondition = condValid ? ((a32Iss >> 20) & 15): 14; // use "unconditional" by default

    u32 op0 = (16 - coproc) & 3;
    u32 op1;

    // Do NOT translate cp15, 0, c7-8 (Cache, and resp. TLB maintenance coproc regs) as they don't map to Aarch64
    if (coproc == 15 && (CRn == 7 || CRn == 8)) {
        return -2;
    }

    // The difficult part
    switch (opc1) {
        case SYSREG_OP1_AARCH32_AUTO: {
            switch (el) {
                case 0:
                    op1 = SYSREG_OP1_EL0;
                    break;
                case 1:
                    op1 = SYSREG_OP1_AARCH64_EL1;
                    break;
                case 2:
                    op1 = SYSREG_OP1_EL2;
                    break;
                case 3:
                    op1 = SYSREG_OP1_EL3;
                    break;
                default:
                    return -1;
            }
            break;
        }

        case SYSREG_OP1_EL0:
        case SYSREG_OP1_EL2:
        case SYSREG_OP1_EL3:
        case SYSREG_OP1_CACHE:
        case SYSREG_OP1_CACHESZSEL:
            op1 = opc1;
            break;

        // We shouldn't even trap those to begin with.
        case SYSREG_OP1_AARCH32_JZL:
        default:
            return -1;
    }

    // Everything but op0 is at its correct place & only op1 needs to be replaced
    return (a32Iss & ~(MASK2(24, 20) | MASK2(16, 14))) | (op0 << 20) | (op1 << 14);
}

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

    __dsb_sy();
    __isb();

    *val = ((u64 (*)(u64))codebuf)(*val);
}

void doSystemRegisterRead(ExceptionStackFrame *frame, u32 iss, u32 reg1, u32 reg2)
{
    // reg1 != reg2: mrrc/mcrr
    u64 val = 0;
    doSystemRegisterRwImpl(&val, iss | 1);
    if (reg1 == reg2) {
        frame->x[reg1] = val;
    } else {
        frame->x[reg1] = val & 0xFFFFFFFF;
        frame->x[reg2] = val >> 32;
    }

    // Sysreg access are always 4 bit in length even for Aarch32
    frame->elr_el2 += 4;
}

void doSystemRegisterWrite(ExceptionStackFrame *frame, u32 iss, u32 reg1, u32 reg2)
{
    // reg1 != reg2: mrrc/mcrr
    u64 val = frame->x[reg1];

    if (reg1 != reg2) {
        val  = (val << 32) >> 32;
        val |= frame->x[reg2] << 32;
    }

    doSystemRegisterRwImpl(&val, iss & ~1);

    // Sysreg access are always 4 bit in length even for Aarch32
    frame->elr_el2 += 4;
}
