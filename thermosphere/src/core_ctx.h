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
#include "utils.h"

typedef struct CoreCtx {
    u64 kernelArgument;             // @0x00
    uintptr_t kernelEntrypoint;     // @0x08
    u8 *crashStack;                 // @0x10
    u64 scratch;                    // @0x18
    u32 coreId;                     // @0x20
    bool isBootCore;                // @0x24
    bool warmboot;                  // @0x25
} CoreCtx;

extern CoreCtx g_coreCtxs[4];
register CoreCtx *currentCoreCtx asm("x18");

void coreCtxInit(u32 coreId, bool isBootCore, u64 argument);
