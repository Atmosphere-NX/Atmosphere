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

        class ThreadQueueImplForKLightConditionVariable final : public KThreadQueue {
            private:
                KThread::WaiterList *m_wait_list;
                bool m_allow_terminating_thread;
            public:
                constexpr ThreadQueueImplForKLightConditionVariable(KThread::WaiterList *wl, bool term) : KThreadQueue(), m_wait_list(wl), m_allow_terminating_thread(term) { /* ... */ }

                virtual void CancelWait(KThread *waiting_thread, Result wait_result, bool cancel_timer_task) override {
                    /* Only process waits if we're allowed to. */
                    if (svc::ResultTerminationRequested::Includes(wait_result) && m_allow_terminating_thread) {
                        return;
                    }

                    /* Remove the thread from the waiting thread from the light condition variable. */
                    m_wait_list->erase(m_wait_list->iterator_to(*waiting_thread));

                    /* Invoke the base cancel wait handler. */
                    KThreadQueue::CancelWait(waiting_thread, wait_result, cancel_timer_task);
                }
        };

    }

    void KLightConditionVariable::Wait(KLightLock *lock, s64 timeout, bool allow_terminating_thread) {
        /* Create thread queue. */
        KThread *owner = GetCurrentThreadPointer();
        KHardwareTimer *timer;

        ThreadQueueImplForKLightConditionVariable wait_queue(std::addressof(m_wait_list), allow_terminating_thread);

        /* Sleep the thread. */
        {
            KScopedSchedulerLockAndSleep lk(&timer, owner, timeout);

            if (!allow_terminating_thread && owner->IsTerminationRequested()) {
                lk.CancelSleep();
                return;
            }

            lock->Unlock();

            /* Add the thread to the queue. */
            m_wait_list.push_back(*owner);

            /* Begin waiting. */
            wait_queue.SetHardwareTimer(timer);
            owner->BeginWait(std::addressof(wait_queue));
        }

        /* Re-acquire the lock. */
        lock->Lock();
    }

    void KLightConditionVariable::Broadcast() {
        KScopedSchedulerLock lk;

        /* Signal all threads. */
        for (auto it = m_wait_list.begin(); it != m_wait_list.end(); it = m_wait_list.erase(it)) {
            it->EndWait(ResultSuccess());
        }
    }

}
