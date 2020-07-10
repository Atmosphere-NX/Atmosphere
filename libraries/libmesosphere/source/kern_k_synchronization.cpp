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
#include <mesosphere.hpp>

namespace ams::kern {

    Result KSynchronization::Wait(s32 *out_index, KSynchronizationObject **objects, const s32 num_objects, s64 timeout) {
        MESOSPHERE_ASSERT_THIS();

        /* Allocate space on stack for thread iterators. */
        KSynchronizationObject::iterator *thread_iters = static_cast<KSynchronizationObject::iterator *>(__builtin_alloca(sizeof(KSynchronizationObject::iterator) * num_objects));

        /* Prepare for wait. */
        KThread *thread = GetCurrentThreadPointer();
        s32 sync_index = -1;
        KHardwareTimer *timer;

        {
            /* Setup the scheduling lock and sleep. */
            KScopedSchedulerLockAndSleep slp(std::addressof(timer), thread, timeout);

            /* Check if any of the objects are already signaled. */
            for (auto i = 0; i < num_objects; ++i) {
                AMS_ASSERT(objects[i] != nullptr);

                if (objects[i]->IsSignaled()) {
                    *out_index = i;
                    slp.CancelSleep();
                    return ResultSuccess();
                }
            }

            /* Check if the timeout is zero. */
            if (timeout == 0) {
                slp.CancelSleep();
                return svc::ResultTimedOut();
            }

            /* Check if the thread should terminate. */
            if (thread->IsTerminationRequested()) {
                slp.CancelSleep();
                return svc::ResultTerminationRequested();
            }

            /* Check if waiting was canceled. */
            if (thread->IsWaitCancelled()) {
                slp.CancelSleep();
                thread->ClearWaitCancelled();
                return svc::ResultCancelled();
            }

            /* Add the waiters. */
            for (auto i = 0; i < num_objects; ++i) {
                thread_iters[i] = objects[i]->RegisterWaitingThread(thread);
            }

            /* Mark the thread as waiting. */
            thread->SetCancellable();
            thread->SetSyncedObject(nullptr, svc::ResultTimedOut());
            thread->SetState(KThread::ThreadState_Waiting);
        }

        /* The lock/sleep is done, so we should be able to get our result. */

        /* Thread is no longer cancellable. */
        thread->ClearCancellable();

        /* Cancel the timer as needed. */
        if (timer != nullptr) {
            timer->CancelTask(thread);
        }

        /* Get the wait result. */
        Result wait_result;
        {
            KScopedSchedulerLock lk;
            KSynchronizationObject *synced_obj;
            wait_result = thread->GetWaitResult(std::addressof(synced_obj));

            for (auto i = 0; i < num_objects; ++i) {
                objects[i]->UnregisterWaitingThread(thread_iters[i]);
                if (objects[i] == synced_obj) {
                    sync_index = i;
                }
            }
        }

        /* Set output. */
        *out_index = sync_index;
        return wait_result;
    }

    void KSynchronization::OnAvailable(KSynchronizationObject *object) {
        MESOSPHERE_ASSERT_THIS();

        KScopedSchedulerLock sl;

        /* If we're not signaled, we've nothing to notify. */
        if (!object->IsSignaled()) {
            return;
        }

        /* Iterate over each thread. */
        for (auto &thread : *object) {
            if (thread.GetState() == KThread::ThreadState_Waiting) {
                thread.SetSyncedObject(object, ResultSuccess());
                thread.SetState(KThread::ThreadState_Runnable);
            }
        }
    }

    void KSynchronization::OnAbort(KSynchronizationObject *object, Result abort_reason) {
        MESOSPHERE_ASSERT_THIS();

        KScopedSchedulerLock sl;

        /* Iterate over each thread. */
        for (auto &thread : *object) {
            if (thread.GetState() == KThread::ThreadState_Waiting) {
                thread.SetSyncedObject(object, abort_reason);
                thread.SetState(KThread::ThreadState_Runnable);
            }
        }
    }

}
