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

#include "spinlock.h"
#include "core_ctx.h"

void recursiveSpinlockLock(RecursiveSpinlock *lock)
{
    if (LIKELY(lock->tag != currentCoreCtx->coreId + 1)) {
        spinlockLock(&lock->lock);
        lock->tag = currentCoreCtx->coreId + 1;
        lock->count = 1;
    } else {
        ++lock->count;
    }
}

bool recursiveSpinlockTryLock(RecursiveSpinlock *lock)
{
    if (LIKELY(lock->tag != currentCoreCtx->coreId + 1)) {
        if (!spinlockTryLock(&lock->lock)) {
            return false;
        } else {
            lock->tag = currentCoreCtx->coreId + 1;
            lock->count = 1;
            return true;
        }
    } else {
        ++lock->count;
        return true;
    }
}

void recursiveSpinlockUnlock(RecursiveSpinlock *lock)
{
    if (LIKELY(--lock->count == 0)) {
        lock->tag = 0;
        spinlockUnlock(&lock->lock);
    }
}
