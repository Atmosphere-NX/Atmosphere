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
            s64 m_timeout_tick;
            KThread *m_thread;
            KHardwareTimer *m_timer;
        public:
            explicit ALWAYS_INLINE KScopedSchedulerLockAndSleep(KHardwareTimer **out_timer, KThread *t, s64 timeout) : m_timeout_tick(timeout), m_thread(t) {
                /* Lock the scheduler. */
                KScheduler::s_scheduler_lock.Lock();

                /* Set our timer only if the absolute time is positive. */
                m_timer = (m_timeout_tick > 0) ? std::addressof(Kernel::GetHardwareTimer()) : nullptr;

                *out_timer = m_timer;
            }

            ~KScopedSchedulerLockAndSleep() {
                /* Register the sleep. */
                if (m_timeout_tick > 0) {
                    m_timer->RegisterAbsoluteTask(m_thread, m_timeout_tick);
                }

                /* Unlock the scheduler. */
                KScheduler::s_scheduler_lock.Unlock();
            }

            ALWAYS_INLINE void CancelSleep() {
                m_timeout_tick = 0;
            }
    };

}
