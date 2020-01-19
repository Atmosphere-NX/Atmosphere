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

// Referring to array bounds:
#define SPI_END_INDEX           (GIC_IRQID_MAX + 1 - 32)
#define MAX_NUM_INTERRUPTS      (SPI_END_INDEX + 32 * 4)

#define VIRQLIST_END_ID         MAX_NUM_INTERRUPTS
#define VIRQLIST_INVALID_ID     (VIRQLIST_END_ID + 1)

#define GICDOFF(field)          (offsetof(ArmGicV2Distributor, field))

typedef struct VirqState {
    u32 listPrev        : 11;
    u32 listNext        : 11;
    u32 priority        : 5;
    bool pending        : 1;
    bool active         : 1;
    bool handled        : 1;
    bool pendingLatch   : 1;
    bool levelSensitive : 1;
    u32 coreId          : 3; // up to 8 cores, but not implemented yet
    u32 targetList      : 8;
    bool enabled        : 1;
    u64                 : 0;
} VirqState;

typedef struct VirqStateList {
    VirqState *first, *last;
    size_t size;
} VirqStateList;

// Note: we reset the GIC from wakeup-from-sleep, and expect the guest OS to save/restore state if needed
static VirqState TEMPORARY g_virqStates[MAX_NUM_INTERRUPTS] = { 0 };
static VirqStateList TEMPORARY g_virqPendingQueue = { NULL };
static u8 TEMPORARY g_vgicIncomingSgiPendingSources[4][32] = { { 0 } };
static u64 TEMPORARY g_vgicUsedLrMap[4] = { 0 };

static bool TEMPORARY g_vgicIsDistributorEnabled = false;

static inline VirqState *vgicGetVirqState(u32 coreId, u16 id)
{
    if (id >= 32) {
        return &g_virqStates[id - 32];
    } else if (id <= GIC_IRQID_MAX) {
        return &g_virqStates[SPI_END_INDEX + 32 * coreId + id];
    } else {
        // Shouldn't happen -- 1020 is a multiple of 4 and the switch case should catch it
        return NULL;
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
    // Note: not a valid pointer -- one past the end
    return &g_virqStates[VIRQLIST_END_ID];
}

static inline bool vgicIsStateQueued(VirqState *state)
{
    return state->listPrev != VIRQLIST_INVALID_ID && state->listNext != VIRQLIST_INVALID_ID;
}

static inline u32 vgicGetVirqStateIndex(VirqState *cur)
{
    return (u32)(cur - &g_virqStates[0]);
}

static inline u16 vgicGetVirqStateInterruptId(VirqState *cur)
{
    u32 idx = vgicGetVirqStateIndex(cur);
    if (idx >= SPI_END_INDEX) {
        return (idx - SPI_END_INDEX) % 32;
    } else {
        return 32 + idx;
    }
}

static inline u32 vgicGetVirqStateCoreId(VirqState *cur)
{
    u32 idx = vgicGetVirqStateIndex(cur);
    if (vgicGetVirqStateInterruptId(cur) < 32) {
        return (idx - SPI_END_INDEX) / 32;
    } else {
        return cur->coreId;
    }
}

static inline u32 vgicGetSgiCurrentSourceCoreId(VirqState *cur)
{
    return cur->coreId;
}

// export symbol to avoid build warnings
void vgicDebugPrintList(VirqStateList *list)
{
    (void)list;
    DEBUG("[");
    for (VirqState *pos = list->first; pos != vgicGetQueueEnd(); pos = vgicGetNextQueuedVirqState(pos)) {
        DEBUG("%u,", vgicGetVirqStateIndex(pos));
    }
    DEBUG("]\n");
}

// export symbol to avoid build warnings
void vgicDebugPrintLrList(void)
{
    DEBUG("core %u lr [", currentCoreCtx->coreId);
    for (u32 i = 0; i < g_irqManager.numListRegisters; i++) {
        if (g_vgicUsedLrMap[currentCoreCtx->coreId] & BITL(i)) {
            DEBUG("%u,", gich->lr[i].virtualId);
        } else {
            DEBUG("-,");
        }
    }
    DEBUG("]\n");
}

static inline void vgicInsertVirqStateBefore(VirqStateList *list, VirqState *pos, VirqState *elem)
{
    ++list->size;
    // Empty list
    if (list->first == vgicGetQueueEnd()) {
        list->first = list->last = elem;
        elem->listPrev = elem->listNext = VIRQLIST_END_ID;
        return;
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
        VirqState *prev = vgicGetPrevQueuedVirqState(pos);

        elem->listNext = posidx;
        elem->listPrev = previdx;

        pos->listPrev = idx;

        if (pos == list->first) {
            list->first = elem;
        } else {
            prev->listNext = idx;
        }
    }
}

// Currently unused
static inline void vgicInsertVirqStateAfter(VirqStateList *list, VirqState *pos, VirqState *elem)
{
    ++list->size;
    // Empty list
    if (list->first == vgicGetQueueEnd()) {
        list->first = list->last = elem;
        elem->listPrev = elem->listNext = VIRQLIST_END_ID;
        return;
    }

    if (pos == vgicGetQueueEnd()) {
        // Insert before first
        pos = list->first;

        elem->listPrev = pos->listPrev;
        elem->listNext = vgicGetVirqStateIndex(pos);
        pos->listPrev = vgicGetVirqStateIndex(elem);
        list->first = elem;
    } else {
        // Otherwise, insert after
        u32 idx = vgicGetVirqStateIndex(elem);
        u32 posidx = vgicGetVirqStateIndex(pos);

        u32 nextidx = pos->listPrev;
        VirqState *next = vgicGetNextQueuedVirqState(pos);

        elem->listPrev = posidx;
        elem->listNext = nextidx;

        pos->listNext = idx;

        if (pos == list->last) {
            list->last = elem;
        } else {
            next->listPrev = idx;
        }
    }
}

static inline int vgicCompareVirqState(const VirqState *a, const VirqState *b)
{
    // Lower priority number is higher; we sort by descending priority, ie. ascending priority number
    // Put the interrupts that were previously in the LR before the one which don't
    int n1 = (int)(a->priority - b->priority);
    return n1 == 0 ? (int)b->handled - (int)a->handled : n1;
}

// Note: ordered by priority
static void vgicEnqueueVirqState(VirqStateList *list, VirqState *elem)
{
    VirqState *pos;

    if (vgicIsStateQueued(elem)) {
        PANIC("vgicEnqueueVirqState: unsanitized argument idx=%u previd=%u nextid=%u\n", (u32)vgicGetVirqStateIndex(elem), elem->listPrev, elem->listNext);
    }

    for (pos = list->first; pos != vgicGetQueueEnd(); pos = vgicGetNextQueuedVirqState(pos)) {
        // Sort predicate should be stable
        if (vgicCompareVirqState(elem, pos) < 0) {
            break;
        }
    }

    vgicInsertVirqStateBefore(list, pos, elem);
}

static void vgicDequeueVirqState(VirqStateList *list, VirqState *elem)
{
    VirqState *prev = vgicGetPrevQueuedVirqState(elem);
    VirqState *next = vgicGetNextQueuedVirqState(elem);

    if (!vgicIsStateQueued(elem)) {
        PANIC("vgicDequeueVirqState: invalid id %x\n", vgicGetVirqStateIndex(elem));
    }

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

    elem->listPrev = elem->listNext = VIRQLIST_INVALID_ID;
}

static inline void vgicNotifyOtherCoreList(u32 coreList)
{
    coreList &= ~BIT(currentCoreCtx->coreId);
    if (coreList != 0) {
        generateSgiForList(ThermosphereSgi_VgicUpdate, coreList);
    }
}

static inline bool vgicIsVirqPending(VirqState *state)
{
    // In case we emulate ispendr in the future...
    // Note: this function is not 100% reliable. The interrupt might be active-not-pending or inactive
    // but it shouldn't matter since where we use it, it would only cause one extraneous SGI.
    return state->pendingLatch || (state->levelSensitive && state->pending);
}

static inline void vgicSetVirqPendingState(VirqState *state, bool val)
{
    if (state->levelSensitive) {
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
    bool old = g_vgicIsDistributorEnabled;
    g_vgicIsDistributorEnabled = (value & 1) != 0;

    // Enable bit is actually just a global enable bit for all irq forwarding, other functions of the GICD aren't affected by it
    if (old != g_vgicIsDistributorEnabled) {
        generateSgiForAllOthers(ThermosphereSgi_VgicUpdate);
    }
}

static inline u32 vgicGetDistributorControlRegister(void)
{
    return g_vgicIsDistributorEnabled ? 1 : 0;
}

static inline u32 vgicGetDistributorTypeRegister(void)
{
    // See above comment.
    // Therefore, LSPI = 0, SecurityExtn = 0, rest = from physical distributor
    return gicd->typer & 0x7F;
}

static inline u32 vgicGetDistributorImplementerIdentificationRegister(void)
{
    u32 iidr =  ((u32)'A' << 24); // Product Id: Atmosphère (?)
    iidr |= 2 << 16;    // Major revision 2 (GICv2)
    iidr |= 0 << 12;    // Minor revision 0
    iidr |= 0x43B;      // Implementer: Arm (value copied from physical GICD)
    return iidr;
}

static void vgicSetInterruptEnabledState(u16 id)
{
    VirqState *state = vgicGetVirqState(currentCoreCtx->coreId, id);

    if (id < 16 || !irqIsGuest(id) || state->enabled) {
        // Nothing to do...
        // Also, ignore for SGIs
        return;
    }

    // Similar effects to setting the target list to non-0 when it was 0...
    if (vgicIsVirqPending(state)) {
        vgicNotifyOtherCoreList(state->targetList);
    }

    state->enabled = true;
    gicd->isenabler[id / 32] = BIT(id % 32);
}

static void vgicClearInterruptEnabledState(u16 id)
{
    VirqState *state = vgicGetVirqState(currentCoreCtx->coreId, id);

    if (id < 16 || !irqIsGuest(id) || !state->enabled) {
        // Nothing to do...
        // Also, ignore for SGIs
        return;
    }

    // Similar effects to setting the target list to 0, we may need to notify the core
    // handling the interrupt if it's pending
    if (state->handled) {
        vgicNotifyOtherCoreList(BIT(vgicGetVirqStateCoreId(state)));
    }

    state->enabled = false;
    gicd->icenabler[id / 32] = BIT(id % 32);
}

static inline bool vgicGetInterruptEnabledState(u16 id)
{
    // SGIs are always enabled
    return id < 16 || (irqIsGuest(id) && vgicGetVirqState(currentCoreCtx->coreId, id)->enabled);
}

static void vgicSetInterruptPriorityByte(u16 id, u8 priority)
{
    if (!irqIsGuest(id)) {
        return;
    }

    // 32 priority levels max, bits [7:3]
    priority >>= 3;
    priority &= 0x1F;

    if (id >= 16) {
        // Ensure we have the correct priority on the physical distributor...
        gicd->ipriorityr[id] = IRQ_PRIORITY_GUEST << g_irqManager.priorityShift;
    }

    VirqState *state = vgicGetVirqState(currentCoreCtx->coreId, id);
    if (priority == state->priority) {
        // Nothing to do...
        return;
    }

    state->priority = priority;
    u32 targets = state->targetList;
    if (targets != 0 && vgicIsVirqPending(state)) {
        vgicNotifyOtherCoreList(targets);
    }
}

static inline u8 vgicGetInterruptPriorityByte(u16 id)
{
    return irqIsGuest(id) ? vgicGetVirqState(currentCoreCtx->coreId, id)->priority << 3 : 0;
}

static void vgicSetInterruptTargets(u16 id, u8 coreList)
{
    // Ignored for SGIs and PPIs, and non-guest interrupts
    if (id < 32 || !irqIsGuest(id)) {
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
        u8 oldList = state->targetList;
        u8 diffList = (oldList ^ coreList) & getActiveCoreMask();
        if (diffList != 0) {
            vgicNotifyOtherCoreList(diffList);
        }
    }

    state->targetList = coreList;
    gicd->itargetsr[id] = state->targetList;
}

static inline u8 vgicGetInterruptTargets(u16 id)
{
    // For SGIs & PPIs, itargetsr is banked and contains the CPU ID
    return (id < 32 || irqIsGuest(id)) ? vgicGetVirqState(currentCoreCtx->coreId, id)->targetList : 0;
}

static inline void vgicSetInterruptConfigBits(u16 id, u32 config)
{
    // Ignored for SGIs, implementation defined for PPIs
    if (id < 32 || !irqIsGuest(id)) {
        return;
    }

    VirqState *state = vgicGetVirqState(currentCoreCtx->coreId, id);

    // Expose bit(2n) as nonprogrammable to the guest no matter what the physical distributor actually behaves
    bool newLvl = ((config & 2) << IRQ_CFGR_SHIFT(id)) == 0;

    if (state->levelSensitive != newLvl) {
        u32 cfg = gicd->icfgr[id / 16];
        cfg &= ~(3 << IRQ_CFGR_SHIFT(id));
        cfg |= (!newLvl ? 3 : 1) << IRQ_CFGR_SHIFT(id);
        gicd->icfgr[id / 16] = cfg;

        state->levelSensitive = newLvl;
    }
}

static inline u32 vgicGetInterruptConfigBits(u16 id)
{
    u32 oneNModel = id < 32 || !irqIsGuest(id) ? 0 : 1;
    return (irqIsGuest(id) && !vgicGetVirqState(currentCoreCtx->coreId, id)->levelSensitive) ? 2 | oneNModel : oneNModel;
}

static void vgicSetSgiPendingState(u16 id, u32 coreId, u32 srcCoreId)
{
    //DEBUG("EL2 [core %u]: sending vSGI %hu to core %u\n", srcCoreId, id, coreId);
    VirqState *state = vgicGetVirqState(coreId, id);
    g_vgicIncomingSgiPendingSources[coreId][id] |= BIT(srcCoreId);
    if (!state->handled && !vgicIsStateQueued(state)) {
        // The SGI was inactive on the target core...
        state->pendingLatch = true;
        state->coreId = srcCoreId;
        g_vgicIncomingSgiPendingSources[coreId][id] &= ~BIT(srcCoreId);
        vgicEnqueueVirqState(&g_virqPendingQueue, state);
        vgicNotifyOtherCoreList(BIT(coreId));
    }
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

    coreList &= getActiveCoreMask();

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
    u32 val = (u32)(readFrameRegisterZ(frame, dabtIss.srt) & MASKL(8 * sz));
    uintptr_t addr = (uintptr_t)gicd + offset;

    //DEBUG("gicd write off 0x%03llx sz %lx val %x w%d\n", offset, sz, val, (int)dabtIss.srt);

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
        case GICDOFF(igroupr) ... GICDOFF(igroupr) + GIC_IRQID_MAX/8:
            // Write ignored because we don't implement Group 1 here
            break;
        case GICDOFF(ispendr) ... GICDOFF(ispendr) + GIC_IRQID_MAX/8:
        case GICDOFF(icpendr) ... GICDOFF(icpendr) + GIC_IRQID_MAX/8:
        case GICDOFF(isactiver) ... GICDOFF(isactiver) + GIC_IRQID_MAX/8:
        case GICDOFF(icactiver) ... GICDOFF(icactiver) + GIC_IRQID_MAX/8:
        case GICDOFF(cpendsgir) ... GICDOFF(cpendsgir) + 15:
        case GICDOFF(spendsgir) ... GICDOFF(spendsgir) + 15:
            // Write ignored, not implemented (at least not yet, TODO)
            break;

        case GICDOFF(ctlr):
            vgicSetDistributorControlRegister(val);
            break;

        case GICDOFF(isenabler) ... GICDOFF(isenabler) + GIC_IRQID_MAX/8: {
            u32 base = 8 * (offset - GICDOFF(isenabler));
            FOREACH_BIT(tmp, pos, val) {
                vgicSetInterruptEnabledState((u16)(base + pos));
            }
            break;
        }
        case GICDOFF(icenabler) ... GICDOFF(icenabler) + GIC_IRQID_MAX/8: {
            u32 base = 8 * (offset - GICDOFF(icenabler));
            FOREACH_BIT(tmp, pos, val) {
                vgicClearInterruptEnabledState((u16)(base + pos));
            }
            break;
        }

        case GICDOFF(ipriorityr) ... GICDOFF(ipriorityr) + GIC_IRQID_MAX: {
            u16 base = (u16)(offset - GICDOFF(ipriorityr));
            for (u16 i = 0; i < sz; i++) {
                vgicSetInterruptPriorityByte(base + i, (u8)val);
                val >>= 8;
            }
            break;
        }

        case GICDOFF(itargetsr) + 32 ... GICDOFF(itargetsr) + GIC_IRQID_MAX: {
            u16 base = (u16)(offset - GICDOFF(itargetsr));
            for (u16 i = 0; i < sz; i++) {
                vgicSetInterruptTargets(base + i, (u8)val);
                val >>= 8;
            }
            break;
        }

        case GICDOFF(icfgr) + 32/4 ... GICDOFF(icfgr) + GIC_IRQID_MAX/4: {
            u16 base = (u16)(4 * (offset & 0xFF));
            for (u16 i = 0; i < 16; i++) {
                vgicSetInterruptConfigBits(base + i, val & 3);
                val >>= 2;
            }
            break;
        }

        case GICDOFF(sgir):
            vgicSendSgi((u16)(val & 0xF), (val >> 24) & 3, (val >> 16) & 0xFF);
            break;

        default:
            dumpUnhandledDataAbort(dabtIss, addr, "GICD reserved/implementation-defined register");
            break;
    }
}

static void handleVgicMmioRead(ExceptionStackFrame *frame, DataAbortIss dabtIss, size_t offset)
{
    size_t sz = BITL(dabtIss.sas);
    uintptr_t addr = (uintptr_t)gicd + offset;

    u32 val = 0;

    //DEBUG("gicd read off 0x%03llx sz %lx\n", offset, sz);

    switch (offset) {
        case GICDOFF(icfgr) ... GICDOFF(icfgr) + 31/4:
            // RAZ because of an implementation-defined choice
            break;
        case GICDOFF(igroupr) ... GICDOFF(igroupr) + GIC_IRQID_MAX/8:
            // RAZ because we don't implement Group 1 here
            break;
        case GICDOFF(ispendr) ... GICDOFF(ispendr) + GIC_IRQID_MAX/8:
        case GICDOFF(icpendr) ... GICDOFF(icpendr) + GIC_IRQID_MAX/8:
        case GICDOFF(isactiver) ... GICDOFF(isactiver) + GIC_IRQID_MAX/8:
        case GICDOFF(icactiver) ... GICDOFF(icactiver) + GIC_IRQID_MAX/8:
        case GICDOFF(cpendsgir) ... GICDOFF(cpendsgir) + 15:
        case GICDOFF(spendsgir) ... GICDOFF(spendsgir) + 15:
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

        case GICDOFF(isenabler) ... GICDOFF(isenabler) + GIC_IRQID_MAX/8:
        case GICDOFF(icenabler) ... GICDOFF(icenabler) + GIC_IRQID_MAX/8: {
            u16 base = (u16)(8 * (offset & 0x7F));
            for (u16 i = 0; i < 32; i++) {
                val |= vgicGetInterruptEnabledState(base + i) ? BIT(i) : 0;
            }
            break;
        }

        case GICDOFF(ipriorityr) ... GICDOFF(ipriorityr) + GIC_IRQID_MAX: {
            u16 base = (u16)(offset - GICDOFF(ipriorityr));
            for (u16 i = 0; i < sz; i++) {
                val |= vgicGetInterruptPriorityByte(base + i) << (8 * i);
            }
            break;
        }

        case GICDOFF(itargetsr) ... GICDOFF(itargetsr) + GIC_IRQID_MAX: {
            u16 base = (u16)(offset - GICDOFF(itargetsr));
            for (u16 i = 0; i < sz; i++) {
                val |= (u32)vgicGetInterruptTargets(base + i) << (8 * i);
            }
            break;
        }

        case GICDOFF(icfgr) + 32/4 ... GICDOFF(icfgr) + GIC_IRQID_MAX/4: {
            u16 base = (u16)(4 * (offset & 0xFF));
            for (u16 i = 0; i < 16; i++) {
                val |= vgicGetInterruptConfigBits(base + i) << (2 * i);
            }
            break;
        }

        case GICDOFF(sgir):
            // Write-only register
            dumpUnhandledDataAbort(dabtIss, addr, "GICD read to write-only register GCID_SGIR");
            break;

        case GICDOFF(icpidr2):
            val = vgicGetPeripheralId2Register();
            break;

        default:
            dumpUnhandledDataAbort(dabtIss, addr, "GICD reserved/implementation-defined register");
            break;
    }

    writeFrameRegisterZ(frame, dabtIss.srt, val);
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
            pending = true;
        } else if (node->levelSensitive) {
            // For hardware interrupts, we have kept the interrupt active on the physical GICD
            // For level-sensitive interrupts, we need to check if they're also still physically pending (resampling).
            // If not, there's nothing to service anymore, and therefore we have to deactivate them, so that
            // we're notified when they become pending again.

            // Note: we can't touch PPIs for other cores... but each core will call this function anyway.
            if (id >= 32 || coreId == currentCoreCtx->coreId) {
                u32 mask = gicd->ispendr[id / 32] & BIT(id % 32);
                if (mask == 0) {
                    gicd->icactiver[id / 32] = BIT(id % 32);
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
            vgicSetVirqPendingState(node, false);
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

    return vgicGetInterruptEnabledState(id) && (id < 32 || (state->targetList & BIT(currentCoreCtx->coreId)) != 0);
}

static void vgicChoosePendingInterrupts(size_t *outNumChosen, VirqState *chosen[], size_t maxNum)
{
    *outNumChosen = 0;

    for (VirqState *node = g_virqPendingQueue.first, *next; node != vgicGetQueueEnd() && *outNumChosen < maxNum; node = next) {
        next = vgicGetNextQueuedVirqState(node);
        if (vgicTestInterruptEligibility(node)) {
            node->handled = true;
            vgicDequeueVirqState(&g_virqPendingQueue, node);
            chosen[(*outNumChosen)++] = node;
        }
    }
}

static inline u64 vgicGetElrsrRegister(void)
{
    return (u64)gich->elsr0 | (((u64)gich->elsr1) << 32);
}

static inline bool vgicIsListRegisterAvailable(u32 id)
{
    return (id >= g_irqManager.numListRegisters) && (vgicGetElrsrRegister() & BITL(id));
}

static inline size_t vgicGetNumberOfFreeListRegisters(void)
{
    return __builtin_popcountll(vgicGetElrsrRegister());
}

static inline volatile ArmGicV2ListRegister *vgicAllocateListRegister(void)
{
    u32 ff = __builtin_ffsll(vgicGetElrsrRegister());
    if (ff == 0) {
        return NULL;
    } else {
        g_vgicUsedLrMap[currentCoreCtx->coreId] |= BITL(ff - 1);
        return &gich->lr[ff - 1];
    }
}

static void vgicPushListRegisters(VirqState *chosen[], size_t num)
{
    for (size_t i = 0; i < num; i++) {
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
            u32 sourceCoreId = vgicGetSgiCurrentSourceCoreId(state);
            lr.physicalId = BIT(9) /* EOI notification bit */ | sourceCoreId;
            // ^ IDK how kvm gets away with not setting the EOI notif bits in some cases,
            // what they do seems to be prone to drop interrupts, etc.

            lr.hw = false; // software
        } else {
            // Actual physical interrupt
            lr.hw = true;
            lr.physicalId = irqId;
        }

        volatile ArmGicV2ListRegister *freeLr = vgicAllocateListRegister();

        if (freeLr == NULL) {
            DEBUG("EL2: vgicPushListRegisters: no free LR!\n");
        }
        *freeLr = lr;
    }
}

static bool vgicUpdateListRegister(volatile ArmGicV2ListRegister *lr)
{
    ArmGicV2ListRegister lrCopy = *lr;
    ArmGicV2ListRegister zero = {0};

    u16 irqId = lrCopy.virtualId;

    // Note: this give priority to multi-SGIs than can be immediately handled

    // Update the state
    VirqState *state = vgicGetVirqState(currentCoreCtx->coreId, irqId);
    u32 srcCoreId = state->coreId;
    u32 coreId = currentCoreCtx->coreId;

    if (!state->handled) {
        PANIC("vgicUpdateListRegister: improper previous state for now pending irq idx %u, active=%d\n", vgicGetVirqStateIndex(state), (int)state->active);
    }

    state->active = lrCopy.active;

    if (lrCopy.active) {
        // We don't dequeue active interrupts
        if (irqId < 16) {
            // We can allow SGIs to be marked active-pending if it's been made pending from the same source again
            if (g_vgicIncomingSgiPendingSources[coreId][irqId] & BIT(srcCoreId)) {
                lrCopy.pending = true;
                g_vgicIncomingSgiPendingSources[coreId][irqId] &= ~BIT(srcCoreId);
            }
        }

        vgicSetVirqPendingState(state, lrCopy.pending);
        *lr = lrCopy;
        return true;
    } else if (lrCopy.pending) {
        // New interrupts might have come, pending status might have been changed, etc.
        // We need to put the interrupt back in the pending list (which we clean up afterwards)
        vgicEnqueueVirqState(&g_virqPendingQueue, state);
        state->handled = false;
        *lr = zero;
        return false;
    } else {
        if (irqId < 16) {
            // Special case for multi-SGIs if they can be immediately handled
            if (g_vgicIncomingSgiPendingSources[coreId][irqId] != 0) {
                srcCoreId = __builtin_ctz(g_vgicIncomingSgiPendingSources[coreId][irqId]);
                state->coreId = srcCoreId;
                g_vgicIncomingSgiPendingSources[coreId][irqId] &= ~BIT(srcCoreId);
                lrCopy.physicalId = BIT(9) /* EOI notification bit */ | srcCoreId;

                lrCopy.pending = true;
                *lr = lrCopy;
            }
        }

        if (!lrCopy.pending) {
            // Inactive interrupt, cleanup
            vgicSetVirqPendingState(state, false);
            state->handled = false;
            *lr = zero;
            return false;
        } else {
            return true;
        }
    }
}

void vgicUpdateState(void)
{
    u32 coreId = currentCoreCtx->coreId;

    // First, put back inactive interrupts into the queue, handle some SGI stuff
    // Need to handle the LRs in reverse order to keep list stability
    u64 usedMap = __rbit64(g_vgicUsedLrMap[coreId]);
    FOREACH_BIT (tmp, pos, usedMap) {
        if (!vgicUpdateListRegister(&gich->lr[63 - pos])) {
            usedMap &= ~BITL(pos);
        }
    }
    g_vgicUsedLrMap[coreId] = __rbit64(usedMap);

    // Then, clean the list up
    vgicCleanupPendingList();

    size_t numChosen;
    size_t numFreeLr = vgicGetNumberOfFreeListRegisters();
    VirqState *chosen[numFreeLr]; // yes this is a VLA, potentially dangerous. Usually max 4 (64 at most)

    // Choose interrupts...
    vgicChoosePendingInterrupts(&numChosen, chosen, numFreeLr);

    // ...and push them
    vgicPushListRegisters(chosen, numChosen);

    // Apparently, the following is not needed because the GIC generates it for us

    // Enable underflow interrupt when appropriate to do so
    if (g_irqManager.numListRegisters - vgicGetNumberOfFreeListRegisters() > 1) {
        gich->hcr.uie = true;
    } else {
        gich->hcr.uie = false;
    }
}

void vgicMaintenanceInterruptHandler(void)
{
    ArmGicV2MaintenanceIntStatRegister misr = gich->misr;

    // Force GICV_CTRL to behave like ns-GICC_CTLR, with group 1 being replaced by group 0
    // Ensure we aren't spammed by maintenance interrupts, either.
    if (misr.vgrp0e || misr.vgrp0d || misr.vgrp1e || misr.vgrp1d) {
        ArmGicV2VmControlRegister vmcr = gich->vmcr;
        vmcr.cbpr = 0;
        vmcr.fiqEn = 0;
        vmcr.ackCtl = 0;
        vmcr.enableGrp1 = 0;
        gich->vmcr = vmcr;
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

    if (misr.lrenp) {
        PANIC("EL2 [core %d]: List Register Entry Not Present maintenance interrupt!\n", currentCoreCtx->coreId);
    }

    if (misr.eoi) {
        //DEBUG("EL2 [core %d]: SGI EOI maintenance interrupt\n", currentCoreCtx->coreId);
    }

    if (misr.u) {
        //DEBUG("EL2 [core %d]: Underflow maintenance interrupt\n", currentCoreCtx->coreId);
    }

    // The rest should be handled by the main loop...
}

void handleVgicdMmio(ExceptionStackFrame *frame, DataAbortIss dabtIss, size_t offset)
{
    size_t sz = BITL(dabtIss.sas);
    uintptr_t addr = (uintptr_t)gicd + offset;
    bool oops = true;

    // ipriorityr, itargetsr, *pendsgir are byte-accessible
    if (
        !(offset >= GICDOFF(ipriorityr) && offset < GICDOFF(ipriorityr) + GIC_IRQID_MAX) &&
        !(offset >= GICDOFF(itargetsr)  && offset < GICDOFF(itargetsr) + GIC_IRQID_MAX) &&
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

    vgicUpdateState();
    recursiveSpinlockUnlock(&g_irqManager.lock);
}

// lock needs to be held by caller
// note, irqId >= 16
void vgicEnqueuePhysicalIrq(u16 irqId)
{
    VirqState *state = vgicGetVirqState(currentCoreCtx->coreId, irqId);
    vgicSetVirqPendingState(state, true);
    vgicEnqueueVirqState(&g_virqPendingQueue, state);
}

void vgicInit(void)
{
    if (currentCoreCtx->isBootCore) {
        // All fields are reset to 0 on reset and deep sleep exit

        g_virqPendingQueue.first = g_virqPendingQueue.last = vgicGetQueueEnd();

        for (u32 i = 0; i < MAX_NUM_INTERRUPTS; i++) {
            g_virqStates[i].listNext = g_virqStates[i].listPrev = VIRQLIST_INVALID_ID;
            g_virqStates[i].priority = 0x1F;
        }

        for (u32 i = 0; i < 4; i++) {
            // SGIs, PPIs
            for (u16 j = 0; j < 32; j++) {
                VirqState *state = vgicGetVirqState(i, j);
                state->targetList = BIT(i);
                if (j < 16) {
                    state->enabled = true;
                } else {
                    state->levelSensitive = (gicd->icfgr[j / 16] & (2 << IRQ_CFGR_SHIFT(j % 16))) == 0;
                }
            }
        }

        // All guest interrupts are initially configured as disabled
        // All guest SPIs are initially configured as edge-triggered with no targets
    }

    // Deassert vIRQ line, just in case
    // Enable a few maintenance interrupts
    ArmGicV2HypervisorControlRegister hcr = {
        .vgrp1eie = true,
        .vgrp0eie = true,
        .lrenpie = true,
        .en = true,
    };
    gich->hcr = hcr;
}
