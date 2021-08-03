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
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_select_cpu.hpp>
#include <mesosphere/kern_k_light_lock.hpp>
#include <mesosphere/kern_k_thread_queue.hpp>
#include <mesosphere/kern_k_scoped_scheduler_lock_and_sleep.hpp>

namespace ams::kern {

    class KLightConditionVariable {
        private:
            KThread::WaiterList m_wait_list;
        public:
            constexpr ALWAYS_INLINE KLightConditionVariable() : m_wait_list() { /* ... */ }
        private:
            void WaitImpl(KLightLock *lock, s64 timeout, bool allow_terminating_thread) {
                KThread *owner = GetCurrentThreadPointer();
                KHardwareTimer *timer;

                /* Sleep the thread. */
                {
                    KScopedSchedulerLockAndSleep lk(&timer, owner, timeout);

                    if (!allow_terminating_thread && owner->IsTerminationRequested()) {
                        lk.CancelSleep();
                        return;
                    }

                    lock->Unlock();


                    /* Set the thread as waiting. */
                    GetCurrentThread().SetState(KThread::ThreadState_Waiting);

                    /* Add the thread to the queue. */
                    m_wait_list.push_back(GetCurrentThread());
                }

                /* Remove the thread from the wait list. */
                {
                    KScopedSchedulerLock sl;

                    m_wait_list.erase(m_wait_list.iterator_to(GetCurrentThread()));
                }

                /* Cancel the task that the sleep setup. */
                if (timer != nullptr) {
                    timer->CancelTask(owner);
                }

                /* Re-acquire the lock. */
                lock->Lock();
            }
        public:
            void Wait(KLightLock *lock, s64 timeout = -1ll, bool allow_terminating_thread = true) {
                this->WaitImpl(lock, timeout, allow_terminating_thread);
            }

            void Broadcast() {
                KScopedSchedulerLock lk;

                /* Signal all threads. */
                for (auto &thread : m_wait_list) {
                    thread.SetState(KThread::ThreadState_Runnable);
                }
            }

    };

}
