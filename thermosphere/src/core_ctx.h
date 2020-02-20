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
#include <stdatomic.h>
#include <assert.h>
#include "utils.h"

struct ExceptionStackFrame;
typedef struct ALIGN(64) CoreCtx {
    // Most likely only just read (assume cache line size of at most 64 bytes):

    u64 kernelArgument;                                     // @0x08
    uintptr_t kernelEntrypoint;                             // @0x10
    u32 coreId;                                             // @0x18
    bool isBootCore;                                        // @0x1D
    bool warmboot;                                          // @0x1E

    // Debug features
    bool wasPaused;                                         // @0x1F
    uintptr_t steppingRangeStartAddr;                       // @0x20
    uintptr_t steppingRangeEndAddr;                         // @0x28

    // Most likely written to:

    ALIGN(64) struct ExceptionStackFrame *guestFrame;       // @0x40

    // Timer stuff
    u64 totalTimeInHypervisor;                              // @0x50. cntvoff_el2 is updated to that value.
    u64 emulPtimerCval;                                     // @0x58. When setting cntp_cval_el0 and on interrupt
} CoreCtx;

/*static_assert(offsetof(CoreCtx, warmboot) == 0x1E, "Wrong definition for CoreCtx");
static_assert(offsetof(CoreCtx, emulPtimerCval) == 0x58, "Wrong definition for CoreCtx");
static_assert(offsetof(CoreCtx, executedFunctionSync) == 0x78, "Wrong definition for CoreCtx");
static_assert(offsetof(CoreCtx, setWayCounter) == 0x7C, "Wrong definition for CoreCtx");*/

extern CoreCtx g_coreCtxs[4];
register CoreCtx *currentCoreCtx asm("x18");

void coreCtxInit(u32 coreId, bool isBootCore, u64 argument);

void setCurrentCoreActive(void);
u32 getActiveCoreMask(void);
