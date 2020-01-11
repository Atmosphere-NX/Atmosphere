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

#include "irq.h"
#include "core_ctx.h"
#include "debug_log.h"
#include "vgic.h"
#include "timer.h"

IrqManager g_irqManager = {0};

static void initGic(void)
{
    // Reinits the GICD and GICC (for non-secure mode, obviously)
    if (currentCoreCtx->isBootCore && !currentCoreCtx->warmboot) {
        initGicV2Pointers(&g_irqManager.gic);

        // Disable interrupt handling & global interrupt distribution
        g_irqManager.gic.gicd->ctlr = 0;

        // Get some info
        g_irqManager.numSharedInterrupts = 32 * (g_irqManager.gic.gicd->typer & 0x1F); // number of interrupt lines / 32

        // unimplemented priority bits (lowest significant) are RAZ/WI
        g_irqManager.gic.gicd->ipriorityr[0] = 0xFF;
        g_irqManager.priorityShift = 8 - __builtin_popcount(g_irqManager.gic.gicd->ipriorityr[0]);
        g_irqManager.numPriorityLevels = (u8)BIT(__builtin_popcount(g_irqManager.gic.gicd->ipriorityr[0]));

        g_irqManager.numCpuInterfaces = (u8)(1 + ((g_irqManager.gic.gicd->typer >> 5) & 7));
        g_irqManager.numListRegisters = (u8)(1 + (g_irqManager.gic.gich->vtr & 0x3F));
    }

    volatile ArmGicV2Controller *gicc = g_irqManager.gic.gicc;
    volatile ArmGicV2Distributor *gicd = g_irqManager.gic.gicd;

    // Only one core will reset the GIC state for the shared peripheral interrupts

    u32 numInterrupts = 32;
    if (currentCoreCtx->isBootCore) {
        numInterrupts += g_irqManager.numSharedInterrupts;
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

    currentCoreCtx->gicInterfaceMask = gicd->itargetsr[0];
}

void configureInterrupt(u16 id, u8 prio, bool isLevelSensitive)
{
    volatile ArmGicV2Distributor *gicd = g_irqManager.gic.gicd;
    gicd->icenabler[id / 32] = BIT(id % 32);

    if (id >= 32) {
        u32 cfgr = gicd->icfgr[id / 16];
        cfgr &= ~(3 << IRQ_CFGR_SHIFT(id));
        cfgr |= (!isLevelSensitive ? 3 : 1) << IRQ_CFGR_SHIFT(id);
        gicd->icfgr[id / 16] = cfgr;
        gicd->itargetsr[id]  |= currentCoreCtx->gicInterfaceMask;
    }
    gicd->icpendr[id / 32]      = BIT(id % 32);
    gicd->ipriorityr[id]        = (prio << g_irqManager.priorityShift) & 0xFF;
    gicd->isenabler[id / 32]    = BIT(id % 32);
}

void initIrq(void)
{
    u64 flags = recursiveSpinlockLockMaskIrq(&g_irqManager.lock);

    initGic();
    vgicInit();

    // Configure the interrupts we use here
    for (u32 i = 0; i < ThermosphereSgi_Max; i++) {
        configureInterrupt(i, IRQ_PRIORITY_HOST, false);
    }

    configureInterrupt(GIC_IRQID_MAINTENANCE, IRQ_PRIORITY_HOST, true);

    recursiveSpinlockUnlockRestoreIrq(&g_irqManager.lock, flags);
}

static inline bool checkRescheduleEmulatedPtimer(ExceptionStackFrame *frame)
{
    // Evaluate if the timer really has expired in the PoV of the guest kernel. If not, reschedule (add missed time delta) it & exit early
    u64 cval = GET_SYSREG(cntp_cval_el0);
    if (cval > frame->cntvct_el0) {
        // It has not: reschedule the timer
        u64 offsetNow = GET_SYSREG(cntvoff_el2);
        SET_SYSREG(cntp_cval_el0, cval + (offsetNow - currentCoreCtx->emulPtimerOffsetThen));
        currentCoreCtx->emulPtimerOffsetThen = offsetNow;

        return false;
    }

    return true;
}

void handleIrqException(ExceptionStackFrame *frame, bool isLowerEl, bool isA32)
{
    (void)isLowerEl;
    (void)isA32;
    volatile ArmGicV2Controller *gicc = g_irqManager.gic.gicc;

    // Acknowledge the interrupt. Interrupt goes from pending to active.
    u32 iar = gicc->iar;
    u32 irqId = iar & 0x3FF;
    u32 srcCore = (iar >> 10) & 7;

    DEBUG("EL2 [core %d]: Received irq %x\n", (int)currentCoreCtx->coreId, irqId);

    if (irqId == GIC_IRQID_SPURIOUS) {
        // Spurious interrupt received
        return;
    } else if (irqId == GIC_IRQID_NS_PHYS_TIMER && !checkRescheduleEmulatedPtimer(frame)) {
        // Deactivate the ptimer interrupt, return early
        gicc->eoir = iar;
        gicc->dir  = iar;
        return;
    }

    bool isGuestInterrupt = false;
    bool isMaintenanceInterrupt = false;

    switch (irqId) {
        case ThermosphereSgi_ExecuteFunction:
            executeFunctionInterruptHandler(srcCore);
            break;
        case ThermosphereSgi_VgicUpdate:
            // Nothing in particular to do here
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

    // Priority drop
    gicc->eoir = iar;

    isGuestInterrupt = isGuestInterrupt && irqIsGuest(irqId);

    recursiveSpinlockLock(&g_irqManager.lock);

    if (!isGuestInterrupt) {
        if (isMaintenanceInterrupt) {
            vgicMaintenanceInterruptHandler();
        }
        // Deactivate the interrupt
        gicc->dir = iar;
    } else {
        vgicEnqueuePhysicalIrq(irqId);
    }

    // Update vgic state
    vgicUpdateState();

    recursiveSpinlockUnlock(&g_irqManager.lock);
}
