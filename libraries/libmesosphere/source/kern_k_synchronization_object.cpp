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
#include <mesosphere.hpp>

namespace ams::kern {

    namespace {

        class ThreadQueueImplForKSynchronizationObjectWait final : public KThreadQueueWithoutEndWait {
            private:
                using ThreadListNode = KSynchronizationObject::ThreadListNode;
            private:
                KSynchronizationObject **m_objects;
                ThreadListNode *m_nodes;
                s32 m_count;
            public:
                constexpr ThreadQueueImplForKSynchronizationObjectWait(KSynchronizationObject **o, ThreadListNode *n, s32 c) : m_objects(o), m_nodes(n), m_count(c) { /* ... */ }

                virtual void NotifyAvailable(KThread *waiting_thread, KSynchronizationObject *signaled_object, Result wait_result) override {
                    /* Determine the sync index, and unlink all nodes. */
                    s32 sync_index = -1;
                    for (auto i = 0; i < m_count; ++i) {
                        /* Check if this is the signaled object. */
                        if (m_objects[i] == signaled_object && sync_index == -1) {
                            sync_index = i;
                        }

                        /* Unlink the current node from the current object. */
                        m_objects[i]->UnlinkNode(std::addressof(m_nodes[i]));
                    }

                    /* Set the waiting thread's sync index. */
                    waiting_thread->SetSyncedIndex(sync_index);

                    /* Set the waiting thread as not cancellable. */
                    waiting_thread->ClearCancellable();

                    /* Invoke the base end wait handler. */
                    KThreadQueue::EndWait(waiting_thread, wait_result);
                }

                virtual void CancelWait(KThread *waiting_thread, Result wait_result, bool cancel_timer_task) override {
                    /* Remove all nodes from our list. */
                    for (auto i = 0; i < m_count; ++i) {
                        m_objects[i]->UnlinkNode(std::addressof(m_nodes[i]));
                    }

                    /* Set the waiting thread as not cancellable. */
                    waiting_thread->ClearCancellable();

                    /* Invoke the base cancel wait handler. */
                    KThreadQueue::CancelWait(waiting_thread, wait_result, cancel_timer_task);
                }
        };

    }

    void KSynchronizationObject::Finalize() {
        MESOSPHERE_ASSERT_THIS();

        /* If auditing, ensure that the object has no waiters. */
        #if defined(MESOSPHERE_BUILD_FOR_AUDITING)
        {
            KScopedSchedulerLock sl;

            for (auto *cur_node = m_thread_list_head; cur_node != nullptr; cur_node = cur_node->next) {
                KThread *thread = cur_node->thread;
                MESOSPHERE_LOG("KSynchronizationObject::Finalize(%p) with %p (id=%ld) waiting.\n", this, thread, thread->GetId());
            }
        }
        #endif

        /* NOTE: In Nintendo's kernel, the following is virtual and called here. */
        /* this->OnFinalizeSynchronizationObject(); */
    }

    Result KSynchronizationObject::Wait(s32 *out_index, KSynchronizationObject **objects, const s32 num_objects, s64 timeout) {
        /* Allocate space on stack for thread nodes. */
        ThreadListNode *thread_nodes = static_cast<ThreadListNode *>(__builtin_alloca(sizeof(ThreadListNode) * num_objects));

        /* Prepare for wait. */
        KThread *thread = GetCurrentThreadPointer();
        KHardwareTimer *timer;
        ThreadQueueImplForKSynchronizationObjectWait wait_queue(objects, thread_nodes, num_objects);

        {
            /* Setup the scheduling lock and sleep. */
            KScopedSchedulerLockAndSleep slp(std::addressof(timer), thread, timeout);

            /* Check if the thread should terminate. */
            if (thread->IsTerminationRequested()) {
                slp.CancelSleep();
                return svc::ResultTerminationRequested();
            }

            /* Check if any of the objects are already signaled. */
            for (auto i = 0; i < num_objects; ++i) {
                MESOSPHERE_ASSERT(objects[i] != nullptr);

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

                objects[i]->LinkNode(std::addressof(thread_nodes[i]));
            }

            /* Mark the thread as cancellable. */
            thread->SetCancellable();

            /* Clear the thread's synced index. */
            thread->SetSyncedIndex(-1);

            /* Wait for an object to be signaled. */
            wait_queue.SetHardwareTimer(timer);
            thread->BeginWait(std::addressof(wait_queue));
        }

        /* Set the output index. */
        *out_index = thread->GetSyncedIndex();

        /* Get the wait result. */
        return thread->GetWaitResult();
    }

    void KSynchronizationObject::NotifyAvailable(Result result) {
        MESOSPHERE_ASSERT_THIS();

        KScopedSchedulerLock sl;

        /* If we're not signaled, we've nothing to notify. */
        if (!this->IsSignaled()) {
            return;
        }

        /* Iterate over each thread. */
        for (auto *cur_node = m_thread_list_head; cur_node != nullptr; cur_node = cur_node->next) {
            cur_node->thread->NotifyAvailable(this, result);
        }
    }

    void KSynchronizationObject::DumpWaiters() {
        MESOSPHERE_ASSERT_THIS();

        /* If debugging, dump the list of waiters. */
        #if defined(MESOSPHERE_BUILD_FOR_DEBUGGING)
        {
            KScopedSchedulerLock sl;

            MESOSPHERE_RELEASE_LOG("Threads waiting on %p:\n", this);

            for (auto *cur_node = m_thread_list_head; cur_node != nullptr; cur_node = cur_node->next) {
                KThread *thread = cur_node->thread;

                if (KProcess *process = thread->GetOwnerProcess(); process != nullptr) {
                    MESOSPHERE_RELEASE_LOG("    %p tid=%ld pid=%ld (%s)\n", thread, thread->GetId(), process->GetId(), process->GetName());
                } else {
                    MESOSPHERE_RELEASE_LOG("    %p tid=%ld (Kernel)\n", thread, thread->GetId());
                }
            }

            /* If we didn't have any waiters, print so. */
            if (m_thread_list_head == nullptr) {
                MESOSPHERE_RELEASE_LOG("    None\n");
            }
        }
        #endif
    }

}
