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

#include "execute_function.h"
#include "utils.h"
#include "core_ctx.h"
#include "irq.h"

void executeFunctionOnCores(ExecutedFunction fun, void *args, bool sync, u32 coreList)
{
    barrierInit(&currentCoreCtx->executedFunctionBarrier, coreList);
    currentCoreCtx->executedFunction = fun;
    currentCoreCtx->executedFunctionArgs = args;
    currentCoreCtx->executedFunctionSync = sync;

    generateSgiForList(ThermosphereSgi_ExecuteFunction, coreList);
}

void executeFunctionOnAllCores(ExecutedFunction fun, void *args, bool sync)
{
    executeFunctionOnCores(fun, args, sync, getActiveCoreMask());
}

void executeFunctionOnAllCoresButSelf(ExecutedFunction fun, void *args, bool sync)
{
    executeFunctionOnCores(fun, args, sync, getActiveCoreMask() & ~(BIT(currentCoreCtx->coreId)));
}

void executeFunctionInterruptHandler(u32 srcCore)
{
    CoreCtx *ctx = &g_coreCtxs[srcCore];
    ctx->executedFunction(ctx->executedFunctionArgs);
    if (ctx->executedFunctionSync) {
        barrierWait(&ctx->executedFunctionBarrier);
    }
}