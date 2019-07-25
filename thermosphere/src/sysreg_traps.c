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
static u32 convertMcrMrcIss(u32 *outCondition, bool *outCondValid, u32 a32Iss, u32 coproc, u32 el)
{
    // NOTE: MCRR / MRRC do NOT map for the most part and need to be handled separately

    //u32 direction = a32Iss & 1;

    //u32 opc2 = (a32Iss >> 17) & 7;
    u32 opc1 = (a32Iss >> 14) & 7;
    u32 CRn  = (a32Iss >> 10) & 15;
    //u32 Rt   = (a32Iss >>  5) & 31;
    //u32 CRm  = (a32Iss >>  1) & 15;
    *outCondValid = (a32Iss & BIT(24)) != 0;
    *outCondition = (a32Iss >> 20) & 15;

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

    iss &= ~((0x1F << 5) | 1);

    doSystemRegisterRwImpl(&val, iss);
    if (reg1 == reg2) {
        frame->x[reg1] = val;
    } else {
        frame->x[reg1] = val & 0xFFFFFFFF;
        frame->x[reg2] = val >> 32;
    }

    skipFaultingInstruction(frame, 4);
}

void doSystemRegisterWrite(ExceptionStackFrame *frame, u32 iss, u32 reg1, u32 reg2)
{
    // reg1 != reg2: mrrc/mcrr
    u64 val = frame->x[reg1];
    iss &= ~((0x1F << 5) | 1);

    if (reg1 != reg2) {
        val  = (val << 32) >> 32;
        val |= frame->x[reg2] << 32;
    }

    doSystemRegisterRwImpl(&val, iss);
    skipFaultingInstruction(frame, 4);
}

void handleMsrMrsTrap(ExceptionStackFrame *frame, ExceptionSyndromeRegister esr)
{
    u32 iss = esr.iss;
    u32 reg = (iss >> 5) & 31;
    bool isRead = (iss & 1) != 0;

    if (isRead) {
        doSystemRegisterRead(frame, iss, reg, reg);
    } else {
        doSystemRegisterWrite(frame, iss, reg, reg);
    }
}

void handleMcrMrcTrap(ExceptionStackFrame *frame, ExceptionSyndromeRegister esr)
{
    u32 condition;
    bool condValid;
    u32 coproc = esr.ec == Exception_CP14RTTrap ? 14 : 15;

    // EL0 if User Mode else EL1
    esr.iss = convertMcrMrcIss(&condition, &condValid, esr.iss, coproc, (frame->spsr_el2 & 0xF) == 0 ? 0 : 1);

    if (esr.iss & BIT(31)) {
        // Error, we shouldn't have trapped those in first place anyway.
        skipFaultingInstruction(frame, 4);
        return;
    } else if (!evaluateMcrMrcCondition(frame->spsr_el2, condition, condValid)) {
        skipFaultingInstruction(frame, 4);
        return;
    }

    handleMsrMrsTrap(frame, esr);
}

void handleMcrrMrrcTrap(ExceptionStackFrame *frame, ExceptionSyndromeRegister esr)
{
    u32 iss = esr.iss;
    u32 coproc = esr.ec == Exception_CP14RRTTrap ? 14 : 15;
    bool cv = (iss & BIT(24)) != 0;
    u32 cond = (iss >> 20) & 0xF;
    u32 opc1 = (iss >> 16) & 0xF;
    u32 reg2 = (iss >> 10) & 0x1F;
    u32 reg1 = (iss >>  5) & 0x1F;
    u32 crm  = (iss >>  1) & 0xF;
    u32 dir  = iss & 1;

    if (!evaluateMcrMrcCondition(frame->spsr_el2, cond, cv)) {
        skipFaultingInstruction(frame, 4);
        return;
    }

    u32 sysregIss = -1;
    // No automatic conversion, handle what we potentiall trap
    if (coproc == 14) {
        if (crm == 1 && opc1 == 0) {
            sysregIss = ENCODE_SYSREG_ISS(MDRAR_EL1);
        } else if (crm == 2 && opc1 == 0) {
            // DBGSAR, deprecated in Armv8 and no reg mapping,
            // we won't handle it
        }
    } else {
        switch (crm) {
            case 2: {
                switch (opc1) {
                    case 0:
                        sysregIss = ENCODE_SYSREG_ISS(TTBR0_EL1);
                        break;
                    case 1:
                        sysregIss = ENCODE_SYSREG_ISS(TTBR1_EL1);
                        break;
                    default:
                        // other regs are el2 ttbr regs, not trapped here
                        break;
                }
                break;
            }

            case 7:
                sysregIss = opc1 == 0 ? ENCODE_SYSREG_ISS(PAR_EL1) : -1;
                break;

            case 14: {
                switch (opc1) {
                    case 0:
                        sysregIss = ENCODE_SYSREG_ISS(CNTPCT_EL0);
                        break;
                    case 1:
                        sysregIss = ENCODE_SYSREG_ISS(CNTVCT_EL0);
                        break;
                    case 2:
                        sysregIss = ENCODE_SYSREG_ISS(CNTP_CVAL_EL0);
                        break;
                    case 3:
                        sysregIss = ENCODE_SYSREG_ISS(CNTV_CVAL_EL0);
                        break;
                    case 4:
                        sysregIss = ENCODE_SYSREG_ISS(CNTVOFF_EL2);
                        break;
                    case 6:
                        sysregIss = ENCODE_SYSREG_ISS(CNTHP_CVAL_EL2);
                        break;
                }
            }
        }
    }

    if (esr.iss & BIT(31)) {
        // Error, we shouldn't have trapped those in first place anyway.
        skipFaultingInstruction(frame, 4);
        return;
    }

    if (dir == 1) {
        doSystemRegisterRead(frame, sysregIss, reg1, reg2);
    } else {
        doSystemRegisterWrite(frame, sysregIss, reg1, reg2);
    }
}

void handleLdcStcTrap(ExceptionStackFrame *frame, ExceptionSyndromeRegister esr)
{
    // Only used for DBGDTRRXint
    // Do not execute the read/writes
    skipFaultingInstruction(frame, esr.il == 0 ? 2 : 4);
}
