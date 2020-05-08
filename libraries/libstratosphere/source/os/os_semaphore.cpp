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
#include <stratosphere.hpp>
#include "impl/os_waitable_object_list.hpp"
#include "impl/os_timeout_helper.hpp"

namespace ams::os {

    void InitializeSemaphore(SemaphoreType *sema, s32 count, s32 max_count) {
        AMS_ASSERT(max_count >= 1);
        AMS_ASSERT(count     >= 0);

        /* Setup objects. */
        new (GetPointer(sema->cs_sema))      impl::InternalCriticalSection;
        new (GetPointer(sema->cv_not_zero))  impl::InternalConditionVariable;

        /* Setup wait lists. */
        new (GetPointer(sema->waitlist)) impl::WaitableObjectList;

        /* Set member variables. */
        sema->count     = count;
        sema->max_count = max_count;

        /* Mark initialized. */
        sema->state = SemaphoreType::State_Initialized;
    }

    void FinalizeSemaphore(SemaphoreType *sema) {
        AMS_ASSERT(sema->state = SemaphoreType::State_Initialized);

        AMS_ASSERT(GetReference(sema->waitlist).IsEmpty());

        /* Mark uninitialized. */
        sema->state = SemaphoreType::State_NotInitialized;

        /* Destroy wait lists. */
        GetReference(sema->waitlist).~WaitableObjectList();

        /* Destroy objects. */
        GetReference(sema->cv_not_zero).~InternalConditionVariable();
        GetReference(sema->cs_sema).~InternalCriticalSection();
    }

    void AcquireSemaphore(SemaphoreType *sema) {
        AMS_ASSERT(sema->state == SemaphoreType::State_Initialized);

        {
            std::scoped_lock lk(GetReference(sema->cs_sema));

            while (sema->count == 0) {
                GetReference(sema->cv_not_zero).Wait(GetPointer(sema->cs_sema));
            }

            --sema->count;
        }
    }

    bool TryAcquireSemaphore(SemaphoreType *sema) {
        AMS_ASSERT(sema->state == SemaphoreType::State_Initialized);

        {
            std::scoped_lock lk(GetReference(sema->cs_sema));

            if (sema->count == 0) {
                return false;
            }

            --sema->count;
        }

        return true;
    }

    bool TimedAcquireSemaphore(SemaphoreType *sema, TimeSpan timeout) {
        AMS_ASSERT(sema->state == SemaphoreType::State_Initialized);
        AMS_ASSERT(timeout.GetNanoSeconds() >= 0);

        {
            impl::TimeoutHelper timeout_helper(timeout);
            std::scoped_lock lk(GetReference(sema->cs_sema));

            while (sema->count == 0) {
                if (timeout_helper.TimedOut()) {
                    return false;
                }
                GetReference(sema->cv_not_zero).TimedWait(GetPointer(sema->cs_sema), timeout_helper);
            }

            --sema->count;
        }

        return true;
    }

    void ReleaseSemaphore(SemaphoreType *sema) {
        AMS_ASSERT(sema->state == SemaphoreType::State_Initialized);

        {
            std::scoped_lock lk(GetReference(sema->cs_sema));

            AMS_ASSERT(sema->count + 1 <= sema->max_count);

            ++sema->count;

            GetReference(sema->cv_not_zero).Signal();
            GetReference(sema->waitlist).SignalAllThreads();
        }
    }

    void ReleaseSemaphore(SemaphoreType *sema, s32 count) {
        AMS_ASSERT(sema->state == SemaphoreType::State_Initialized);

        {
            std::scoped_lock lk(GetReference(sema->cs_sema));

            AMS_ASSERT(sema->count + count <= sema->max_count);

            sema->count += count;

            GetReference(sema->cv_not_zero).Signal();
            GetReference(sema->waitlist).SignalAllThreads();
        }
    }

    s32 GetCurrentSemaphoreCount(const SemaphoreType *sema) {
        AMS_ASSERT(sema->state == SemaphoreType::State_Initialized);

        return sema->count;
    }

    // void InitializeWaitableHolder(WaitableHolderType *waitable_holder, SemaphoreType *sema);

}
