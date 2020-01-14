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

#include "fpu.h"
#include "execute_function.h"
#include "core_ctx.h"

FpuRegisterStorage TEMPORARY g_fpuRegisterStorage[4] = { 0 };

// fpu_regs_load_store.s
void fpuLoadRegistersFromStorage(const FpuRegisterStorage *storage);
void fpuStoreRegistersToStorage(FpuRegisterStorage *storage);

static void fpuDumpRegistersImpl(void *p)
{
    (void)p;
    fpuStoreRegistersToStorage(&g_fpuRegisterStorage[currentCoreCtx->coreId]);
}

static void fpuRestoreRegistersImpl(void *p)
{
    (void)p;
    fpuLoadRegistersFromStorage(&g_fpuRegisterStorage[currentCoreCtx->coreId]);
}

void fpuDumpRegisters(u32 coreList)
{
    executeFunctionOnCores(fpuDumpRegistersImpl, NULL, true, coreList);
}

void fpuRestoreRegisters(u32 coreList)
{
    executeFunctionOnCores(fpuRestoreRegistersImpl, NULL, true, coreList);
}
