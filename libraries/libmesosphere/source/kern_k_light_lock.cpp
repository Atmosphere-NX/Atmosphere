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
#include <mesosphere.hpp>

namespace ams::kern {

    void KLightLock::LockSlowPath(uintptr_t _owner, uintptr_t _cur_thread) {
        KThread *cur_thread = reinterpret_cast<KThread *>(_cur_thread);

        /* Pend the current thread waiting on the owner thread. */
        {
            KScopedSchedulerLock sl;

            /* Ensure we actually have locking to do. */
            if (AMS_UNLIKELY(m_tag.load(std::memory_order_relaxed) != _owner)) {
                return;
            }

            /* Add the current thread as a waiter on the owner. */
            KThread *owner_thread = reinterpret_cast<KThread *>(_owner & ~1ul);
            cur_thread->SetAddressKey(reinterpret_cast<uintptr_t>(std::addressof(m_tag)));
            owner_thread->AddWaiter(cur_thread);

            /* Set thread states. */
            if (AMS_LIKELY(cur_thread->GetState() == KThread::ThreadState_Runnable)) {
                cur_thread->SetState(KThread::ThreadState_Waiting);
            } else {
                KScheduler::SetSchedulerUpdateNeeded();
            }

            if (owner_thread->IsSuspended()) {
                owner_thread->ContinueIfHasKernelWaiters();
                KScheduler::SetSchedulerUpdateNeeded();
            }
        }

        /* We're no longer waiting on the lock owner. */
        {
            KScopedSchedulerLock sl;
            KThread *owner_thread = cur_thread->GetLockOwner();
            if (AMS_UNLIKELY(owner_thread)) {
                owner_thread->RemoveWaiter(cur_thread);
            }
        }
    }

    void KLightLock::UnlockSlowPath(uintptr_t _cur_thread) {
        KThread *owner_thread = reinterpret_cast<KThread *>(_cur_thread);

        /* Unlock. */
        {
            KScopedSchedulerLock sl;

            /* Get the next owner. */
            s32 num_waiters = 0;
            KThread *next_owner = owner_thread->RemoveWaiterByKey(std::addressof(num_waiters), reinterpret_cast<uintptr_t>(std::addressof(m_tag)));

            /* Pass the lock to the next owner. */
            uintptr_t next_tag = 0;
            if (next_owner) {
                next_tag = reinterpret_cast<uintptr_t>(next_owner);
                if (num_waiters > 1) {
                    next_tag |= 0x1;
                }

                if (AMS_LIKELY(next_owner->GetState() == KThread::ThreadState_Waiting)) {
                    next_owner->SetState(KThread::ThreadState_Runnable);
                } else {
                    KScheduler::SetSchedulerUpdateNeeded();
                }

                if (next_owner->IsSuspended()) {
                    next_owner->ContinueIfHasKernelWaiters();
                }
            }

            /* We may have unsuspended in the process of acquiring the lock, so we'll re-suspend now if so. */
            if (owner_thread->IsSuspended()) {
                owner_thread->TrySuspend();
            }

            /* Write the new tag value. */
            m_tag.store(next_tag);
        }
    }

}
