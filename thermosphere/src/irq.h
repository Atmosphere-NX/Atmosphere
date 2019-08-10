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

#pragma once

#include "gicv2.h"
#include "spinlock.h"
#include "exceptions.h"
#include "utils.h"

typedef struct IrqManager {
    RecursiveSpinlock lock;
    ArmGicV2 gic;
    u8 numPriorityLevels;
    u8 numCpuInterfaces;
    u8 numSharedInterrupts;
    // Note: we don't store interrupt handlers since we will handle some SGI + uart interrupt(s)...
} IrqManager;

extern IrqManager g_irqManager;

void initIrq(void);
void handleIrqException(ExceptionStackFrame *frame, bool isLowerEl, bool isA32);

static inline void generateSgiForAllOthers(u32 id)
{
    g_irqManager.gic.gicd->sgir = (1 << 24) | (id & 0xF);
}

static inline void generateSgiForSelf(u32 id)
{
    g_irqManager.gic.gicd->sgir = (2 << 24) | (id & 0xF);
}

static inline void generateSgiForList(u32 id, u32 list)
{
    g_irqManager.gic.gicd->sgir = (0 << 24) | (list << 16) | (id & 0xF);
}

static inline void generateSgiForAll(u32 id)
{
    generateSgiForList(id, MASK(g_irqManager.numCpuInterfaces));
}
