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
            KThreadQueue m_thread_queue;
        public:
            constexpr ALWAYS_INLINE KLightConditionVariable() : m_thread_queue() { /* ... */ }
        private:
            void WaitImpl(KLightLock *lock, s64 timeout) {
                KThread *owner = GetCurrentThreadPointer();
                KHardwareTimer *timer;

                /* Sleep the thread. */
                {
                    KScopedSchedulerLockAndSleep lk(&timer, owner, timeout);
                    lock->Unlock();

                    if (!m_thread_queue.SleepThread(owner)) {
                        lk.CancelSleep();
                        return;
                    }
                }

                /* Cancel the task that the sleep setup. */
                if (timer != nullptr) {
                    timer->CancelTask(owner);
                }
            }
        public:
            void Wait(KLightLock *lock, s64 timeout = -1ll) {
                this->WaitImpl(lock, timeout);
                lock->Lock();
            }

            void Broadcast() {
                KScopedSchedulerLock lk;
                while (m_thread_queue.WakeupFrontThread() != nullptr) {
                    /* We want to signal all threads, and so should continue waking up until there's nothing to wake. */
                }
            }

    };

}
