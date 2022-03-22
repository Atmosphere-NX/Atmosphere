/*
 * Copyright (c) Atmosphère-NX
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

    struct LightSemaphoreType;

    void InitializeLightSemaphore(LightSemaphoreType *sema, s32 count, s32 max_count);
    void FinalizeLightSemaphore(LightSemaphoreType *sema);

    void AcquireLightSemaphore(LightSemaphoreType *sema);
    bool TryAcquireLightSemaphore(LightSemaphoreType *sema);
    bool TimedAcquireLightSemaphore(LightSemaphoreType *sema, TimeSpan timeout);

    void ReleaseLightSemaphore(LightSemaphoreType *sema);
    void ReleaseLightSemaphore(LightSemaphoreType *sema, s32 count);

    s32 GetCurrentLightSemaphoreCount(const LightSemaphoreType *sema);

}
