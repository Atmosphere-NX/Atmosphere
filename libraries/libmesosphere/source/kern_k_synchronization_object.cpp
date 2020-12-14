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

    void KSynchronizationObject::Finalize() {
        MESOSPHERE_ASSERT_THIS();

        /* If auditing, ensure that the object has no waiters. */
        #if defined(MESOSPHERE_BUILD_FOR_AUDITING)
        {
            KScopedSchedulerLock sl;

            for (auto *cur_node = this->thread_list_root; cur_node != nullptr; cur_node = cur_node->next) {
                KThread *thread = cur_node->thread;
                MESOSPHERE_LOG("KSynchronizationObject::Finalize(%p) with %p (id=%ld) waiting.\n", this, thread, thread->GetId());
            }
        }
        #endif

        this->OnFinalizeSynchronizationObject();
        KAutoObject::Finalize();
    }

    Result KSynchronizationObject::Wait(s32 *out_index, KSynchronizationObject **objects, const s32 num_objects, s64 timeout) {
        /* Allocate space on stack for thread nodes. */
        ThreadListNode *thread_nodes = static_cast<ThreadListNode *>(__builtin_alloca(sizeof(ThreadListNode) * num_objects));

        /* Prepare for wait. */
        KThread *thread = GetCurrentThreadPointer();
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
                thread_nodes[i].thread      = thread;
                thread_nodes[i].next        = nullptr;

                if (objects[i]->thread_list_tail == nullptr) {
                    objects[i]->thread_list_head = std::addressof(thread_nodes[i]);
                } else {
                    objects[i]->thread_list_tail->next = std::addressof(thread_nodes[i]);
                }

                objects[i]->thread_list_tail = std::addressof(thread_nodes[i]);
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
        s32 sync_index = -1;
        {
            KScopedSchedulerLock lk;
            KSynchronizationObject *synced_obj;
            wait_result = thread->GetWaitResult(std::addressof(synced_obj));

            for (auto i = 0; i < num_objects; ++i) {
                /* Unlink the object from the list. */
                ThreadListNode *prev_ptr = reinterpret_cast<ThreadListNode *>(std::addressof(objects[i]->thread_list_head));
                ThreadListNode *prev_val = nullptr;
                ThreadListNode *prev, *tail_prev;

                do {
                    prev      = prev_ptr;
                    prev_ptr  = prev_ptr->next;
                    tail_prev = prev_val;
                    prev_val  = prev_ptr;
                } while (prev_ptr != std::addressof(thread_nodes[i]));

                if (objects[i]->thread_list_tail == std::addressof(thread_nodes[i])) {
                    objects[i]->thread_list_tail = tail_prev;
                }

                prev->next = thread_nodes[i].next;

                if (objects[i] == synced_obj) {
                    sync_index = i;
                }
            }
        }

        /* Set output. */
        *out_index = sync_index;
        return wait_result;
    }

    void KSynchronizationObject::NotifyAvailable(Result result) {
        MESOSPHERE_ASSERT_THIS();

        KScopedSchedulerLock sl;

        /* If we're not signaled, we've nothing to notify. */
        if (!this->IsSignaled()) {
            return;
        }

        /* Iterate over each thread. */
        for (auto *cur_node = this->thread_list_head; cur_node != nullptr; cur_node = cur_node->next) {
            KThread *thread = cur_node->thread;
            if (thread->GetState() == KThread::ThreadState_Waiting) {
                thread->SetSyncedObject(this, result);
                thread->SetState(KThread::ThreadState_Runnable);
            }
        }
    }

    void KSynchronizationObject::DumpWaiters() {
        MESOSPHERE_ASSERT_THIS();

        /* If debugging, dump the list of waiters. */
        #if defined(MESOSPHERE_BUILD_FOR_DEBUGGING)
        {
            KScopedSchedulerLock sl;

            MESOSPHERE_RELEASE_LOG("Threads waiting on %p:\n", this);

            for (auto *cur_node = this->thread_list_head; cur_node != nullptr; cur_node = cur_node->next) {
                KThread *thread = cur_node->thread;

                if (KProcess *process = thread->GetOwnerProcess(); process != nullptr) {
                    MESOSPHERE_RELEASE_LOG("    %p tid=%ld pid=%ld (%s)\n", thread, thread->GetId(), process->GetId(), process->GetName());
                } else {
                    MESOSPHERE_RELEASE_LOG("    %p tid=%ld (Kernel)\n", thread, thread->GetId());
                }
            }

            /* If we didn't have any waiters, print so. */
            if (this->thread_list_head == nullptr) {
                MESOSPHERE_RELEASE_LOG("    None\n");
            }
        }
        #endif
    }

}
