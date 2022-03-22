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
#pragma once
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_k_thread.hpp>
#include <mesosphere/kern_select_hardware_timer.hpp>

namespace ams::kern {

    class KThreadQueue {
        private:
            KHardwareTimer *m_hardware_timer;
        public:
            constexpr ALWAYS_INLINE KThreadQueue() : m_hardware_timer(nullptr) { /* ... */ }

            constexpr void SetHardwareTimer(KHardwareTimer *timer) { m_hardware_timer = timer; }

            virtual void NotifyAvailable(KThread *waiting_thread, KSynchronizationObject *signaled_object, Result wait_result);
            virtual void EndWait(KThread *waiting_thread, Result wait_result);
            virtual void CancelWait(KThread *waiting_thread, Result wait_result, bool cancel_timer_task);
    };

    class KThreadQueueWithoutEndWait : public KThreadQueue {
        public:
            constexpr ALWAYS_INLINE KThreadQueueWithoutEndWait() : KThreadQueue() { /* ... */ }

            virtual void EndWait(KThread *waiting_thread, Result wait_result) override final;
    };

}
