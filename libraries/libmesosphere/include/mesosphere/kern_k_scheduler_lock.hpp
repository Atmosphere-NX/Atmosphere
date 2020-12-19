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
#include <mesosphere/kern_k_spin_lock.hpp>
#include <mesosphere/kern_k_current_context.hpp>
#include <mesosphere/kern_k_scoped_lock.hpp>

namespace ams::kern {

    class KThread;

    template<typename T>
    concept KSchedulerLockable = !std::is_reference<T>::value && requires(T) {
        { T::DisableScheduling()                    } -> std::same_as<void>;
        { T::EnableScheduling(std::declval<u64>())  } -> std::same_as<void>;
        { T::UpdateHighestPriorityThreads()         } -> std::convertible_to<u64>;
    };

    template<typename SchedulerType> requires KSchedulerLockable<SchedulerType>
    class KAbstractSchedulerLock {
        private:
            KAlignedSpinLock m_spin_lock;
            s32 m_lock_count;
            KThread *m_owner_thread;
        public:
            constexpr ALWAYS_INLINE KAbstractSchedulerLock() : m_spin_lock(), m_lock_count(0), m_owner_thread(nullptr) { MESOSPHERE_ASSERT_THIS(); }

            ALWAYS_INLINE bool IsLockedByCurrentThread() const {
                MESOSPHERE_ASSERT_THIS();

                return m_owner_thread == GetCurrentThreadPointer();
            }

            void Lock() {
                MESOSPHERE_ASSERT_THIS();

                if (this->IsLockedByCurrentThread()) {
                    /* If we already own the lock, we can just increment the count. */
                    MESOSPHERE_ASSERT(m_lock_count > 0);
                    m_lock_count++;
                } else {
                    /* Otherwise, we want to disable scheduling and acquire the spinlock. */
                    SchedulerType::DisableScheduling();
                    m_spin_lock.Lock();

                    /* For debug, ensure that our state is valid. */
                    MESOSPHERE_ASSERT(m_lock_count == 0);
                    MESOSPHERE_ASSERT(m_owner_thread == nullptr);

                    /* Increment count, take ownership. */
                    m_lock_count = 1;
                    m_owner_thread = GetCurrentThreadPointer();
                }
            }

            void Unlock() {
                MESOSPHERE_ASSERT_THIS();
                MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());
                MESOSPHERE_ASSERT(m_lock_count > 0);

                /* Release an instance of the lock. */
                if ((--m_lock_count) == 0) {
                    /* We're no longer going to hold the lock. Take note of what cores need scheduling. */
                    const u64 cores_needing_scheduling = SchedulerType::UpdateHighestPriorityThreads();

                   /* Note that we no longer hold the lock, and unlock the spinlock. */
                    m_owner_thread = nullptr;
                    m_spin_lock.Unlock();

                    /* Enable scheduling, and perform a rescheduling operation. */
                    SchedulerType::EnableScheduling(cores_needing_scheduling);
                }
            }
    };

}
