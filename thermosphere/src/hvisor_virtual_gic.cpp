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

#include "hvisor_virtual_gic.hpp"

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

    VirtualGic::VirqQueue::iterator VirtualGic::VirqQueue::erase(VirtualGic::VirqQueue::iterator startPos, VirtualGic::VirqQueue::iterator endPos)
    {
        VirqState &prev = m_storage[startPos->listPrev];
        VirqState &next = *endPos;
        u32 nextPos = GetStateIndex(*endPos);

        if (startPos->listPrev != virqListEndIndex) {
            prev.listNext = nextPos;
        } else {
            m_first = &next;
        }

        if (nextPos != virqListEndIndex) {
            next.listPrev = startPos->listPrev;
        } else {
            m_last = &prev;
        }

        for (auto it = startPos; it != endPos; ++it) {
            it->listPrev = it->listNext = virqListInvalidIndex;
        }
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
        bool newLvl = ((config & 2) << GicV2Distributor::GetCfgrShift(id)) == 0;

        if (state.levelSensitive != newLvl) {
            state.levelSensitive = newLvl;
            IrqManager::SetInterruptMode(id, newLvl);
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
}
