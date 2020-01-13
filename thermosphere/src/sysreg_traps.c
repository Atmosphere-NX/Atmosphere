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
#include "guest_timers.h"
#include "software_breakpoints.h"

static inline u64 doSystemRegisterRead(const ExceptionStackFrame *frame, u32 normalizedIss)
{
    // See https://developer.arm.com/architectures/learn-the-architecture/generic-timer/single-page

    u64 val;
    switch (normalizedIss) {
        case ENCODE_SYSREG_ISS(CNTPCT_EL0): {
            u64 vct = computeCntvct(frame);
            val = vct;
            break;
        }
        case ENCODE_SYSREG_ISS(CNTP_TVAL_EL0): {
            u64 vct  = frame->cntpct_el0 - currentCoreCtx->totalTimeInHypervisor;
            u64 cval = currentCoreCtx->emulPtimerCval;
            val = (cval - vct) & 0xFFFFFFFF;
            break;
        }
        case ENCODE_SYSREG_ISS(CNTP_CTL_EL0): {
            val = frame->cntp_ctl_el0;
            break;
        }
        case ENCODE_SYSREG_ISS(CNTP_CVAL_EL0): {
            val = currentCoreCtx->emulPtimerCval;
            break;
        }

        default: {
            // We shouldn't have trapped on other registers other than debug regs
            // and we want the latter as RA0/WI
            val = 0;
            break;
        }
    }

    return val;
}

static inline void doSystemRegisterWrite(ExceptionStackFrame *frame, u32 normalizedIss, u64 val)
{
    // See https://developer.arm.com/architectures/learn-the-architecture/generic-timer/single-page

    switch (normalizedIss) {
        case ENCODE_SYSREG_ISS(CNTP_TVAL_EL0): {
            // Sign-extend
            u64 vct = computeCntvct(frame);
            writeEmulatedPhysicalCompareValue(frame, vct + (u64)(s32)val);
            break;
        }
        case ENCODE_SYSREG_ISS(CNTP_CTL_EL0): {
            frame->cntp_ctl_el0 = val;
            break;
        }
        case ENCODE_SYSREG_ISS(CNTP_CVAL_EL0): {
            writeEmulatedPhysicalCompareValue(frame, val);
            break;
        }

        default: {
            // We shouldn't have trapped on other registers other than debug regs
            // and we want the latter as RA0/WI
            break;
        }
    }
}

static inline void doMrs(ExceptionStackFrame *frame, u32 normalizedIss, u32 reg)
{
    writeFrameRegisterZ(frame, reg, doSystemRegisterRead(frame, normalizedIss));
    skipFaultingInstruction(frame, 4);
}

static inline void doMsr(ExceptionStackFrame *frame, u32 normalizedIss, u32 reg)
{
    u64 val = readFrameRegisterZ(frame, reg);
    doSystemRegisterWrite(frame, normalizedIss, val);
    skipFaultingInstruction(frame, 4);
}

static inline void doMrc(ExceptionStackFrame *frame, u32 normalizedIss, u32 instructionLength, u32 reg)
{
    writeFrameRegisterZ(frame, reg, doSystemRegisterRead(frame, normalizedIss) & 0xFFFFFFFF);
    skipFaultingInstruction(frame, instructionLength);
}

static inline void doMcr(ExceptionStackFrame *frame, u32 normalizedIss, u32 instructionLength, u32 reg)
{
    u64 val = readFrameRegisterZ(frame, reg) & 0xFFFFFFFF;
    doSystemRegisterWrite(frame, normalizedIss, val);
    skipFaultingInstruction(frame, instructionLength);
}

static inline void doMrrc(ExceptionStackFrame *frame, u32 normalizedIss, u32 instructionLength, u32 reg, u32 reg2)
{
    u64 val = doSystemRegisterRead(frame, normalizedIss);
    writeFrameRegister(frame, reg, val & 0xFFFFFFFF);
    writeFrameRegister(frame, reg2, val >> 32);
    skipFaultingInstruction(frame, instructionLength);
}

static inline void doMcrr(ExceptionStackFrame *frame, u32 normalizedIss, u32 instructionLength, u32 reg, u32 reg2)
{
    u64 valLo = readFrameRegister(frame, reg)  & 0xFFFFFFFF;
    u64 valHi = readFrameRegister(frame, reg2) << 32;
    doSystemRegisterWrite(frame, normalizedIss, valHi | valLo);
    skipFaultingInstruction(frame, instructionLength);
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

void handleMsrMrsTrap(ExceptionStackFrame *frame, ExceptionSyndromeRegister esr)
{
    u32 iss = esr.iss;
    u32 reg = (iss >> 5) & 31;
    bool isRead = (iss & 1) != 0;

    iss &= ~((0x1F << 5) | 1);

    if (isRead) {
        doMrs(frame, iss, reg);
    } else {
        doMsr(frame, iss, reg);
    }
}

void handleMcrMrcCP15Trap(ExceptionStackFrame *frame, ExceptionSyndromeRegister esr)
{
    u32 iss = esr.iss;

    if (!evaluateMcrMrcCondition(frame->spsr_el2, (iss >> 20) & 0xF, (iss & BIT(24)) != 0)) {
        // If instruction not valid/condition code says no
        skipFaultingInstruction(frame, esr.il == 0 ? 2 : 4);
        return;
    }

    u32 opc2 = (iss >> 17) & 7;
    u32 opc1 = (iss >> 14) & 7;
    u32 CRn  = (iss >> 10) & 15;
    u32 Rt   = (iss >>  5) & 31;
    u32 CRm  = (iss >>  1) & 15;
    bool isRead = (iss & 1) != 0;
    u32 instructionLength = esr.il == 0 ? 2 : 4;

    if (LIKELY(opc1 == 0 && CRn == 14 && CRm == 2 && opc2 <= 1)) {
        iss = opc2 == 0 ? ENCODE_SYSREG_ISS(CNTP_TVAL_EL0) : ENCODE_SYSREG_ISS(CNTP_CTL_EL0);
    } else {
        PANIC("handleMcrMrcTrap: unexpected cp15 register, instruction: %s p15, #%u, r%u, c%u, c%u, #%u\n", isRead ? "mrc" : "mcr", opc1, Rt, CRn, CRm, opc2);
    }

    if (isRead) {
        doMrc(frame, iss, instructionLength, Rt);
    } else {
        doMcr(frame, iss, instructionLength, Rt);
    }
}

void handleMcrrMrrcCP15Trap(ExceptionStackFrame *frame, ExceptionSyndromeRegister esr)
{
    u32 iss = esr.iss;

    if (!evaluateMcrMrcCondition(frame->spsr_el2, (iss >> 20) & 0xF, (iss & BIT(24)) != 0)) {
        // If instruction not valid/condition code says no
        skipFaultingInstruction(frame, esr.il == 0 ? 2 : 4);
        return;
    }

    u32 opc1 = (iss >> 16) & 15;
    u32 Rt2  = (iss >> 10) & 31;
    u32 Rt   = (iss >>  5) & 31;
    u32 CRm  = (iss >>  1) & 15;

    bool isRead = (iss & 1) != 0;
    u32 instructionLength = esr.il == 0 ? 2 : 4;

    if (LIKELY(CRm == 14 && (opc1 == 0 || opc1 == 2))) {
        iss = opc1 == 0 ? ENCODE_SYSREG_ISS(CNTPCT_EL0) : ENCODE_SYSREG_ISS(CNTP_CVAL_EL0);
    } else {
        PANIC("handleMcrrMrrcTrap: unexpected cp15 register, instruction: %s p15, #%u, r%u, r%u, c%u\n", isRead ? "mrrc" : "mcrr", opc1, Rt, Rt, CRm);
    }

    if (isRead) {
        doMrrc(frame, iss, instructionLength, Rt, Rt2);
    } else {
        doMcrr(frame, iss, instructionLength, Rt, Rt2);
    }
}

void handleA32CP14Trap(ExceptionStackFrame *frame, ExceptionSyndromeRegister esr)
{
    // LDC/STC: Skip instruction, read 0 if necessary, since only one debug reg can be accessed with it
    // Other CP14 accesses: do the same thing

    if (esr.iss & 1 && evaluateMcrMrcCondition(frame->spsr_el2, (esr.iss >> 20) & 0xF, (esr.iss & BIT(24)) != 0)) {
        writeFrameRegisterZ(frame, (esr.iss >> 5) & 0x1F, 0);
    }
    skipFaultingInstruction(frame, esr.il == 0 ? 2 : 4);
}
