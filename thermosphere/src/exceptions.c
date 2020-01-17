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

#include "exceptions.h"

#include "hvc.h"
#include "traps.h"
#include "sysreg_traps.h"
#include "smc.h"
#include "core_ctx.h"
#include "single_step.h"
#include "data_abort.h"
#include "spinlock.h"
#include "debug_pause.h"
#include "timer.h"

bool spsrEvaluateConditionCode(u64 spsr, u32 conditionCode)
{
    if (conditionCode == 14) {
        // AL
        return true;
    } else if (conditionCode == 15) {
        // Invalid encoding
        return false;
    }

    // NZCV
    bool n = (spsr & BIT(31)) != 0;
    bool z = (spsr & BIT(30)) != 0;
    bool c = (spsr & BIT(29)) != 0;
    bool v = (spsr & BIT(28)) != 0;

    bool tableHalf[] = {
        // EQ, CS, MI, VS, HI, GE, GT
        z, c, n, v, c && !z, n == v, !z && n == v,
    };

    return (conditionCode & 1) == 0 ? tableHalf[conditionCode / 2] : !tableHalf[conditionCode / 2];
}

void dumpStackFrame(const ExceptionStackFrame *frame, bool sameEl)
{
#ifndef NDEBUG
    for (u32 i = 0; i < 30; i += 2) {
        DEBUG("x%u\t\t%016llx\t\tx%u\t\t%016llx\n", i, frame->x[i], i + 1, frame->x[i + 1]);
    }

    DEBUG("x30\t\t%016llx\n\n", frame->x[30]);
    DEBUG("elr_el2\t\t%016llx\n", frame->elr_el2);
    DEBUG("spsr_el2\t%016llx\n", frame->spsr_el2);
    DEBUG("far_el2\t\t%016llx\n", frame->far_el2);
    if (sameEl) {
        DEBUG("sp_el2\t\t%016llx\n", frame->sp_el2);
    } else {
        DEBUG("sp_el0\t\t%016llx\n", frame->sp_el0);
    }
    DEBUG("sp_el1\t\t%016llx\n", frame->sp_el1);
    DEBUG("cntpct_el0\t%016llx\n", frame->cntpct_el0);
    DEBUG("cntp_ctl_el0\t%016llx\n", frame->cntp_ctl_el0);
    DEBUG("cntv_ctl_el0\t%016llx\n", frame->cntv_ctl_el0);
#else
    (void)frame;
    (void)sameEl;
#endif
}

static void advanceItState(ExceptionStackFrame *frame)
{
    // Just in case EL0 is executing A32 (& not sure if fully supported)
    if (!spsrIsThumb(frame->spsr_el2) || spsrGetT32ItFlags(frame->spsr_el2) == 0) {
        return;
    }

    u32 it = spsrGetT32ItFlags(frame->spsr_el2);

    // Last instruction of the block => wipe, otherwise advance
    spsrSetT32ItFlags(&frame->spsr_el2, (it & 7) == 0 ? 0 : (it & 0xE0) | ((it << 1) & 0x1F));
}

void skipFaultingInstruction(ExceptionStackFrame *frame, u32 size)
{
    advanceItState(frame);
    frame->elr_el2 += size;
}

void exceptionEnterInterruptibleHypervisorCode(void)
{
    // We don't want the guest to spam us with its timer interrupts. Disable the timers.
    SET_SYSREG(cntp_ctl_el0, 0);
    SET_SYSREG(cntv_ctl_el0, 0);
}

// Called on exception entry (avoids overflowing a vector section)
void exceptionEntryPostprocess(ExceptionStackFrame *frame, bool isLowerEl)
{
    if (frame == currentCoreCtx->guestFrame) {
        frame->cntp_ctl_el0 = GET_SYSREG(cntp_ctl_el0);
        frame->cntv_ctl_el0 = GET_SYSREG(cntv_ctl_el0);
    }
}

// Called on exception return (avoids overflowing a vector section)
void exceptionReturnPreprocess(ExceptionStackFrame *frame)
{
    if (currentCoreCtx->wasPaused && frame == currentCoreCtx->guestFrame) {
        // Were we paused & are we about to return to the guest?
        exceptionEnterInterruptibleHypervisorCode();
        debugPauseWaitAndUpdateSingleStep();
    }

    // Update virtual counter
    currentCoreCtx->totalTimeInHypervisor += timerGetSystemTick() - frame->cntpct_el0;
    SET_SYSREG(cntvoff_el2, currentCoreCtx->totalTimeInHypervisor);

    if (frame == currentCoreCtx->guestFrame) {
        // Restore interrupt mask
        SET_SYSREG(cntp_ctl_el0, frame->cntp_ctl_el0);
        SET_SYSREG(cntv_ctl_el0, frame->cntv_ctl_el0);
    }
}

void handleLowerElSyncException(ExceptionStackFrame *frame, ExceptionSyndromeRegister esr)
{
    switch (esr.ec) {
        case Exception_CP15RTTrap:
            handleMcrMrcCP15Trap(frame, esr);
            break;
        case Exception_CP15RRTTrap:
            handleMcrrMrrcCP15Trap(frame, esr);
            break;
        case Exception_CP14RTTrap:
        case Exception_CP14DTTrap:
        case Exception_CP14RRTTrap:
            // A32 stub: Skip instruction, read 0 if necessary (there are debug regs at EL0)
            handleA32CP14Trap(frame, esr);
            break;
        case Exception_HypervisorCallA64:
            handleHypercall(frame, esr);
            break;
        case Exception_MonitorCallA64:
            handleSmcTrap(frame, esr);
            break;
        case Exception_SystemRegisterTrap:
            handleMsrMrsTrap(frame, esr);
            break;
        case Exception_DataAbortLowerEl:
            // Basically, stage2 translation faults
            handleLowerElDataAbortException(frame, esr);
            break;
        case Exception_SoftwareStepLowerEl:
            handleSingleStep(frame, esr);
            break;

        default:
            DEBUG("Lower EL sync exception, EC = 0x%02llx IL=%llu ISS=0x%06llx\n", (u64)esr.ec, esr.il, esr.iss);
            dumpStackFrame(frame, false);
            break;
    }
}

void handleSameElSyncException(ExceptionStackFrame *frame, ExceptionSyndromeRegister esr)
{
    DEBUG("Same EL sync exception on core %x, EC = 0x%02llx IL=%llu ISS=0x%06llx\n", currentCoreCtx->coreId, (u64)esr.ec, esr.il, esr.iss);
    dumpStackFrame(frame, true);
}

void handleUnknownException(u32 offset)
{
    DEBUG("Unknown exception on core %x! (offset 0x%03lx)\n", offset, currentCoreCtx->coreId);
}
