/*
 * Copyright (c) 2019-2020 Atmosphère-NX
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

#include "hvisor_exception_dispatcher.hpp"
#include "hvisor_irq_manager.hpp"
#include "hvisor_fpu_register_cache.hpp"
#include "hvisor_guest_timers.hpp"

#include "debug_manager.h"

namespace ams::hvisor {

    void EnableGeneralTraps(void)
    {
        u64 hcr = THERMOSPHERE_GET_SYSREG(hcr_el2);

        // Trap SMC instructions
        hcr |= cpu::HCR_TSC;

        // Trap set/way isns
        hcr |= cpu::HCR_TSW;

        // Reroute physical IRQs to EL2
        hcr |= cpu::HCR_IMO;

        // Make sure HVC is enabled
        hcr &= ~cpu::HCR_HCD;

        THERMOSPHERE_SET_SYSREG(hcr_el2, hcr);

        EnableGuestTimerTraps();
    }

void DumpStackFrame(ExceptionStackFrame *frame, bool sameEl)
    {
#ifndef NDEBUG
        uintptr_t stackTop = memoryMapGetStackTop(currentCoreCtx->GetCoreId());

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
            DEBUG("sp_el1\t\t%016llx\n", frame->sp_el1);
        }
        DEBUG("sp_el0\t\t%016llx\n", frame->sp_el0);
        DEBUG("cntpct_el0\t%016llx\n", frame->cntpct_el0);
        if (frame == currentCoreCtx->GetGuestFrame()) {
            DEBUG("cntp_ctl_el0\t%016llx\n", frame->cntp_ctl_el0);
            DEBUG("cntv_ctl_el0\t%016llx\n", frame->cntv_ctl_el0);
        } else if ((frame->sp_el2 & ~0xFFFul) + 0x1000 == stackTop) {
            // Try to dump the stack (comment if this crashes)
            u64 *sp = (u64 *)frame->sp_el2;
            u64 *spEnd = sp + 0x20;
            u64 *spMax = reinterpret_cast<u64 *>((frame->sp_el2 + 0xFFF) & ~0xFFFul);
            DEBUG("Stack trace:\n");
            while (sp < spEnd && sp < spMax) {
                DEBUG("\t%016lx\n", *sp++);
            }
        } else {
            DEBUG("Stack overflow/double fault detected!\n");
        }
#else
        (void)frame;
        (void)sameEl;
#endif
    }

    void ExceptionEntryPostprocess(ExceptionStackFrame *frame, bool isLowerEl)
    {
        if (frame == currentCoreCtx->GetGuestFrame()) {
            frame->cntp_ctl_el0 = THERMOSPHERE_GET_SYSREG(cntp_ctl_el0);
            frame->cntv_ctl_el0 = THERMOSPHERE_GET_SYSREG(cntv_ctl_el0);
        }
    }

    void ExceptionReturnPreprocess(ExceptionStackFrame *frame)
    {
        if (frame == currentCoreCtx->GetGuestFrame()) {
            if (currentCoreCtx->WasPaused()) { 
                // Were we paused & are we about to return to the guest?
                IrqManager::EnterInterruptibleHypervisorCode();
                while (!debugManagerHandlePause());
                FpuRegisterCache::GetInstance().CleanInvalidate;
            }

            // Update virtual counter
            u64 ticksNow = 0;// TODO timerGetSystemTick();
            currentCoreCtx->IncrementTotalTimeInHypervisor(ticksNow - frame->cntpct_el0);
            UpdateVirtualOffsetSysreg();

            // Restore timer interrupt config
            THERMOSPHERE_SET_SYSREG(cntp_ctl_el0, frame->cntp_ctl_el0);
            THERMOSPHERE_SET_SYSREG(cntv_ctl_el0, frame->cntv_ctl_el0);
        }
    }

    void HandleLowerElSyncException(ExceptionStackFrame *frame)
    {
        auto esr = frame->esr_el2;
        switch (esr.ec) {
            case cpu::ExceptionSyndromeRegister::CP15RTTrap:
                handleMcrMrcCP15Trap(frame, esr);
                break;
            case cpu::ExceptionSyndromeRegister::CP15RRTTrap:
                handleMcrrMrrcCP15Trap(frame, esr);
                break;
            case cpu::ExceptionSyndromeRegister::CP14RTTrap:
            case cpu::ExceptionSyndromeRegister::CP14DTTrap:
            case cpu::ExceptionSyndromeRegister::CP14RRTTrap:
                // A32 stub: Skip instruction, read 0 if necessary (there are debug regs at EL0)
                handleA32CP14Trap(frame, esr);
                break;
            case cpu::ExceptionSyndromeRegister::HypervisorCallA64:
                handleHypercall(frame, esr);
                break;
            case cpu::ExceptionSyndromeRegister::MonitorCallA64:
                handleSmcTrap(frame, esr);
                break;
            case cpu::ExceptionSyndromeRegister::SystemRegisterTrap:
                handleMsrMrsTrap(frame, esr);
                break;
            case cpu::ExceptionSyndromeRegister::DataAbortLowerEl:
                // Basically, stage2 translation faults
                handleLowerElDataAbortException(frame, esr);
                break;
            case cpu::ExceptionSyndromeRegister::SoftwareStepLowerEl:
                handleSingleStep(frame, esr);
                break;
            case cpu::ExceptionSyndromeRegister::BreakpointLowerEl:
            case cpu::ExceptionSyndromeRegister::WatchpointLowerEl:
            case cpu::ExceptionSyndromeRegister::SoftwareBreakpointA64:
            case cpu::ExceptionSyndromeRegister::SoftwareBreakpointA32:
                debugManagerReportEvent(DBGEVENT_EXCEPTION);
                break;
            default:
                DEBUG("Lower EL sync exception, EC = 0x%02llx IL=%llu ISS=0x%06llx\n", (u64)esr.ec, esr.il, esr.iss);
                DumpStackFrame(frame, false);
                break;
        }
    }

    void HandleSameElSyncException(ExceptionStackFrame *frame)
    {
        auto esr = frame->esr_el2;
        DEBUG("Same EL sync exception on core %x, EC = 0x%02x IL=%llu ISS=0x%06llx\n", currentCoreCtx->GetCoreId(), esr.ec, esr.il, esr.iss);
        DumpStackFrame(frame, true);
    }

    void HandleUnknownException(u32 offset)
    {
        DEBUG("Unknown exception on core %x! (offset 0x%03lx)\n", currentCoreCtx->GetCoreId(), offset);
    }

}
