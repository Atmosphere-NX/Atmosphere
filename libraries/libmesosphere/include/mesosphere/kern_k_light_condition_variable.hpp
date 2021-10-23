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
#include <mesosphere/kern_select_cpu.hpp>
#include <mesosphere/kern_k_light_lock.hpp>
#include <mesosphere/kern_k_thread_queue.hpp>
#include <mesosphere/kern_k_scoped_scheduler_lock_and_sleep.hpp>

namespace ams::kern {

    class KLightConditionVariable {
        private:
            KThread::WaiterList m_wait_list;
        public:
            constexpr explicit ALWAYS_INLINE KLightConditionVariable(util::ConstantInitializeTag) : m_wait_list() { /* ... */ }

            explicit ALWAYS_INLINE KLightConditionVariable() { /* ... */ }
        public:
            void Wait(KLightLock *lock, s64 timeout = -1ll, bool allow_terminating_thread = true);
            void Broadcast();
    };

}
