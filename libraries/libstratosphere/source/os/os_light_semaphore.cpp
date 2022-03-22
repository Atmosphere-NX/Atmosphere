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
#include <stratosphere.hpp>
#include "impl/os_timeout_helper.hpp"

namespace ams::os {

    void InitializeLightSemaphore(LightSemaphoreType *sema, s32 count, s32 max_count) {
        /* Check pre-conditions. */
        AMS_ASSERT(max_count >= 1);
        AMS_ASSERT(0 <= count && count <= max_count);

        /* Setup objects. */
        util::ConstructAt(sema->mutex);
        util::ConstructAt(sema->ev_not_zero, count > 0);

        /* Set member variables. */
        sema->count     = count;
        sema->max_count = max_count;

        /* Mark initialized. */
        sema->state = LightSemaphoreType::State_Initialized;
    }

    void FinalizeLightSemaphore(LightSemaphoreType *sema) {
        /* Check pre-conditions. */
        AMS_ASSERT(sema->state == LightSemaphoreType::State_Initialized);

        /* Mark uninitialized. */
        sema->state = LightSemaphoreType::State_NotInitialized;

        /* Destroy objects. */
        util::DestroyAt(sema->mutex);
        util::DestroyAt(sema->ev_not_zero);
    }

    void AcquireLightSemaphore(LightSemaphoreType *sema) {
        /* Check pre-conditions. */
        AMS_ASSERT(sema->state == LightSemaphoreType::State_Initialized);

        /* Repeatedly try to acquire the semaphore. */
        while (true) {
            /* Try to acquire the semaphore. */
            {
                /* Acquire exclusive access to the semaphore. */
                std::scoped_lock lk(util::GetReference(sema->mutex));

                /* If we can, acquire. */
                if (sema->count > 0) {
                    --sema->count;
                    return;
                }

                /* Ensure that we can wait once we're unlocked. */
                util::GetReference(sema->ev_not_zero).Clear();
            }

            /* Wait until we can try again. */
            util::GetReference(sema->ev_not_zero).WaitWithManualClear();
        }
    }

    bool TryAcquireLightSemaphore(LightSemaphoreType *sema) {
        /* Check pre-conditions. */
        AMS_ASSERT(sema->state == LightSemaphoreType::State_Initialized);

        /* Acquire exclusive access to the semaphore. */
        std::scoped_lock lk(util::GetReference(sema->mutex));

        /* Try to acquire. */
        if (sema->count > 0) {
            --sema->count;
            return true;
        } else {
            return false;
        }
    }

    bool TimedAcquireLightSemaphore(LightSemaphoreType *sema, TimeSpan timeout) {
        /* Check pre-conditions. */
        AMS_ASSERT(sema->state == LightSemaphoreType::State_Initialized);
        AMS_ASSERT(timeout.GetNanoSeconds() >= 0);

        /* Create timeout helper. */
        impl::TimeoutHelper timeout_helper(timeout);

        /* Repeatedly try to acquire the semaphore. */
        while (true) {
            /* Try to acquire the semaphore. */
            {
                /* Acquire exclusive access to the semaphore. */
                std::scoped_lock lk(util::GetReference(sema->mutex));

                /* If we can, acquire. */
                if (sema->count > 0) {
                    --sema->count;
                    return true;
                }

                /* Ensure that we can wait once we're unlocked. */
                util::GetReference(sema->ev_not_zero).Clear();
            }

            /* Check if we're timed out. */
            if (timeout_helper.TimedOut()) {
                return false;
            }

            /* Wait until we can try again. */
            util::GetReference(sema->ev_not_zero).TimedWaitWithManualClear(timeout_helper);
        }
    }

    void ReleaseLightSemaphore(LightSemaphoreType *sema) {
        /* Check pre-conditions. */
        AMS_ASSERT(sema->state == LightSemaphoreType::State_Initialized);

        /* Release the semaphore. */
        {
            /* Acquire exclusive access to the semaphore. */
            std::scoped_lock lk(util::GetReference(sema->mutex));

            /* Check that we can release. */
            AMS_ASSERT(sema->count + 1 <= sema->max_count);

            /* Release. */
            ++sema->count;
        }

        /* Signal that we released. */
        util::GetReference(sema->ev_not_zero).SignalWithManualClear();
    }

    void ReleaseLightSemaphore(LightSemaphoreType *sema, s32 count) {
        /* Check pre-conditions. */
        AMS_ASSERT(sema->state == LightSemaphoreType::State_Initialized);
        AMS_ASSERT(count >= 1);

        /* Release the semaphore. */
        {
            /* Acquire exclusive access to the semaphore. */
            std::scoped_lock lk(util::GetReference(sema->mutex));

            /* Check that we can release. */
            AMS_ASSERT(sema->count + count <= sema->max_count);

            /* Release. */
            sema->count += count;
        }

        /* Signal that we released. */
        util::GetReference(sema->ev_not_zero).SignalWithManualClear();
    }

    s32 GetCurrentLightSemaphoreCount(const LightSemaphoreType *sema) {
        /* Check pre-conditions. */
        AMS_ASSERT(sema->state == LightSemaphoreType::State_Initialized);

        /* Acquire exclusive access to the semaphore. */
        std::scoped_lock lk(util::GetReference(sema->mutex));

        /* Get the count. */
        return sema->count;
    }

}
