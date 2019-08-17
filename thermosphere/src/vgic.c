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

#include "vgic.h"
#include "irq.h"
#include "utils.h"
#include "core_ctx.h"

#include "debug_log.h"

#define MAX_NUM_INTERRUPTS  (512 - 32 + 32 * 4)

#define GICDOFF(field)      (offsetof(ArmGicV2Distributor, field))

typedef struct VirqState {
    u32 listPrev        : 10;
    u32 listNext        : 10;
    u8  priority        : 5;
    bool pending        : 1;
    bool active         : 1;
    bool handled        : 1;
    bool pendingLatch   : 1;
    u32 coreId          : 3; // up to 8 cores, but not implemented yet
} VirqState;

typedef struct VirqStateList {
    VirqState *first, *last;
    size_t size;
} VirqStateList;

// Note: we reset the GIC from wakeup-from-sleep, and expect the guest OS to save/restore state if needed
static VirqState TEMPORARY g_virqStates[MAX_NUM_INTERRUPTS] = { 0 };
static VirqStateList TEMPORARY g_virqPendingQueue = { NULL };
static u8 TEMPORARY g_virqSgiPendingSources[4][32] = { { 0 } };

static bool TEMPORARY g_virqIsDistributorEnabled = false;

static inline VirqState *vgicGetVirqState(u32 coreId, u16 id)
{
    if (id >= 32) {
        return &g_virqStates[id];
    } else {
        return &g_virqStates[512 - 32 + 32 * coreId + id];
    }
}

static inline VirqState *vgicGetNextQueuedVirqState(VirqState *cur)
{
    return &g_virqStates[cur->listNext];
}

static inline VirqState *vgicGetPrevQueuedVirqState(VirqState *cur)
{
    return &g_virqStates[cur->listPrev];
}

static inline VirqState *vgicGetQueueEnd(void)
{
    return &g_virqStates[MAX_NUM_INTERRUPTS];
}

static inline u32 vgicGetVirqStateIndex(VirqState *cur)
{
    return (u32)(cur - &g_virqStates[0]);
}

static inline u16 vgicGetVirqStateInterruptId(VirqState *cur)
{
    u32 idx = vgicGetVirqStateIndex(cur);
    /*if (idx == MAX_NUM_INTERRUPTS) {
        return GIC_IRQID_SPURIOUS;
    } else*/ if (idx >= 512 - 32) {
        return (idx - 512 + 32) % 32;
    } else {
        return idx;
    }
}

static inline u32 vgicGetVirqStateCoreId(VirqState *cur)
{
    u32 idx = vgicGetVirqStateIndex(cur);
    if (vgicGetVirqStateInterruptId(cur) < 32) {
        return (idx - 512 + 32) / 32;
    } else {
        return cur->coreId;
    }
}

static inline u32 vgicGetSgiCurrentSourceCoreId(VirqState *cur)
{
    return cur->coreId;
}


// Note: ordered by priority
static void vgicEnqueueVirqState(VirqStateList *list, VirqState *elem)
{
    VirqState *pos;

    ++list->size;
    // Empty list
    if (list->first == vgicGetQueueEnd()) {
        list->first = list->last = elem;
        return;
    }

    for (pos = list->first; pos != vgicGetQueueEnd(); pos = vgicGetNextQueuedVirqState(pos)) {
        // Lowest priority number is higher
        if (elem->priority <= pos->priority) {
            break;
        }
    }

    if (pos == vgicGetQueueEnd()) {
        // Insert after last
        pos = list->last;
        elem->listPrev = vgicGetVirqStateIndex(pos);
        elem->listNext = pos->listNext;
        pos->listNext = vgicGetVirqStateIndex(elem);
        list->last = elem;
    } else {
        // Otherwise, insert before
        u32 idx = vgicGetVirqStateIndex(elem);
        u32 posidx = vgicGetVirqStateIndex(pos);
        u32 previdx = pos->listPrev;

        elem->listNext = posidx;
        elem->listPrev = previdx;

        pos->listPrev = idx;

        if (pos == list->first) {
            list->first = elem;
        } else {
            VirqState *prev = vgicGetPrevQueuedVirqState(pos);
            prev->listNext = idx;
        }

    }
}

static void vgicDequeueVirqState(VirqStateList *list, VirqState *elem)
{
    VirqState *prev = vgicGetPrevQueuedVirqState(elem);
    VirqState *next = vgicGetNextQueuedVirqState(elem);

    --list->size;
    if (prev != vgicGetQueueEnd()) {
        prev->listNext = elem->listNext;
    } else {
        list->first = next;
    }

    if (next != vgicGetQueueEnd()) {
        next->listPrev = elem->listPrev;
    } else {
        list->last = prev;
    }
}

static inline void vgicNotifyOtherCoreList(u32 coreList)
{
    coreList &= ~BIT(currentCoreCtx->coreId);
    generateSgiForList(ThermosphereSgi_VgicUpdate, coreList);
}

static inline bool vgicIsVirqEdgeTriggered(u16 id)
{
    // Note: banked per CPU for SGIs and PPIs
    // SGIs are *always* edge-triggered, and we decide to keep all PPIs level-sensitive at all times.

    if (id < 16) {
        return true;
    } else if (id < 32) {
        return false;
    } else {
        return (g_irqManager.gic.gicd->icfgr[id / 16] & (2 << (id % 16))) != 0;
    }
}

static inline bool vgicIsVirqEnabled(u16 id)
{
    return (g_irqManager.gic.gicd->isenabler[id / 32] & BIT(id % 32)) != 0;
}

static inline bool vgicIsVirqPending(VirqState *state)
{
    // In case we emulate ispendr in the future...
    // Note: this function is not 100% reliable. The interrupt might be active-not-pending or inactive
    // but it shouldn't matter since where we use it, it would only cause one extraneous SGI.
    return state->pendingLatch || (!vgicIsVirqEdgeTriggered(vgicGetVirqStateInterruptId(state)) && state->pending);
}

static inline void vgicSetVirqPendingField(VirqState *state, bool val)
{
    if (!vgicIsVirqEdgeTriggered(vgicGetVirqStateInterruptId(state))) {
        state->pending = val;
    } else {
        state->pendingLatch = val;
    }
}

//////////////////////////////////////////////////

static void vgicSetDistributorControlRegister(u32 value)
{
    // We implement a virtual distributor/interface w/o security extensions.
    // Moreover, we forward all interrupts as Group 0 so that non-secure code that assumes GICv2
    // *with* security extensions (and thus all interrupts fw as group 1 there) still works (bit are in the same positions).

    // We don't implement Group 1 interrupts, either (so that's similar to GICv1).
    bool old = g_virqIsDistributorEnabled;
    g_virqIsDistributorEnabled = (value & 1) != 0;

    // Enable bit is actually just a global enable bit for all irq forwarding, other functions of the GICD aren't affected by it
    if (old != g_virqIsDistributorEnabled) {
        generateSgiForAllOthers(ThermosphereSgi_VgicUpdate);
    }
}

static inline u32 vgicGetDistributorControlRegister(void)
{
    return g_virqIsDistributorEnabled ? 1 : 0;
}

static inline u32 vgicGetDistributorTypeRegister(void)
{
    // See above comment.
    // Therefore, LSPI = 0, SecurityExtn = 0, rest = from physical distributor
    return g_irqManager.gic.gicd->typer & 0x7F;
}

static inline u32 vgicGetDistributorImplementerIdentificationRegister(void)
{
    u32 iidr =  ((u32)'A' << 24); // Product Id: Atmosphère (?)
    iidr |= 1 << 16;    // Major revision 1
    iidr |= 0 << 12;    // Minor revision 0
    iidr |= 0x43B;      // Implementer: Arm (value copied from physical GICD)
    return iidr;
}

static void vgicSetInterruptEnabledState(u16 id)
{
    if (id < 16 || !vgicIsVirqEnabled(id)) {
        // Nothing to do...
        // Also, ignore for SGIs
        return;
    }

    // Similar effects to setting the target list to non-0 when it was 0...
    VirqState *state = vgicGetVirqState(currentCoreCtx->coreId, id);
    if (vgicIsVirqPending(state)) {
        vgicNotifyOtherCoreList(g_irqManager.gic.gicd->itargetsr[id]);
    }
}

static void vgicClearInterruptEnabledState(u16 id)
{
    if (id < 16 || !vgicIsVirqEnabled(id)) {
        // Nothing to do...
        // Also, ignore for SGIs
        return;
    }

    // Similar effects to setting the target list to 0, we may need to notify the core
    // handling the interrupt if it's pending
    VirqState *state = vgicGetVirqState(currentCoreCtx->coreId, id);
    if (state->handled) {
        vgicNotifyOtherCoreList(BIT(vgicGetVirqStateCoreId(state)));
    }

    g_irqManager.gic.gicd->isenabler[id / 32] &= ~BIT(id % 32);
}

static inline bool vgicGetInterruptEnabledState(u16 id)
{
    // SGIs are always enabled
    return id < 16 || vgicIsVirqEnabled(id);
}

static void vgicSetInterruptPriorityByte(u16 id, u8 priority)
{
    // 32 priority levels max, bits [7:3]
    priority >>= 3;
    priority &= 0x1F;

    VirqState *state = vgicGetVirqState(currentCoreCtx->coreId, id);
    if (priority == state->priority) {
        // Nothing to do...
        return;
    }
    state->priority = priority;
    u32 targets = g_irqManager.gic.gicd->itargetsr[id];
    if (targets != 0 && vgicIsVirqPending(state)) {
        vgicNotifyOtherCoreList(targets);
    }
}

static inline u8 vgicGetInterruptPriorityByte(u16 id)
{
    return vgicGetVirqState(currentCoreCtx->coreId, id)->priority << 3;
}

static void vgicSetInterruptTargets(u16 id, u8 coreList)
{
    // Ignored for SGIs and PPIs
    if (id < 32) {
        return;
    }

    // Interrupt not pending (inactive or active-only): nothing much to do (see reference manual)
    // Otherwise, we may need to migrate the interrupt.
    // In our model, while a physical interrupt can be pending on multiple cores, we decide that a pending SPI
    // can only be handled on a single core (either it's in a LR, or in the global list), therefore we need to
    // send a signal to (oldList XOR newList) to either put the interrupt back in the global list or potentially handle it

    // Note that we take into account that the interrupt may be disabled.
    VirqState *state = vgicGetVirqState(currentCoreCtx->coreId, id);
    if (vgicIsVirqPending(state)) {
        u8 oldList = g_irqManager.gic.gicd->itargetsr[id];
        u8 diffList = (oldList ^ coreList) & getActiveCoreMask();
        if (diffList != 0) {
            vgicNotifyOtherCoreList(diffList);
        }
    }
    g_irqManager.gic.gicd->itargetsr[id] = coreList;
}

static inline u8 vgicGetInterruptTargets(u16 id)
{
    // For SGIs & PPIs, itargetsr is banked and contains the CPU ID
    return g_irqManager.gic.gicd->itargetsr[id];
}

static inline void vgicSetInterruptConfigByte(u16 id, u32 config)
{
    // Ignored for SGIs, implementation defined for PPIs
    if (id < 32) {
        return;
    }

    u32 cfg = g_irqManager.gic.gicd->icfgr[id / 16];
    cfg &= ~(3 << (id % 16));
    cfg |= (config & 2) << (id % 16);
    g_irqManager.gic.gicd->icfgr[id / 16] = cfg;
}

static inline u32 vgicGetInterruptConfigByte(u16 id, u32 config)
{
    return vgicIsVirqEdgeTriggered(id) ? 2 : 0;
}

static void vgicSetSgiPendingState(u16 id, u32 coreId, u32 srcCoreId)
{
    u8 old = g_virqSgiPendingSources[coreId][id];
    g_virqSgiPendingSources[coreId][id] = old | srcCoreId;
    if (old == 0) {
        // SGI is now pending & possibly needs to be serviced
        VirqState *state = vgicGetVirqState(coreId, id);
        state->pendingLatch = true;
        state->coreId = srcCoreId;
        vgicEnqueueVirqState(&g_virqPendingQueue, state);
        vgicNotifyOtherCoreList(BIT(coreId));
    }
}

static void vgicClearSgiPendingState(u16 id, u32 srcCoreId)
{
    // Only for the current core, therefore no need to signal physical SGI, etc., etc.
    u32 coreId = currentCoreCtx->coreId;
    u8 old = g_virqSgiPendingSources[coreId][id];
    u8 new_ =  old & ~(u8)srcCoreId;
    g_virqSgiPendingSources[coreId][id] = new_;
    if (old != 0 && new_ == 0) {
        VirqState *state = vgicGetVirqState(coreId, id);
        state->pendingLatch = false;
        vgicNotifyOtherCoreList(BIT(coreId));
    }
}

static inline u32 vgicGetSgiPendingState(u16 id)
{
    return g_virqSgiPendingSources[currentCoreCtx->coreId][id];
}

static void vgicSendSgi(u16 id, u32 filter, u32 coreList)
{
    switch (filter) {
        case 0:
            // Forward to coreList
            break;
        case 1:
            // Forward to all but current core
            coreList = getActiveCoreMask() & ~BIT(currentCoreCtx->coreId);
            break;
        case 2:
            // Forward to current core only
            coreList = BIT(currentCoreCtx->coreId);
            break;
        default:
            DEBUG("Emulated GCID_SGIR: invalid TargetListFilter value!\n");
            return;
    }

    FOREACH_BIT(tmp, dstCore, coreList) {
        vgicSetSgiPendingState(id, dstCore, currentCoreCtx->coreId);
    }
}

static inline u32 vgicGetPeripheralId2Register(void)
{
    // 2 for Gicv2
    return 2 << 4;
}

static void handleVgicMmioWrite(ExceptionStackFrame *frame, DataAbortIss dabtIss, size_t offset)
{
    size_t sz = BITL(dabtIss.sas);
    u32 val = (u32)frame->x[dabtIss.srt];
    uintptr_t addr = (uintptr_t)g_irqManager.gic.gicd + offset;

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
        case GICDOFF(igroupr) ... GICDOFF(igroupr) + 511/32:
            // Write ignored because we don't implement Group 1 here
            break;
        case GICDOFF(ispendr) ... GICDOFF(ispendr) + 511/32:
        case GICDOFF(icpendr) ... GICDOFF(icpendr) + 511/32:
        case GICDOFF(isactiver) ... GICDOFF(isactiver) + 511/32:
        case GICDOFF(icactiver) ... GICDOFF(icactiver) + 511/32:
            // Write ignored, not implemented (at least not yet, TODO)
            break;

        case GICDOFF(ctlr):
            vgicSetDistributorControlRegister(val);
            break;

        case GICDOFF(isenabler) ... GICDOFF(isenabler) + 511/32: {
            u32 base = 32 * (offset - GICDOFF(isenabler));
            FOREACH_BIT(tmp, pos, val) {
                vgicSetInterruptEnabledState((u16)(base + pos));
            }
            break;
        }
        case GICDOFF(icenabler) ... GICDOFF(icenabler) + 511/32: {
            u32 base = 32 * (offset - GICDOFF(icenabler));
            FOREACH_BIT(tmp, pos, val) {
                vgicClearInterruptEnabledState((u16)(base + pos));
            }
            break;
        }

        case GICDOFF(ipriorityr) ... GICDOFF(ipriorityr) + 511: {
            u16 base = (u16)(offset - GICDOFF(ipriorityr));
            for (u16 i = 0; i < sz; i++) {
                vgicSetInterruptPriorityByte(base + i, (u8)val);
                val >>= 8;
            }
            break;
        }

        case GICDOFF(itargetsr) + 32 ... GICDOFF(itargetsr) + 511: {
            u16 base = (u16)(offset - GICDOFF(itargetsr));
            for (u16 i = 0; i < sz; i++) {
                vgicSetInterruptTargets(base + i, (u8)val);
                val >>= 8;
            }
            break;
        }

        case GICDOFF(sgir):
            vgicSendSgi((u16)(val & 0xF), (val >> 24) & 3, (val >> 16) & 0xFF);
            break;

        case GICDOFF(cpendsgir) ... GICDOFF(cpendsgir) + 15: {
            u16 base = (u16)(offset - GICDOFF(cpendsgir));
            for (u16 i = 0; i < sz; i++) {
                FOREACH_BIT(tmp, pos, val & 0xFF) {
                    vgicClearSgiPendingState(base + i, pos);
                }
                val >>= 8;
            }
            break;
        }
        case GICDOFF(spendsgir) ... GICDOFF(spendsgir) + 15: {
            u16 base = (u16)(offset - GICDOFF(spendsgir));
            for (u16 i = 0; i < sz; i++) {
                FOREACH_BIT(tmp, pos, val & 0xFF) {
                    vgicSetSgiPendingState(base + i, currentCoreCtx->coreId, pos);
                }
                val >>= 8;
            }
            break;
        }

        default:
            dumpUnhandledDataAbort(dabtIss, addr, "GICD reserved/implementation-defined register");
            break;
    }
}

static void handleVgicMmioRead(ExceptionStackFrame *frame, DataAbortIss dabtIss, size_t offset)
{
    size_t sz = BITL(dabtIss.sas);
    uintptr_t addr = (uintptr_t)g_irqManager.gic.gicd + offset;

    u32 val = 0;

    switch (offset) {
        case GICDOFF(icfgr) ... GICDOFF(icfgr) + 31/4:
            // RAZ because of an implementation-defined choice
            break;
        case GICDOFF(igroupr) ... GICDOFF(igroupr) + 511/32:
            // RAZ because we don't implement Group 1 here
            break;
        case GICDOFF(ispendr) ... GICDOFF(ispendr) + 511/32:
        case GICDOFF(icpendr) ... GICDOFF(icpendr) + 511/32:
        case GICDOFF(isactiver) ... GICDOFF(isactiver) + 511/32:
        case GICDOFF(icactiver) ... GICDOFF(icactiver) + 511/32:
            // RAZ, not implemented (at least not yet, TODO)
            break;

        case GICDOFF(ctlr):
            val = vgicGetDistributorControlRegister();
            break;
        case GICDOFF(typer):
            val = vgicGetDistributorTypeRegister();
            break;
        case GICDOFF(iidr):
            val = vgicGetDistributorImplementerIdentificationRegister();
            break;

        case GICDOFF(isenabler) ... GICDOFF(isenabler) + 511/32:
        case GICDOFF(icenabler) ... GICDOFF(icenabler) + 511/32: {
            u16 base = (u16)(32 * (offset & 0x7F));
            for (u16 i = 0; i < 32; i++) {
                val |= vgicGetInterruptEnabledState(base + i) ? BIT(i) : 0;
            }
            break;
        }

        case GICDOFF(ipriorityr) ... GICDOFF(ipriorityr) + 511: {
            u16 base = (u16)(offset - GICDOFF(ipriorityr));
            for (u16 i = 0; i < sz; i++) {
                val |= vgicGetInterruptPriorityByte(base + i) << (8 * i);
            }
            break;
        }

        case GICDOFF(itargetsr) ... GICDOFF(itargetsr) + 511: {
            u16 base = (u16)(offset - GICDOFF(itargetsr));
            for (u16 i = 0; i < sz; i++) {
                val |= (u32)vgicGetInterruptTargets(base + i) << (8 * i);
            }
            break;
        }

        case GICDOFF(sgir):
            // Write-only register
            dumpUnhandledDataAbort(dabtIss, addr, "GICD read to write-only register GCID_SGIR");
            break;

        case GICDOFF(cpendsgir) ... GICDOFF(cpendsgir) + 15:
        case GICDOFF(spendsgir) ... GICDOFF(spendsgir) + 15: {
            u16 base = (u16)(offset & 0xF);
            for (u16 i = 0; i < sz; i++) {
                val |= (u32)vgicGetSgiPendingState(base + i) << (8 * i);
            }
            break;
        }

        case GICDOFF(icpidr2):
            val = vgicGetPeripheralId2Register();
            break;

        default:
            dumpUnhandledDataAbort(dabtIss, addr, "GICD reserved/implementation-defined register");
            break;
    }

    frame->x[dabtIss.srt] = val;
}

static void vgicCleanupPendingList(void)
{
    VirqState *node, *next;
    u16 id;
    bool pending;
    u32 coreId;

    for (node = g_virqPendingQueue.first; node != vgicGetQueueEnd(); node = next) {
        next = vgicGetNextQueuedVirqState(node);

        // For SGIs, check the pending bits
        id = vgicGetVirqStateInterruptId(node);
        coreId = vgicGetVirqStateCoreId(node);
        if (id < 16) {
            pending = g_virqSgiPendingSources[coreId][id] != 0;
        } else if (!vgicIsVirqEdgeTriggered(id)) {
            // For hardware interrupts, we have kept the interrupt active on the physical GICD
            // For level-sensitive interrupts, we need to check if they're also still physically pending (resampling).
            // If not, there's nothing to service anymore, and therefore we have to deactivate them, so that
            // we're notified when they become pending again.

            // Note: we can't touch PPIs for other cores... but each core will call this function anyway.
            if (id >= 32 || coreId == currentCoreCtx->coreId) {
                u8 mask = g_irqManager.gic.gicd->ispendr[id / 32] & BIT(id % 32);
                if (mask == 0) {
                    g_irqManager.gic.gicd->icactiver[id / 32] = mask;
                    pending = false;
                } else {
                    pending = true;
                }
            } else {
                // Oops, can't handle PPIs of other cores
                // Assume interrupt is still pending and call it a day
                pending = true;
            }
        } else {
            pending = node->pendingLatch;
        }

        if (!pending) {
            vgicSetVirqPendingField(node, false);
            vgicDequeueVirqState(&g_virqPendingQueue, node);
        }
    }
}

static bool vgicTestInterruptEligibility(VirqState *state)
{
    u16 id = vgicGetVirqStateInterruptId(state);
    u32 coreId = vgicGetVirqStateCoreId(state);

    // Precondition: state still in list

    if (id < 32 && coreId != currentCoreCtx->coreId) {
        // We can't handle SGIs/PPIs of other cores.
        return false;
    }

    return vgicIsVirqEnabled(id) && (g_irqManager.gic.gicd->itargetsr[id] & BIT(currentCoreCtx->coreId)) != 0;
}

// Returns highest priority
static u32 vgicChoosePendingInterrupts(size_t *outNumChosen, VirqState *chosen[], size_t maxNum)
{
    u32 highestPrio = 0x1F;
    *outNumChosen = 0;

    for (VirqState *node = g_virqPendingQueue.first, *next; node != vgicGetQueueEnd() && *outNumChosen < maxNum; node = next) {
        next = vgicGetNextQueuedVirqState(node);
        if (vgicTestInterruptEligibility(node)) {
            u16 irqId = vgicGetVirqStateInterruptId(node);
            highestPrio = highestPrio < node->priority ? highestPrio : node->priority;
            node->handled = true;
            if (irqId < 16) {
                node->coreId = __builtin_ctz(g_virqSgiPendingSources[vgicGetVirqStateCoreId(node)][irqId]);
            }
            vgicDequeueVirqState(&g_virqPendingQueue, node);
            chosen[(*outNumChosen)++] = node;
        }
    }

    return highestPrio;
}

static inline bool vgicIsInterruptRaisable(u32 prio)
{
    ArmGicV2VmControlRegister vmcr = g_irqManager.gic.gich->vmcr;
    if (prio >= vmcr.pmr) {
        return false;
    }

    u32 grpMask = ~MASK(vmcr.bpr + 1) & 0xFF;
    u32 rpr = g_irqManager.gic.gicv->rpr;
    return rpr >= GICV_IDLE_PRIORITY || ((prio << 3) & grpMask) < (g_irqManager.gic.gicv->rpr & grpMask);
}

static inline u64 vgicGetElrsrRegister(void)
{
    return (u64)g_irqManager.gic.gich->elsr0 | (((u64)g_irqManager.gic.gich->elsr1) << 32);
}

static inline bool vgicIsListRegisterAvailable(u32 id)
{
    return (id >= g_irqManager.numListRegisters) && (vgicGetElrsrRegister() & BITL(id));
}

static inline size_t vgicGetNumberOfFreeListRegisters(void)
{
    return __builtin_popcountll(vgicGetElrsrRegister());
}

static inline volatile ArmGicV2ListRegister *vgicGetFreeListRegister(void)
{
    u32 ff = __builtin_ffsll(vgicGetElrsrRegister());
    return ff == 0 ? NULL : &g_irqManager.gic.gich->lr[ff - 1];
}

static void vgicPushListRegisters(VirqState *chosen[], size_t num)
{
    for (size_t i = 0; i < num; num++) {
        VirqState *state = chosen[i];
        u16 irqId = vgicGetVirqStateInterruptId(state);

        ArmGicV2ListRegister lr = {0};
        lr.grp1 = false; // group0
        lr.priority = state->priority;
        lr.virtualId = irqId;

        // We only add new pending interrupts here...
        lr.pending = true;
        lr.active = false;

        // We don't support guests setting the pending latch, so the logic is probably simpler...

        if (irqId < 16) {
            // SGI
            // Unset one pennding source temporarily
            u32 sourceCoreId = vgicGetSgiCurrentSourceCoreId(state);
            if (g_virqSgiPendingSources[state->coreId][irqId] & ~BIT(sourceCoreId)) {
                // Multiple sources
                lr.physicalId = BIT(9) /* EOI notification bit */ | sourceCoreId;
            } else {
                lr.physicalId = sourceCoreId;
            }

            lr.hw = false; // software
        } else {
            // Actual physical interrupt
            lr.hw = true;
            lr.physicalId = irqId;
        }

        *vgicGetFreeListRegister() = lr;
    }
}

static bool vgicUpdateListRegister(volatile ArmGicV2ListRegister *lr)
{
    u16 irqId = lr->virtualId;
    ArmGicV2ListRegister zero = {0};

    // Update the state
    VirqState *state = vgicGetVirqState(currentCoreCtx->coreId, irqId);
    state->active = lr->active;

    if (lr->active) {
        // We don't touch active interrupts
        return false;
    } else if (lr->pending) {
        // New interrupts might have come, pending status might have been changed, etc.
        // We need to put the interrupt back in the pending list (which we clean up afterwards)
        vgicEnqueueVirqState(&g_virqPendingQueue, state);
        state->handled = false;
        *lr = zero;
        return true;
    } else {
        // Inactive interrupt, cleanup
        vgicSetVirqPendingField(state, 0);
        state->handled = false;
        *lr = zero;
        return false;
    }
}

void vgicUpdateState(void)
{
    volatile ArmGicV2VirtualInterfaceController *gich = g_irqManager.gic.gich;
    u64 usedMap = ~vgicGetElrsrRegister() & MASKL(g_irqManager.numListRegisters);

    // First, put back inactive interrupts into the queue
    FOREACH_BIT (tmp, pos, usedMap) {
        vgicUpdateListRegister(&gich->lr[pos]);
    }

    // Then, clean the list up
    vgicCleanupPendingList();

    size_t numChosen;
    u32 newHiPrio;
    size_t numFreeLr = vgicGetNumberOfFreeListRegisters();
    VirqState *chosen[numFreeLr]; // yes this is a VLA, potentially dangerous. Usually max 4 (64 at most)

    // Choose interrupts...
    newHiPrio = vgicChoosePendingInterrupts(&numChosen, chosen, numFreeLr);

    // ...and push them
    for (size_t i = 0; i < numChosen; i++) {
        vgicPushListRegisters(chosen, numChosen);
    }

    // Raise vIRQ when applicable. We only need to check for the highest priority
    if (vgicIsInterruptRaisable(newHiPrio)) {
        u32 hcr = GET_SYSREG(hcr_el2);
        SET_SYSREG(hcr_el2, hcr | HCR_VI);
    }

    // Enable underflow interrupt when appropriate to do so
    if (vgicGetNumberOfFreeListRegisters() != g_irqManager.numListRegisters) {
        gich->hcr.uie = true;
    } else {
        gich->hcr.uie = false;
    }
}

void vgicMaintenanceInterruptHandler(void)
{
    ArmGicV2MaintenanceIntStatRegister misr = g_irqManager.gic.gich->misr;

    // Force GICV_CTRL to behave like ns-GICC_CTLR, with group 1 being replaced by group 0
    if (misr.vgrp0e || misr.vgrp0d || misr.vgrp1e || misr.vgrp1d) {
        g_irqManager.gic.gicv->ctlr &= BIT(9) | BIT(0);
    }

    if (misr.lrenp) {
        DEBUG("VGIC: List Register Entry Not Present maintenance interrupt!");
        panic();
    }

    // The rest should be handled by the main loop...
}
void handleVgicdMmio(ExceptionStackFrame *frame, DataAbortIss dabtIss, size_t offset)
{
    size_t sz = BITL(dabtIss.sas);
    uintptr_t addr = (uintptr_t)g_irqManager.gic.gicd + offset;
    bool oops = true;

    // ipriorityr, itargetsr, *pendsgir are byte-accessible
    if (
        !(offset >= GICDOFF(ipriorityr) && offset < GICDOFF(ipriorityr) + 512) &&
        !(offset >= GICDOFF(itargetsr)  && offset < GICDOFF(itargetsr) + 512) &&
        !(offset >= GICDOFF(cpendsgir)  && offset < GICDOFF(cpendsgir) + 16) &&
        !(offset >= GICDOFF(spendsgir)  && offset < GICDOFF(spendsgir) + 16)
    ) {
        if ((offset & 3) != 0 || sz != 4) {
            dumpUnhandledDataAbort(dabtIss, addr, "GICD non-word aligned MMIO");
        } else {
            oops = false;
        }
    } else {
        if (sz != 1 && sz != 4) {
            dumpUnhandledDataAbort(dabtIss, addr, "GICD 16 or 64-bit access");
        } else if (sz == 4 && (offset & 3) != 0) {
            dumpUnhandledDataAbort(dabtIss, addr, "GICD misaligned MMIO");
        } else {
            oops = false;
        }
    }

    recursiveSpinlockLock(&g_irqManager.lock);

    if (dabtIss.wnr && !oops) {
        handleVgicMmioWrite(frame, dabtIss, offset);
    } else if (!oops) {
        handleVgicMmioRead(frame, dabtIss, offset);
    }

    // TODO gic main loop
    recursiveSpinlockUnlock(&g_irqManager.lock);
}

// lock needs to be held by caller
// note, irqId >= 16
void vgicEnqueuePhysicalIrq(u16 irqId)
{
    VirqState *state = vgicGetVirqState(currentCoreCtx->coreId, irqId);
    vgicSetVirqPendingField(state, true);
    vgicEnqueueVirqState(&g_virqPendingQueue, state);
}

void vgicInit(void)
{
    if (currentCoreCtx->isBootCore) {
        g_virqPendingQueue.first = g_virqPendingQueue.last = vgicGetQueueEnd();

        for (u32 i = 0; i < 512 - 32 + 32 * 4; i++) {
            g_virqStates[i].listNext = g_virqStates[i].listPrev = MAX_NUM_INTERRUPTS;
        }
    }

    g_irqManager.gic.gich->hcr.en = true;
}
