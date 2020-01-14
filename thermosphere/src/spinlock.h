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

#include "core_ctx.h"
#include "utils.h"
#include "sysreg.h"

typedef struct Spinlock {
    u32 lock;
} Spinlock;

typedef struct RecursiveSpinlock {
    Spinlock lock;
    u32 count;
    vu32 tag;
} RecursiveSpinlock;

static inline u64 maskIrq(void)
{
    u64 ret = GET_SYSREG(daif);
    SET_SYSREG_IMM(daifset, BITL(1));
    return ret;
}

static inline u64 unmaskIrq(void)
{
    u64 ret = GET_SYSREG(daif);
    SET_SYSREG_IMM(daifclr, BITL(1));
    return ret;
}

static inline void restoreInterruptFlags(u64 flags)
{
    SET_SYSREG(daif, flags);
}

void spinlockLock(Spinlock *lock);
void spinlockUnlock(Spinlock *lock);

static inline u64 spinlockLockMaskIrq(Spinlock *lock)
{
    u64 ret = maskIrq();
    spinlockLock(lock);
    return ret;
}

static inline void spinlockUnlockRestoreIrq(Spinlock *lock, u64 flags)
{
    spinlockUnlock(lock);
    restoreInterruptFlags(flags);
}

static inline void recursiveSpinlockLock(RecursiveSpinlock *lock)
{
    if (lock->tag != currentCoreCtx->coreId + 1) {
        spinlockLock(&lock->lock);
        lock->tag = currentCoreCtx->coreId + 1;
        lock->count = 1;
    } else {
        ++lock->count;
    }
}

static inline void recursiveSpinlockUnlock(RecursiveSpinlock *lock)
{
    if (--lock->count == 0) {
        lock->tag = 0;
        spinlockUnlock(&lock->lock);
    }
}

static inline u64 recursiveSpinlockLockMaskIrq(RecursiveSpinlock *lock)
{
    u64 ret = maskIrq();
    recursiveSpinlockLock(lock);
    return ret;
}

static inline void recursiveSpinlockUnlockRestoreIrq(RecursiveSpinlock *lock, u64 flags)
{
    recursiveSpinlockUnlock(lock);
    restoreInterruptFlags(flags);
}
