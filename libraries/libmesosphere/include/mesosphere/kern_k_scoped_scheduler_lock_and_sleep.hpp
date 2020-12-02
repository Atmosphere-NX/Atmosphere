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
#include <mesosphere/kern_k_scheduler.hpp>
#include <mesosphere/kern_select_hardware_timer.hpp>
#include <mesosphere/kern_kernel.hpp>

namespace ams::kern {

    class KScopedSchedulerLockAndSleep {
        private:
            s64 timeout_tick;
            KThread *thread;
            KHardwareTimer *timer;
        public:
            explicit ALWAYS_INLINE KScopedSchedulerLockAndSleep(KHardwareTimer **out_timer, KThread *t, s64 timeout) : timeout_tick(timeout), thread(t) {
                /* Lock the scheduler. */
                KScheduler::s_scheduler_lock.Lock();

                /* Set our timer only if the absolute time is positive. */
                this->timer = (this->timeout_tick > 0) ? std::addressof(Kernel::GetHardwareTimer()) : nullptr;

                *out_timer = this->timer;
            }

            ~KScopedSchedulerLockAndSleep() {
                /* Register the sleep. */
                if (this->timeout_tick > 0) {
                    this->timer->RegisterAbsoluteTask(this->thread, this->timeout_tick);
                }

                /* Unlock the scheduler. */
                KScheduler::s_scheduler_lock.Unlock();
            }

            ALWAYS_INLINE void CancelSleep() {
                this->timeout_tick = 0;
            }
    };

}
