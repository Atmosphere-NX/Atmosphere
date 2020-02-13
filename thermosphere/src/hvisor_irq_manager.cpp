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

#include <mutex>

#include "hvisor_irq_manager.hpp"
#include "cpu/hvisor_cpu_interrupt_mask_guard.hpp"
#include "platform/interrupt_config.h"
#include "core_ctx.h"
#include "guest_timers.h"
#include "transport_interface.h"
#include "timer.h"

#include "vgic.h"
//#include "debug_manager.h"

namespace {

    inline bool checkRescheduleEmulatedPtimer(ExceptionStackFrame *frame)
    {
        // Evaluate if the timer has really expired in the PoV of the guest kernel.
        // If not, reschedule (add missed time delta) it & exit early
        u64 cval = currentCoreCtx->emulPtimerCval;
        u64 vct  = computeCntvct(frame);

        if (cval > vct) {
            // It has not: reschedule the timer
            // Note: this isn't 100% precise esp. on QEMU so it may take a few tries...
            writeEmulatedPhysicalCompareValue(frame, cval);
            return false;
        }

        return true;
    }

    inline bool checkGuestTimerInterrupts(ExceptionStackFrame *frame, u32 irqId)
    {
        // A thing that might have happened is losing the race vs disabling the guest interrupts
        // Another thing is that the virtual timer might have fired before us updating voff when executing a top half?
        if (irqId == TIMER_IRQID(NS_VIRT_TIMER)) {
            u64 cval = THERMOSPHERE_GET_SYSREG(cntp_cval_el0);
            return cval <= computeCntvct(frame);
        } else if (irqId == TIMER_IRQID(NS_PHYS_TIMER)) {
            return checkRescheduleEmulatedPtimer(frame);
        } else {
            return true;
        }
    }

}

namespace ams::hvisor {

    bool IrqManager::IsGuestInterrupt(u32 id)
    {
        // We don't care about the interrupts we don't use

        bool ret = true;
        ret = ret && id != GIC_IRQID_MAINTENANCE;
        ret = ret && id != GIC_IRQID_NS_PHYS_HYP_TIMER;

        ret = ret && transportInterfaceFindByIrqId(id) == NULL;
        return ret;
    }

    void IrqManager::InitializeGic()
    {
        // Reinits the GICD and GICC (for non-secure mode, obviously)
        if (currentCoreCtx->isBootCore && !currentCoreCtx->warmboot) {
            // Disable interrupt handling & global interrupt distribution
            gicd->ctlr = 0;

            // Get some info
            m_numSharedInterrupts = 32 * (gicd->typer & 0x1F); // number of interrupt lines / 32

            // unimplemented priority bits (lowest significant) are RAZ/WI
            gicd->ipriorityr[0] = 0xFF;
            m_priorityShift = 8 - __builtin_popcount(gicd->ipriorityr[0]);
            m_numPriorityLevels = static_cast<u8>(BIT(__builtin_popcount(gicd->ipriorityr[0])));

            m_numCpuInterfaces = static_cast<u8>(1 + ((gicd->typer >> 5) & 7));
            m_numListRegisters = static_cast<u8>(1 + (gich->vtr & 0x3F));
        }

        // Only one core will reset the GIC state for the shared peripheral interrupts

        u32 numInterrupts = 32;
        if (currentCoreCtx->isBootCore) {
            numInterrupts += m_numSharedInterrupts;
        }

        // Filter all interrupts
        gicc->pmr = 0;

        // Disable interrupt preemption
        gicc->bpr = 7;

        // Note: the GICD I...n regs are banked for private interrupts

        // Disable all interrupts, clear active status, clear pending status
        for (u32 i = 0; i < numInterrupts / 32; i++) {
            gicd->icenabler[i] = 0xFFFFFFFF;
            gicd->icactiver[i] = 0xFFFFFFFF;
            gicd->icpendr[i] = 0xFFFFFFFF;
        }

        // Set priorities to lowest
        for (u32 i = 0; i < numInterrupts; i++) {
            gicd->ipriorityr[i] = 0xFF;
        }

        // Reset icfgr, itargetsr for shared peripheral interrupts
        for (u32 i = 32 / 16; i < numInterrupts / 16; i++) {
            gicd->icfgr[i] = 0x55555555;
        }

        for (u32 i = 32; i < numInterrupts; i++) {
            gicd->itargetsr[i] = 0;
        }

        // Now, reenable interrupts

        // Enable the distributor
        if (currentCoreCtx->isBootCore) {
            gicd->ctlr = 1;
        }

        // Enable the CPU interface. Set EOIModeNS=1 (split prio drop & deactivate priority)
        gicc->ctlr = BIT(9) | 1;

        // Disable interrupt filtering
        gicc->pmr = 0xFF;
    }

    void IrqManager::DoConfigureInterrupt(u32 id, u8 prio, bool isLevelSensitive)
    {
        ClearInterruptEnabled(id);
        ClearInterruptPending(id);
        if (id >= 32) {
            SetInterruptMode(id, isLevelSensitive);
            SetInterruptTargets(id, 0xFF); // all possible processors
        }
        SetInterruptPriority(id, prio);
        SetInterruptEnabled(id);
    }

    void IrqManager::Initialize()
    {
        cpu::InterruptMaskGuard mg{};
        std::scoped_lock lk{m_lock};

        InitializeGic();
        for (u32 i = 0; i < MaxSgi; i++) {
            DoConfigureInterrupt(i, hostPriority, false);
        }

        DoConfigureInterrupt(GIC_IRQID_MAINTENANCE, hostPriority, true);

        vgicInit();
    }

    void IrqManager::ConfigureInterrupt(u32 id, u8 prio, bool isLevelSensitive)
    {
        cpu::InterruptMaskGuard mg{};
        std::scoped_lock lk{m_lock};

        DoConfigureInterrupt(id, prio, isLevelSensitive);
    }

    void IrqManager::SetInterruptAffinity(u32 id, u8 affinity)
    {
        cpu::InterruptMaskGuard mg{};
        std::scoped_lock lk{m_lock};

        SetInterruptTargets(id, affinity);
    }

    void IrqManager::HandleInterrupt(ExceptionStackFrame *frame)
    {
        // TODO refactor c parts

        // Acknowledge the interrupt. Interrupt goes from pending to active.
        u32 iar = AcknowledgeIrq();
        u32 irqId = iar & 0x3FF;
        u32 srcCore = (iar >> 10) & 7;

        //DEBUG("EL2 [core %d]: Received irq %x\n", (int)currentCoreCtx->coreId, irqId);

        if (irqId == GicV2Distributor::spuriousIrqId) {
            // Spurious interrupt received
            return;
        } else if (!checkGuestTimerInterrupts(frame, irqId)) {
            // Deactivate the interrupt, return early
            DropCurrentInterruptPriority(iar);
            DeactivateCurrentInterrupt(iar);
            return;
        }

        bool isGuestInterrupt = false;
        bool isMaintenanceInterrupt = false;
        bool isPaused = false;
        bool hasDebugEvent = false;

        switch (irqId) {
            case ExecuteFunctionSgi:
                executeFunctionInterruptHandler(srcCore);
                break;
            case VgicUpdateSgi:
                // Nothing in particular to do here
                break;
            case DebugPauseSgi:
                // TODO debugManagerPauseSgiHandler();
                break;
            case ReportDebuggerBreakSgi:
            case DebuggerContinueSgi:
                // See bottom halves
                // Because exceptions (other debug events) are handling w/ interrupts off, if
                // we get there, there's no race condition possible with debugManagerReportEvent
                break;
            case GIC_IRQID_MAINTENANCE:
                isMaintenanceInterrupt = true;
                break;
            case TIMER_IRQID(CURRENT_TIMER):
                timerInterruptHandler();
                break;
            default:
                isGuestInterrupt = irqId >= 16;
                break;
        }

        TransportInterface *transportIface = irqId >= 32 ? transportInterfaceIrqHandlerTopHalf(irqId) : NULL;

        // Priority drop
        DropCurrentInterruptPriority(iar);

        isGuestInterrupt = isGuestInterrupt && transportIface == NULL && IsGuestInterrupt(irqId);

        instance.m_lock.lock();

        if (!isGuestInterrupt) {
            if (isMaintenanceInterrupt) {
                vgicMaintenanceInterruptHandler();
            }
            // Deactivate the interrupt
            DeactivateCurrentInterrupt(iar);
        } else {
            vgicEnqueuePhysicalIrq(irqId);
        }

        // Update vgic state
        vgicUpdateState();

        instance.m_lock.unlock();

        // TODO

        /*isPaused = debugManagerIsCorePaused(currentCoreCtx->coreId);
        hasDebugEvent = debugManagerHasDebugEvent(currentCoreCtx->coreId);
        if (irqId == ThermosphereSgi_ReportDebuggerBreak) DEBUG("debug event=%d\n", (int)debugManagerGetDebugEvent(currentCoreCtx->coreId)->type);
        // Bottom half part
        if (transportIface != NULL) {
            exceptionEnterInterruptibleHypervisorCode();
            unmaskIrq();
            transportInterfaceIrqHandlerBottomHalf(transportIface);
        } else if (irqId == ThermosphereSgi_ReportDebuggerBreak && !hasDebugEvent) {
            debugManagerReportEvent(DBGEVENT_DEBUGGER_BREAK);
        } else if (irqId == DebuggerContinueSgi && isPaused) {
            debugManagerUnpauseCores(BIT(currentCoreCtx->coreId));
        }*/

    }
}
