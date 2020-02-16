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
#include "hvisor_virtual_gic.hpp"
#include "cpu/hvisor_cpu_instructions.hpp"

#include "platform/interrupt_config.h" // TODO remove

#define GICDOFF(field)          (offsetof(GicV2Distributor, field))

namespace ams::hvisor {


    VirtualGic::VirqQueue::iterator VirtualGic::VirqQueue::insert(VirtualGic::VirqQueue::iterator pos, VirtualGic::VirqState &elem)
    {
        // Insert before
        ENSURE(!elem.IsQueued());

        // Empty list
        if (begin() == end()) {
            m_first = m_last = &elem;
            elem.listPrev = elem.listNext = virqListEndIndex;
            return begin();
        }

        if (pos == end()) {
            // Insert after last
            VirqState &prev = back();
            elem.listPrev = GetStateIndex(prev);
            elem.listNext = prev.listNext;
            prev.listNext = GetStateIndex(elem);
            m_last = &elem;
        } else {
            u32 idx = GetStateIndex(elem);
            u32 posidx = GetStateIndex(*pos);
            u32 previdx = elem.listPrev;

            elem.listNext = posidx;
            elem.listPrev = previdx;

            pos->listPrev = idx;

            if (pos == begin()) {
                m_first = &elem;
            } else {
                --pos;
                pos->listNext = idx;
            }
        }

        return iterator{&elem, m_storage};
    }

    VirtualGic::VirqQueue::iterator VirtualGic::VirqQueue::insert(VirtualGic::VirqState &elem)
    {
        // Insert in a stable sorted way
        // Lower priority number is higher; we sort by descending priority, ie. ascending priority number
        // Put the interrupts that were previously in the LR before the one which don't
        return insert(
            std::find_if(begin(), end(), [&a = elem](const VirqState &b) {
                return a.priority == b.priority ? a.handled && !b.handled : a.priority < b.priority;
            }),
            elem
        );
    }

    void VirtualGic::SetInterruptEnabledState(u32 id)
    {
        VirqState &state = GetVirqState(id);

        if (id < 16 || !IrqManager::IsGuestInterrupt(id) || state.enabled) {
            // Nothing to do...
            // Also, ignore for SGIs
            return;
        }

        // Similar effects to setting the target list to non-0 when it was 0...
        if (state.IsPending()) {
            NotifyOtherCoreList(state.targetList);
        }

        state.enabled = true;
        IrqManager::SetInterruptEnabled(id);
    }

    void VirtualGic::ClearInterruptEnabledState(u32 id)
    {
        VirqState &state = GetVirqState(id);

        if (id < 16 || !IrqManager::IsGuestInterrupt(id) || !state.enabled) {
            // Nothing to do...
            // Also, ignore for SGIs
            return;
        }

        // Similar effects to setting the target list to 0, we may need to notify the core
        // handling the interrupt if it's pending
        if (state.handled) {
            NotifyOtherCoreList(BIT(state.coreId));
        }

        state.enabled = false;
        IrqManager::ClearInterruptEnabled(id);
    }

    void VirtualGic::SetInterruptPriorityByte(u32 id, u8 priority)
    {
        if (!IrqManager::IsGuestInterrupt(id)) {
            return;
        }

        // 32 priority levels max, bits [7:3]
        priority >>= priorityShift;

        if (id >= 16) {
            // Ensure we have the correct priority on the physical distributor...
            IrqManager::GetInstance().SetInterruptPriority(id, IrqManager::guestPriority);
        }

        VirqState &state = GetVirqState(id);
        if (priority == state.priority) {
            // Nothing to do...
            return;
        }

        state.priority = priority;
        u32 targets = state.targetList;
        if (targets != 0 && state.IsPending()) {
            NotifyOtherCoreList(targets);
        }
    }

    void VirtualGic::SetInterruptTargets(u32 id, u8 coreList)
    {
        // Ignored for SGIs and PPIs, and non-guest interrupts
        if (id < 32 || !IrqManager::IsGuestInterrupt(id)) {
            return;
        }

        // Interrupt not pending (inactive or active-only): nothing much to do (see reference manual)
        // Otherwise, we may need to migrate the interrupt.
        // In our model, while a physical interrupt can be pending on multiple cores, we decide that a pending SPI
        // can only be handled on a single core (either it's in a LR, or in the global list), therefore we need to
        // send a signal to (oldList XOR newList) to either put the interrupt back in the global list or potentially handle it

        // Note that we take into account that the interrupt may be disabled.
        VirqState &state = GetVirqState(id);
        if (state.IsPending()) {
            u8 oldList = state.targetList;
            u8 diffList = (oldList ^ coreList) & getActiveCoreMask();
            if (diffList != 0) {
                NotifyOtherCoreList(diffList);
            }
        }

        state.targetList = coreList;
        IrqManager::SetInterruptTargets(id, state.targetList);
    }

    void VirtualGic::SetInterruptConfigBits(u32 id, u32 config)
    {
        // Ignored for SGIs, implementation defined for PPIs
        if (id < 32 || !IrqManager::IsGuestInterrupt(id)) {
            return;
        }

        VirqState &state = GetVirqState(id);

        // Expose bit(2n) as nonprogrammable to the guest no matter what the physical distributor actually behaves
        bool newEdgeTriggered = ((config & 2) << GicV2Distributor::GetCfgrShift(id)) != 0;

        if (state.edgeTriggered != newEdgeTriggered) {
            state.edgeTriggered = newEdgeTriggered;
            IrqManager::SetInterruptMode(id, newEdgeTriggered);
        }
    }

    void VirtualGic::SetSgiPendingState(u32 id, u32 coreId, u32 srcCoreId)
    {
        VirqState &state = GetVirqState(coreId, id);
        m_incomingSgiPendingSources[coreId][id] |= BIT(srcCoreId);
        if (!state.handled && !state.IsQueued()) {
            // The SGI was inactive on the target core...
            state.SetPending();
            state.srcCoreId = srcCoreId;
            m_incomingSgiPendingSources[coreId][id] &= ~BIT(srcCoreId);
            m_virqPendingQueue.insert(state);
            NotifyOtherCoreList(BIT(coreId));
        }
    }

    void VirtualGic::SendSgi(u32 id, GicV2Distributor::SgirTargetListFilter filter, u32 coreList)
    {
        switch (filter) {
            case GicV2Distributor::ForwardToTargetList:
                // Forward to coreList
                break;
            case GicV2Distributor::ForwardToAllOthers:
                // Forward to all but current core
                coreList = ~BIT(currentCoreCtx->coreId);
                break;
            case GicV2Distributor::ForwardToSelf:
                // Forward to current core only
                coreList = BIT(currentCoreCtx->coreId);
                break;
            default:
                DEBUG("Emulated GCID_SGIR: invalid TargetListFilter value!\n");
                return;
        }

        coreList &= getActiveCoreMask();
        for (u32 dstCore: util::BitsOf{coreList}) {
            SetSgiPendingState(id, dstCore, currentCoreCtx->coreId);
        }
    }

    bool VirtualGic::ValidateGicdRegisterAccess(size_t offset, size_t sz)
    {
        // ipriorityr, itargetsr, *pendsgir are byte-accessible
        // Report a fault on accessing fields for 
        if (
            !(offset >= GICDOFF(ipriorityr) && offset < GICDOFF(ipriorityr) + GicV2Distributor::maxIrqId) &&
            !(offset >= GICDOFF(itargetsr)  && offset < GICDOFF(itargetsr) + GicV2Distributor::maxIrqId) &&
            !(offset >= GICDOFF(cpendsgir)  && offset < GICDOFF(cpendsgir) + 16) &&
            !(offset >= GICDOFF(spendsgir)  && offset < GICDOFF(spendsgir) + 16)
        ) {
            return (offset & 3) == 0 && sz == 4;
        } else {
            return sz == 1 || (sz == 4 && ((offset & 3) != 0));
        }
    }

    void VirtualGic::WriteGicdRegister(u32 val, size_t offset, size_t sz)
    {
        static constexpr auto maxIrqId = GicV2Distributor::maxIrqId;
        std::scoped_lock lk{IrqManager::GetInstance().m_lock};

        switch (offset) {
            case GICDOFF(typer):
            case GICDOFF(iidr):
            case GICDOFF(icpidr2):
            case GICDOFF(itargetsr) ... GICDOFF(itargetsr) + 31:
                // Write ignored (read-only registers)
                break;
            case GICDOFF(icfgr) ... GICDOFF(icfgr) + 31/4:
                // Write ignored because of an implementation-defined choice
                break;
            case GICDOFF(igroupr) ... GICDOFF(igroupr) + maxIrqId/8:
                // Write ignored because we don't implement Group 1 here
                break;
            case GICDOFF(ispendr) ... GICDOFF(ispendr) + maxIrqId/8:
            case GICDOFF(icpendr) ... GICDOFF(icpendr) + maxIrqId/8:
            case GICDOFF(isactiver) ... GICDOFF(isactiver) + maxIrqId/8:
            case GICDOFF(icactiver) ... GICDOFF(icactiver) + maxIrqId/8:
            case GICDOFF(cpendsgir) ... GICDOFF(cpendsgir) + 15:
            case GICDOFF(spendsgir) ... GICDOFF(spendsgir) + 15:
                // Write ignored, not implemented (at least not yet, TODO)
                break;

            case GICDOFF(ctlr): {
                SetDistributorControlRegister(val);
                break;
            }

            case GICDOFF(isenabler) ... GICDOFF(isenabler) + maxIrqId/8: {
                u32 base = 8 * static_cast<u32>(offset - GICDOFF(isenabler));
                for(u32 pos: util::BitsOf{val}) {
                    SetInterruptEnabledState(base + pos);
                }
                break;
            }
            case GICDOFF(icenabler) ... GICDOFF(icenabler) + maxIrqId/8: {
                u32 base = 8 * static_cast<u32>(offset - GICDOFF(icenabler));
                for(u32 pos: util::BitsOf{val}) {
                    SetInterruptEnabledState(base + pos);
                }
                break;
            }

            case GICDOFF(ipriorityr) ... GICDOFF(ipriorityr) + maxIrqId: {
                u32 base = static_cast<u32>(offset - GICDOFF(ipriorityr));
                for (u32 i = 0; i < static_cast<u32>(sz); i++) {
                    SetInterruptPriorityByte(base + i, static_cast<u8>(val));
                    val >>= 8;
                }
                break;
            }

            case GICDOFF(itargetsr) + 32 ... GICDOFF(itargetsr) + maxIrqId: {
                u32 base = static_cast<u32>(offset - GICDOFF(itargetsr));
                for (u32 i = 0; i < static_cast<u32>(sz); i++) {
                    SetInterruptTargets(base + i, static_cast<u8>(val));
                    val >>= 8;
                }
                break;
            }

            case GICDOFF(icfgr) + 32/4 ... GICDOFF(icfgr) + maxIrqId/4: {
                u32 base = 4 * static_cast<u32>(offset & 0xFF);
                for (u32 i = 0; i < 16; i++) {
                    SetInterruptConfigBits(base + i, val & 3);
                    val >>= 2;
                }
                break;
            }

            case GICDOFF(sgir): {
                SendSgi(val & 0xF, static_cast<GicV2Distributor::SgirTargetListFilter>((val >> 24) & 3), (val >> 16) & 0xFF);
                break;
            }

            default:
                DEBUG("Write to GICD reserved/implementation-defined register offset=0x%03lx value=0x%08lx", offset, val);
                break;
        }

        UpdateState();
    }


    u32 VirtualGic::ReadGicdRegister(size_t offset, size_t sz)
    {
        static constexpr auto maxIrqId = GicV2Distributor::maxIrqId;
        std::scoped_lock lk{IrqManager::GetInstance().m_lock};

        //DEBUG("gicd read off 0x%03llx sz %lx\n", offset, sz);
        u32 val = 0;

        switch (offset) {
            case GICDOFF(icfgr) ... GICDOFF(icfgr) + 31/4:
                // RAZ because of an implementation-defined choice
                break;
            case GICDOFF(igroupr) ... GICDOFF(igroupr) + maxIrqId/8:
                // RAZ because we don't implement Group 1 here
                break;
            case GICDOFF(ispendr) ... GICDOFF(ispendr) + maxIrqId/8:
            case GICDOFF(icpendr) ... GICDOFF(icpendr) + maxIrqId/8:
            case GICDOFF(isactiver) ... GICDOFF(isactiver) + maxIrqId/8:
            case GICDOFF(icactiver) ... GICDOFF(icactiver) + maxIrqId/8:
            case GICDOFF(cpendsgir) ... GICDOFF(cpendsgir) + 15:
            case GICDOFF(spendsgir) ... GICDOFF(spendsgir) + 15:
                // RAZ, not implemented (at least not yet, TODO)
                break;

            case GICDOFF(ctlr): {
                val = GetDistributorControlRegister();
                break;
            }
            case GICDOFF(typer): {
                val = GetDistributorTypeRegister();
                break;
            }
            case GICDOFF(iidr): {
                val = GetDistributorImplementerIdentificationRegister();
                break;
            }

            case GICDOFF(isenabler) ... GICDOFF(isenabler) + maxIrqId/8:
            case GICDOFF(icenabler) ... GICDOFF(icenabler) + maxIrqId/8: {
                u32 base = 8 * static_cast<u32>(offset & 0x7F);
                for (u32 i = 0; i < 32; i++) {
                    val |= GetInterruptEnabledState(base + i) ? BIT(i) : 0;
                }
                break;
            }

            case GICDOFF(ipriorityr) ... GICDOFF(ipriorityr) + maxIrqId: {
                u32 base = static_cast<u32>(offset - GICDOFF(ipriorityr));
                for (u32 i = 0; i < sz; i++) {
                    val |= GetInterruptPriorityByte(base + i) << (8 * i);
                }
                break;
            }

            case GICDOFF(itargetsr) ... GICDOFF(itargetsr) + maxIrqId: {
                u32 base = static_cast<u32>(offset - GICDOFF(itargetsr));
                for (u32 i = 0; i < sz; i++) {
                    val |= GetInterruptTargets(base + i) << (8 * i);
                }
                break;
            }

            case GICDOFF(icfgr) + 32/4 ... GICDOFF(icfgr) + maxIrqId/4: {
                u32 base = 4 * static_cast<u32>(offset & 0xFF);
                for (u32 i = 0; i < 16; i++) {
                    val |= GetInterruptConfigBits(base + i) << (2 * i);
                }
                break;
            }

            case GICDOFF(sgir):
                // Write-only register
                DEBUG("Read from write-only register GCID_SGIR\n");
                break;

            case GICDOFF(icpidr2): {
                val = GetPeripheralId2Register();
                break;
            }

            default:
                DEBUG("Read from GICD reserved/implementation-defined register offset=0x%03lx\n", offset);
                break;
        }

        UpdateState();
        return val;
    }

    void VirtualGic::ResampleVirqLevel(VirtualGic::VirqState &state)
    {
        /*
            For hardware interrupts, we have kept the interrupt active on the physical GICD
            For level-sensitive interrupts, we need to check if they're also still physically pending (resampling).
            If not, there's nothing to service anymore, and therefore we have to deactivate them, so that
            we're notified when they become pending again.
         */

        if (state.edgeTriggered || !state.IsPending()) {
            // Nothing to do for edge-triggered interrupts and non-pending interrupts
            return;
        }

        u32 irqId = state.irqId;

        // Can't do anything for level-sensitive PPIs from other cores either
        if (irqId < 32 && state.coreId != currentCoreCtx->coreId) {
            return;
        }

        bool lineLevel = IrqManager::IsInterruptPending(irqId);
        if (!lineLevel) {
            IrqManager::ClearInterruptActive(irqId);
            state.ClearPendingLine();
        }
    }

    void VirtualGic::CleanupPendingQueue()
    {
        // SGIs are pruned elsewhere

        // Resample line level for level-sensitive interrupts
        for (VirqState &state: m_virqPendingQueue) {
            ResampleVirqLevel(state);
        }

        // Cleanup the list
        m_virqPendingQueue.erase_if([](const VirqState &state) { return !state.IsPending(); });
    }

    size_t VirtualGic::ChoosePendingInterrupts(VirtualGic::VirqState *chosen[], size_t maxNum)
    {
        size_t numChosen = 0;
        auto pred = [](const VirqState &state) {
            if (state.irqId < 32 && state.coreId != currentCoreCtx->coreId) {
                // We can't handle SGIs/PPIs of other cores.
                return false;
            }

            return state.enabled  && (state.irqId < 32 || (state.targetList & BIT(currentCoreCtx->coreId)) != 0);
        };

        for (VirqState &state: m_virqPendingQueue) {
            if (pred(state)) {
                chosen[numChosen++] = &state;
            }
        }

        for (size_t i = 0; i < numChosen; i++) {
            chosen[i]->handled = true;
            chosen[i]->coreId = currentCoreCtx->coreId;
            m_virqPendingQueue.erase(*chosen[i]);
        }
    }

    void VirtualGic::PushListRegisters(VirqState *chosen[], size_t num)
    {
        for (size_t i = 0; i < num; i++) {
            VirqState &state = *chosen[i];
            u32 irqId = state.irqId;

            GicV2VirtualInterfaceController::ListRegister lr = {0};
            lr.grp1 = false; // group0
            lr.priority = state.priority;
            lr.virtualId = irqId;

            // We only add new pending interrupts here...
            lr.pending = true;
            lr.active = false;

            // We don't support guests setting the pending latch, so the logic is probably simpler...

            if (irqId < 16) {
                // SGI
                lr.physicalId = BIT(9) /* EOI notification bit */ | state.srcCoreId;
                // ^ IDK how kvm gets away with not setting the EOI notif bits in some cases,
                // what they do seems to be prone to drop interrupts, etc.

                lr.hw = false; // software
            } else {
                // Actual physical interrupt
                lr.hw = true;
                lr.physicalId = irqId;
            }

            volatile auto *freeLr = AllocateListRegister();
            ENSURE(freeLr != nullptr);
            freeLr->raw = lr.raw;
        }
    }

    bool VirtualGic::UpdateListRegister(volatile GicV2VirtualInterfaceController::ListRegister *lr)
    {
        GicV2VirtualInterfaceController::ListRegister lrCopy = { .raw = lr->raw };

        u32 irqId = lrCopy.virtualId;

        // Note: this give priority to multi-SGIs than can be immediately handled

        // Update the state
        VirqState &state = GetVirqState(irqId);
        ENSURE(state.handled);

        u32 srcCoreId = state.coreId;
        u32 coreId = currentCoreCtx->coreId;

        state.active = lrCopy.active;

        if (lrCopy.active) {
            // We don't dequeue active interrupts

            if (irqId < 16) {
                // We can allow SGIs to be marked active-pending if it's been made pending from the same source again
                // For hw interrupts, the active-pending state is tracked in the real GICD
                if (m_incomingSgiPendingSources[coreId][irqId] & BIT(srcCoreId)) {
                    lrCopy.pending = true;
                    m_incomingSgiPendingSources[coreId][irqId] &= ~BIT(srcCoreId);
                }
            }

            // If the vIRQ goes from pending to active, it has been acknowledged: clear line level and pending latch
            // SGIs are always edge-triggered, so line level doesn't matter & that's why we handle them above to simplify the code
            if (!lrCopy.pending) {
                state.ClearPendingOnAck();
            }
            lr->raw = lrCopy.raw;
            return true;
        } else if (lrCopy.pending) {
            // New interrupts might have come, pending status might have been changed, etc.
            // We need to put the interrupt back in the pending list (which we clean up afterwards)
            state.handled = false;
            m_virqPendingQueue.insert(state);
            lr->raw = 0;
            return false;
        } else {
            // Interrupt is inactive. This means it has been acked and handled.
            // SGIs are always edge-triggered, so line level doesn't matter & that's why we handle them above to simplify the code

            if (irqId < 16) {
                // Special case for multi-SGIs if they can be immediately handled
                if (m_incomingSgiPendingSources[coreId][irqId] != 0) {
                    srcCoreId = __builtin_ctz(m_incomingSgiPendingSources[coreId][irqId]);
                    state.srcCoreId = srcCoreId;
                    m_incomingSgiPendingSources[coreId][irqId] &= ~BIT(srcCoreId);
                    lrCopy.physicalId = BIT(9) /* EOI notification bit */ | srcCoreId;

                    lrCopy.pending = true;
                    lr->raw = lrCopy.raw;
                }
            }

            if (!lrCopy.pending) {
                // Inactive interrupt, cleanup
                state.ClearPendingOnAck();
                state.handled = false;
                lr->raw = 0;
                return false;
            } else {
                return true;
            }
        }
    }

    void VirtualGic::UpdateState()
    {
        GicV2VirtualInterfaceController::HypervisorControlRegister hcr = { .raw = gich->hcr.raw };
        u32 coreId = currentCoreCtx->coreId;

        // First, put back inactive interrupts into the queue, handle some SGI stuff
        // Need to handle the LRs in reverse order to keep list stability
        u64 usedMap = cpu::rbit(m_usedLrMap[coreId]);
        for (auto pos: util::BitsOf{usedMap}) {
            if (!UpdateListRegister(&gich->lr[63 - pos])) {
                usedMap &= ~BITL(pos);
            }
        }
        m_usedLrMap[coreId] = cpu::rbit(usedMap);

        // Then, clean the list up
        CleanupPendingQueue();

        size_t numFreeLr = GetNumberOfFreeListRegisters();
        VirqState *chosen[64];

        // Choose interrupts...
        size_t numChosen = ChoosePendingInterrupts(chosen, numFreeLr);

        // ...and push them
        PushListRegisters(chosen, numChosen);

        // Enable underflow interrupt when appropriate to do so
        hcr.uie = m_numListRegisters - GetNumberOfFreeListRegisters() > 1;
        gich->hcr.raw = hcr.raw;
    }

    std::optional<bool> VirtualGic::InterruptTopHalfHandler(u32 irqId, u32)
    {
        if (irqId == IrqManager::VgicUpdateSgi) {
            // This SGI is just there to trigger the state update
            return false;
        } else if (irqId != GIC_IRQID_MAINTENANCE) {
            return {};
        }

        // Maintenance interrupt handler:
        GicV2VirtualInterfaceController::MaintenanceIntStatRegister misr = { .raw = gich->misr.raw };

        // Force GICV_CTRL to behave like ns-GICC_CTLR, with group 1 being replaced by group 0
        // Ensure we aren't spammed by maintenance interrupts, either.
        if (misr.vgrp0e || misr.vgrp0d || misr.vgrp1e || misr.vgrp1d) {
            GicV2VirtualInterfaceController::VmControlRegister vmcr = { .raw = gich->vmcr.raw };
            vmcr.cbpr = 0;
            vmcr.fiqEn = 0;
            vmcr.ackCtl = 0;
            vmcr.enableGrp1 = 0;
            gich->vmcr.raw = vmcr.raw;
        }

        if (misr.vgrp0e) {
            DEBUG("EL2 [core %d]: Group 0 enabled maintenance interrupt\n", (int)currentCoreCtx->coreId);
            gich->hcr.vgrp0eie = false;
            gich->hcr.vgrp0die = true;
        } else if (misr.vgrp0d) {
            DEBUG("EL2 [core %d]: Group 0 disabled maintenance interrupt\n", (int)currentCoreCtx->coreId);
            gich->hcr.vgrp0eie = true;
            gich->hcr.vgrp0die = false;
        }

        // Already handled the following 2 above:
        if (misr.vgrp1e) {
            DEBUG("EL2 [core %d]: Group 1 enabled maintenance interrupt\n", (int)currentCoreCtx->coreId);
        }
        if (misr.vgrp1d) {
            DEBUG("EL2 [core %d]: Group 1 disabled maintenance interrupt\n", (int)currentCoreCtx->coreId);
        }

        if (misr.eoi) {
            //DEBUG("EL2 [core %d]: SGI EOI maintenance interrupt\n", currentCoreCtx->coreId);
        }

        if (misr.u) {
            //DEBUG("EL2 [core %d]: Underflow maintenance interrupt\n", currentCoreCtx->coreId);
        }

        ENSURE2(!misr.lrenp, "List Register Entry Not Present maintenance interrupt!\n");

        // The rest should be handled by the main loop...
        return false;
    }

    void VirtualGic::EnqueuePhysicalIrq(u32 id)
    {
        VirqState &state = GetVirqState(id);
        state.SetPending();
        m_virqPendingQueue.insert(state);
    }

    void VirtualGic::Initialize()
    {
        if (currentCoreCtx->isBootCore) {
            m_virqPendingQueue.Initialize(m_virqStates.data());
            m_numListRegisters = static_cast<u8>(1 + (gich->vtr & 0x3F));

            // All fields are reset to 0 on reset and deep sleep exit

            for (VirqState &state: m_virqStates) {
                state.listPrev = state.listNext = virqListInvalidIndex;
                state.priority = lowestPriority;
            }

            // SPIs (+ reserved interrupts just in case)
            for (u32 i = 32; i < 1024; i++) {
                GetVirqState(0, i).irqId = i;
            }

            // SGIs, PPIs
            for (u32 coreId = 0; coreId < MAX_CORE; coreId++) {
                for (u32 i = 0; i < 32; i++) {
                    VirqState &state = GetVirqState(coreId, i);
                    state.coreId = coreId;
                    state.irqId = i;
                    if (i < 16) {
                        state.edgeTriggered = true;
                        state.enabled = true;
                    } else {
                        state.edgeTriggered = IrqManager::IsInterruptEdgeTriggered(i);
                    }
                }
            }

            // All guest interrupts are initially configured as disabled
            // All guest SPIs are initially configured as level-sensitive with no targets
        }

        auto &mgr = IrqManager::GetInstance();

        mgr.Register(*this, GIC_IRQID_MAINTENANCE, true);
        mgr.Register(*this, IrqManager::VgicUpdateSgi, false);

        // Clear the list registers (they reset to 0, though)
        for (u8 i = 0; i < m_numListRegisters; i++) {
            gich->lr[i].raw = 0;
        }

        // Enable a few maintenance interrupts. Enable the virtual interface.
        GicV2VirtualInterfaceController::HypervisorControlRegister hcr = {};
        hcr.vgrp1eie = true, 
        hcr.vgrp0eie = true,
        hcr.lrenpie = true,
        hcr.en = true,
        gich->hcr.raw = hcr.raw;
    }
}
