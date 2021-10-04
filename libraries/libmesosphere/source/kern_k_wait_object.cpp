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

        class ThreadQueueImplForKWaitObjectSynchronize final : public KThreadQueueWithoutEndWait {
            private:
                KThread::WaiterList *m_wait_list;
                KThread **m_thread;
            public:
                constexpr ThreadQueueImplForKWaitObjectSynchronize(KThread::WaiterList *wl, KThread **t) : KThreadQueueWithoutEndWait(), m_wait_list(wl), m_thread(t) { /* ... */ }

                virtual void CancelWait(KThread *waiting_thread, Result wait_result, bool cancel_timer_task) override {
                    /* Remove the thread from the wait list. */
                    m_wait_list->erase(m_wait_list->iterator_to(*waiting_thread));

                    /* If the result was a timeout and the thread is our wait object thread, cancel recursively. */
                    if (svc::ResultTimedOut::Includes(wait_result) && waiting_thread == *m_thread) {
                        for (auto &thread : *m_wait_list) {
                            thread.CancelWait(svc::ResultTimedOut(), false);
                        }
                    }

                    /* If the thread is our wait object thread, clear it. */
                    if (*m_thread == waiting_thread) {
                        *m_thread = nullptr;
                    }

                    /* Invoke the base cancel wait handler. */
                    KThreadQueue::CancelWait(waiting_thread, wait_result, cancel_timer_task);
                }
        };

    }

    Result KWaitObject::Synchronize(s64 timeout) {
        /* Perform the wait. */
        KHardwareTimer *timer;
        KThread *cur_thread = GetCurrentThreadPointer();
        ThreadQueueImplForKWaitObjectSynchronize wait_queue(std::addressof(m_wait_list), std::addressof(m_next_thread));

        {
            KScopedSchedulerLockAndSleep slp(std::addressof(timer), cur_thread, timeout);

            /* Check that the thread isn't terminating. */
            if (cur_thread->IsTerminationRequested()) {
                slp.CancelSleep();
                return svc::ResultTerminationRequested();
            }

            /* Handle the case where timeout is non-negative/infinite. */
            if (timeout >= 0) {
                /* Check if we're already waiting. */
                if (m_next_thread != nullptr) {
                    slp.CancelSleep();
                    return svc::ResultBusy();
                }

                /* If timeout is zero, handle the special case by canceling all waiting threads. */
                if (timeout == 0) {
                    for (auto &thread : m_wait_list) {
                        thread.CancelWait(svc::ResultTimedOut(), false);
                    }

                    slp.CancelSleep();
                    return ResultSuccess();
                }
            }

            /* If the timeout isn't infinite, register it as our next timeout. */
            if (timeout > 0) {
                wait_queue.SetHardwareTimer(timer);
                m_next_thread = cur_thread;
            }

            /* Add the current thread to our wait list. */
            m_wait_list.push_back(*cur_thread);

            /* Wait until the timeout occurs. */
            cur_thread->BeginWait(std::addressof(wait_queue));
        }

        return ResultSuccess();
    }

}
