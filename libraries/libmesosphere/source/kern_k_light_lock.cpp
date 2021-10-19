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

        class ThreadQueueImplForKLightLock final : public KThreadQueue {
            public:
                constexpr ThreadQueueImplForKLightLock() : KThreadQueue() { /* ... */ }

                virtual void CancelWait(KThread *waiting_thread, Result wait_result, bool cancel_timer_task) override {
                    /* Do nothing, waiting to acquire a light lock cannot be canceled. */
                    MESOSPHERE_UNUSED(waiting_thread, wait_result, cancel_timer_task);
                }
        };

    }

    bool KLightLock::LockSlowPath(uintptr_t _owner, uintptr_t _cur_thread) {
        KThread *cur_thread = reinterpret_cast<KThread *>(_cur_thread);
        ThreadQueueImplForKLightLock wait_queue;

        /* Pend the current thread waiting on the owner thread. */
        {
            KScopedSchedulerLock sl;

            /* Ensure we actually have locking to do. */
            if (m_tag.Load<std::memory_order_relaxed>() != _owner) {
                return false;
            }

            /* Add the current thread as a waiter on the owner. */
            KThread *owner_thread = reinterpret_cast<KThread *>(_owner & ~1ul);
            cur_thread->SetAddressKey(reinterpret_cast<uintptr_t>(std::addressof(m_tag)));
            owner_thread->AddWaiter(cur_thread);

            /* Begin waiting to hold the lock. */
            cur_thread->BeginWait(std::addressof(wait_queue));

            if (owner_thread->IsSuspended()) {
                owner_thread->ContinueIfHasKernelWaiters();
            }
        }

        return true;
    }

    void KLightLock::UnlockSlowPath(uintptr_t _cur_thread) {
        KThread *owner_thread = reinterpret_cast<KThread *>(_cur_thread);

        /* Unlock. */
        {
            KScopedSchedulerLock sl;

            /* Get the next owner. */
            s32 num_waiters;
            KThread *next_owner = owner_thread->RemoveWaiterByKey(std::addressof(num_waiters), reinterpret_cast<uintptr_t>(std::addressof(m_tag)));

            /* Pass the lock to the next owner. */
            uintptr_t next_tag = 0;
            if (next_owner != nullptr) {
                next_tag = reinterpret_cast<uintptr_t>(next_owner) | static_cast<uintptr_t>(num_waiters > 1);

                next_owner->EndWait(ResultSuccess());

                if (next_owner->IsSuspended()) {
                    next_owner->ContinueIfHasKernelWaiters();
                }
            }

            /* We may have unsuspended in the process of acquiring the lock, so we'll re-suspend now if so. */
            if (owner_thread->IsSuspended()) {
                owner_thread->TrySuspend();
            }

            /* Write the new tag value. */
            m_tag.Store<std::memory_order_release>(next_tag);
        }
    }

}
