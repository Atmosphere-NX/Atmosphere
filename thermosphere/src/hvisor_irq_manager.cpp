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
#include "hvisor_virtual_gic.hpp"
#include "hvisor_core_context.hpp"
#include "hvisor_guest_timers.hpp"

#include "cpu/hvisor_cpu_interrupt_mask_guard.hpp"
#include "platform/interrupt_config.h"
#include "transport_interface.h"
#include "timer.h"

//#include "debug_manager.h"

namespace {

    inline bool CheckGuestTimerInterrupts(ams::hvisor::ExceptionStackFrame *frame, u32 irqId)
    {
        // A thing that might have happened is losing the race vs disabling the guest interrupts
        // Another thing is that the virtual timer might have fired before us updating voff when executing a top half?
        if (irqId == TIMER_IRQID(NS_VIRT_TIMER)) {
            u64 cval = THERMOSPHERE_GET_SYSREG(cntp_cval_el0);
            return cval <= ams::hvisor::ComputeCntvct(frame);
        } else if (irqId == TIMER_IRQID(NS_PHYS_TIMER)) {
            return ams::hvisor::CheckRescheduleEmulatedPtimer(frame);
        } else {
            return true;
        }
    }

}

namespace ams::hvisor {

    bool IrqManager::IsGuestInterrupt(u32 id)
    {
        // We don't care about the interrupts we don't use
        // Special interrupts id (eg. spurious interrupt id 1023) are also reserved to us
        // because the virtual interface hw itself will generate it for the guest.

        bool ret = id <= GicV2Distributor::maxIrqId && id != GIC_IRQID_MAINTENANCE && id != GIC_IRQID_NS_PHYS_HYP_TIMER;
        ret = ret && transportInterfaceFindByIrqId(id) == NULL;
        return ret;
    }

    void IrqManager::InitializeGic()
    {
        // Reinits the GICD and GICC (for non-secure mode, obviously)
        if (currentCoreCtx->IsBootCore() && currentCoreCtx->IsColdboot()) {
            // Disable interrupt handling & global interrupt distribution
            gicd->ctlr = 0;

            // Get some info
            m_numSharedInterrupts = 32 * (gicd->typer & 0x1F); // number of interrupt lines / 32

            // unimplemented priority bits (lowest significant) are RAZ/WI
            gicd->ipriorityr[0] = 0xFF;
            m_priorityShift = 8 - __builtin_popcount(gicd->ipriorityr[0]);
            m_numPriorityLevels = static_cast<u8>(BIT(__builtin_popcount(gicd->ipriorityr[0])));

            m_numCpuInterfaces = static_cast<u8>(1 + ((gicd->typer >> 5) & 7));
        }

        // Only one core will reset the GIC state for the shared peripheral interrupts

        u32 numInterrupts = 32;
        if (currentCoreCtx->IsBootCore()) {
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
        if (currentCoreCtx->IsBootCore()) {
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
            SetInterruptMode(id, !isLevelSensitive);
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
        DoConfigureInterrupt(GIC_IRQID_MAINTENANCE, hostPriority, true);

        VirtualGic::GetInstance().Initialize();
    }

    void IrqManager::Register(IInterruptTask &task, u32 id, bool isLevelSensitive, u8 prio)
    {
        cpu::InterruptMaskGuard mg{};
        std::scoped_lock lk{m_lock};

        DoConfigureInterrupt(id, prio, isLevelSensitive);
        if (!task.IsLinked()) {
            m_interruptTaskList.push_back(task);
        }
    }

    void IrqManager::SetInterruptAffinity(u32 id, u8 affinity)
    {
        cpu::InterruptMaskGuard mg{};
        std::scoped_lock lk{m_lock};

        SetInterruptTargets(id, affinity);
    }

    void IrqManager::HandleInterrupt(ExceptionStackFrame *frame)
    {
        // Acknowledge the interrupt. Interrupt goes from pending to active.
        u32 iar = AcknowledgeIrq();
        u32 irqId = iar & 0x3FF;
        u32 srcCore = (iar >> 10) & 7;
        IInterruptTask *taskForBottomHalf;

        //DEBUG("EL2 [core %d]: Received irq %x\n", (int)currentCoreCtx->coreId, irqId);
        if (irqId == GicV2Distributor::spuriousIrqId) {
            // Spurious interrupt received
            return;
        } else if (!CheckGuestTimerInterrupts(frame, irqId)) {
            // Deactivate the interrupt, return ASAP
            DropCurrentInterruptPriority(iar);
            DeactivateCurrentInterrupt(iar);
            return;
        } else {
            // Everything else
            std::scoped_lock lk{instance.m_lock};
            VirtualGic &vgic = VirtualGic::GetInstance();

            if (irqId >= 16 && IsGuestInterrupt(irqId)) {
                // Guest interrupts
                taskForBottomHalf = nullptr;
                DropCurrentInterruptPriority(iar);
                vgic.EnqueuePhysicalIrq(irqId);
            } else {
                // Host interrupts
                // Try all handlers and see which one fits
                for (IInterruptTask &task: instance.m_interruptTaskList) {
                    auto b = task.InterruptTopHalfHandler(irqId, srcCore);
                    if (b) {
                        taskForBottomHalf = *b ? &task : nullptr;
                        break;
                    }
                }
                DropCurrentInterruptPriority(iar);
                DeactivateCurrentInterrupt(iar);
            }

            vgic.UpdateState();
        }


        if (taskForBottomHalf != nullptr) {
            // Unmasking the irq signal is left at the discretion of the bottom half handler
            EnterInterruptibleHypervisorCode();
            taskForBottomHalf->InterruptBottomHalfHandler(irqId, srcCore);
        }
    }
}
