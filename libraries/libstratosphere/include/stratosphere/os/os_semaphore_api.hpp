/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
#include <vapours.hpp>

namespace ams::os {

    struct SemaphoreType;
    struct WaitableHolderType;

    void InitializeSemaphore(SemaphoreType *sema, s32 count, s32 max_count);
    void FinalizeSemaphore(SemaphoreType *sema);

    void AcquireSemaphore(SemaphoreType *sema);
    bool TryAcquireSemaphore(SemaphoreType *sema);
    bool TimedAcquireSemaphore(SemaphoreType *sema, TimeSpan timeout);

    void ReleaseSemaphore(SemaphoreType *sema);
    void ReleaseSemaphore(SemaphoreType *sema, s32 count);

    s32 GetCurrentSemaphoreCount(const SemaphoreType *sema);

    void InitializeWaitableHolder(WaitableHolderType *waitable_holder, SemaphoreType *sema);

}
