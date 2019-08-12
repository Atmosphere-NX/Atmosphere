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

#include <stdatomic.h>
#include "barrier.h"
#include "core_ctx.h"
#include "utils.h"


void barrierInit(Barrier *barrier, u32 coreList)
{
    atomic_store(&barrier->val, coreList);
}

void barrierInitAllButSelf(Barrier *barrier)
{
    barrierInit(barrier, getActiveCoreMask() & ~(BIT(currentCoreCtx->coreId)));
}

void barrierInitAll(Barrier *barrier)
{
    barrierInit(barrier, getActiveCoreMask());
}

void barrierWait(Barrier *barrier)
{
    while (atomic_fetch_and(&barrier->val, ~(BIT(currentCoreCtx->coreId))) != 0);
}
