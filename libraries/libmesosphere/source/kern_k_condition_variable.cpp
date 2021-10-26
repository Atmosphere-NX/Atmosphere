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

        ALWAYS_INLINE bool ReadFromUser(u32 *out, KProcessAddress address) {
            return UserspaceAccess::CopyMemoryFromUserSize32Bit(out, GetVoidPointer(address));
        }

        ALWAYS_INLINE bool WriteToUser(KProcessAddress address, const u32 *p) {
            return UserspaceAccess::CopyMemoryToUserSize32Bit(GetVoidPointer(address), p);
        }

        ALWAYS_INLINE bool UpdateLockAtomic(u32 *out, KProcessAddress address, u32 if_zero, u32 new_orr_mask) {
            return UserspaceAccess::UpdateLockAtomic(out, GetPointer<u32>(address), if_zero, new_orr_mask);
        }

        class ThreadQueueImplForKConditionVariableWaitForAddress final : public KThreadQueue {
            public:
                constexpr ThreadQueueImplForKConditionVariableWaitForAddress() : KThreadQueue() { /* ... */ }

                virtual void CancelWait(KThread *waiting_thread, Result wait_result, bool cancel_timer_task) override {
                    /* Remove the thread as a waiter from its owner. */
                    waiting_thread->GetLockOwner()->RemoveWaiter(waiting_thread);

                    /* Invoke the base cancel wait handler. */
                    KThreadQueue::CancelWait(waiting_thread, wait_result, cancel_timer_task);
                }
        };

        class ThreadQueueImplForKConditionVariableWaitConditionVariable final : public KThreadQueue {
            private:
                KConditionVariable::ThreadTree *m_tree;
            public:
                constexpr ThreadQueueImplForKConditionVariableWaitConditionVariable(KConditionVariable::ThreadTree *t) : KThreadQueue(), m_tree(t) { /* ... */ }

                virtual void CancelWait(KThread *waiting_thread, Result wait_result, bool cancel_timer_task) override {
                    /* Remove the thread as a waiter from its owner. */
                    if (KThread *owner = waiting_thread->GetLockOwner(); owner != nullptr) {
                        owner->RemoveWaiter(waiting_thread);
                    }

                    /* If the thread is waiting on a condvar, remove it from the tree. */
                    if (waiting_thread->IsWaitingForConditionVariable()) {
                        m_tree->erase(m_tree->iterator_to(*waiting_thread));
                        waiting_thread->ClearConditionVariable();
                    }

                    /* Invoke the base cancel wait handler. */
                    KThreadQueue::CancelWait(waiting_thread, wait_result, cancel_timer_task);
                }
        };

    }

    Result KConditionVariable::SignalToAddress(KProcessAddress addr) {
        KThread *owner_thread = GetCurrentThreadPointer();

        /* Signal the address. */
        {
            KScopedSchedulerLock sl;

            /* Remove waiter thread. */
            s32 num_waiters;
            KThread *next_owner_thread = owner_thread->RemoveWaiterByKey(std::addressof(num_waiters), addr);

            /* Determine the next tag. */
            u32 next_value = 0;
            if (next_owner_thread != nullptr) {
                next_value = next_owner_thread->GetAddressKeyValue();
                if (num_waiters > 1) {
                    next_value |= ams::svc::HandleWaitMask;
                }

                /* Write the value to userspace. */
                Result result;
                if (AMS_LIKELY(WriteToUser(addr, std::addressof(next_value)))) {
                    result = ResultSuccess();
                } else {
                    result = svc::ResultInvalidCurrentMemory();
                }

                /* Signal the next owner thread. */
                next_owner_thread->EndWait(result);
                return result;
            } else {
                /* Just write the value to userspace. */
                R_UNLESS(WriteToUser(addr, std::addressof(next_value)), svc::ResultInvalidCurrentMemory());

                return ResultSuccess();
            }
        }
    }

    Result KConditionVariable::WaitForAddress(ams::svc::Handle handle, KProcessAddress addr, u32 value) {
        KThread *cur_thread = GetCurrentThreadPointer();
        ThreadQueueImplForKConditionVariableWaitForAddress wait_queue;

        /* Wait for the address. */
        KThread *owner_thread;
        {
            KScopedSchedulerLock sl;

            /* Check if the thread should terminate. */
            R_UNLESS(!cur_thread->IsTerminationRequested(), svc::ResultTerminationRequested());

            /* Read the tag from userspace. */
            u32 test_tag;
            R_UNLESS(ReadFromUser(std::addressof(test_tag), addr), svc::ResultInvalidCurrentMemory());

            /* If the tag isn't the handle (with wait mask), we're done. */
            R_SUCCEED_IF(test_tag != (handle | ams::svc::HandleWaitMask));

            /* Get the lock owner thread. */
            owner_thread = GetCurrentProcess().GetHandleTable().GetObjectWithoutPseudoHandle<KThread>(handle).ReleasePointerUnsafe();
            R_UNLESS(owner_thread != nullptr, svc::ResultInvalidHandle());

            /* Update the lock. */
            cur_thread->SetAddressKey(addr, value);
            owner_thread->AddWaiter(cur_thread);

            /* Begin waiting. */
            cur_thread->BeginWait(std::addressof(wait_queue));
        }

        /* Close our reference to the owner thread, now that the wait is over. */
        owner_thread->Close();

        /* Get the wait result. */
        return cur_thread->GetWaitResult();
    }

    void KConditionVariable::SignalImpl(KThread *thread) {
        /* Check pre-conditions. */
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());

        /* Update the tag. */
        KProcessAddress address = thread->GetAddressKey();
        u32 own_tag             = thread->GetAddressKeyValue();

        u32 prev_tag;
        bool can_access;
        {
            KScopedInterruptDisable di;

            can_access = cpu::CanAccessAtomic(address);
            if (AMS_LIKELY(can_access)) {
                can_access = UpdateLockAtomic(std::addressof(prev_tag), address, own_tag, ams::svc::HandleWaitMask);
            }
        }

        if (AMS_LIKELY(can_access)) {
            if (prev_tag == ams::svc::InvalidHandle) {
                /* If nobody held the lock previously, we're all good. */
                thread->EndWait(ResultSuccess());
            } else {
                /* Get the previous owner. */
                KThread *owner_thread = GetCurrentProcess().GetHandleTable().GetObjectWithoutPseudoHandle<KThread>(static_cast<ams::svc::Handle>(prev_tag & ~ams::svc::HandleWaitMask))
                                                                            .ReleasePointerUnsafe();
                if (AMS_LIKELY(owner_thread != nullptr)) {
                    /* Add the thread as a waiter on the owner. */
                    owner_thread->AddWaiter(thread);
                    owner_thread->Close();
                } else {
                    /* The lock was tagged with a thread that doesn't exist. */
                    thread->EndWait(svc::ResultInvalidState());
                }
            }
        } else {
            /* If the address wasn't accessible, note so. */
            thread->EndWait(svc::ResultInvalidCurrentMemory());
        }
    }

    void KConditionVariable::Signal(uintptr_t cv_key, s32 count) {
        /* Perform signaling. */
        int num_waiters = 0;
        {
            KScopedSchedulerLock sl;

            auto it = m_tree.nfind_key({ cv_key, -1 });
            while ((it != m_tree.end()) && (count <= 0 || num_waiters < count) && (it->GetConditionVariableKey() == cv_key)) {
                KThread *target_thread = std::addressof(*it);

                this->SignalImpl(target_thread);
                it = m_tree.erase(it);
                target_thread->ClearConditionVariable();
                ++num_waiters;
            }

            /* If we have no waiters, clear the has waiter flag. */
            if (it == m_tree.end() || it->GetConditionVariableKey() != cv_key) {
                const u32 has_waiter_flag = 0;
                WriteToUser(cv_key, std::addressof(has_waiter_flag));
            }
        }
    }

    Result KConditionVariable::Wait(KProcessAddress addr, uintptr_t key, u32 value, s64 timeout) {
        /* Prepare to wait. */
        KThread *cur_thread = GetCurrentThreadPointer();
        KHardwareTimer *timer;
        ThreadQueueImplForKConditionVariableWaitConditionVariable wait_queue(std::addressof(m_tree));

        {
            KScopedSchedulerLockAndSleep slp(std::addressof(timer), cur_thread, timeout);

            /* Check that the thread isn't terminating. */
            if (cur_thread->IsTerminationRequested()) {
                slp.CancelSleep();
                return svc::ResultTerminationRequested();
            }

            /* Update the value and process for the next owner. */
            {
                /* Remove waiter thread. */
                s32 num_waiters;
                KThread *next_owner_thread = cur_thread->RemoveWaiterByKey(std::addressof(num_waiters), GetInteger(addr));

                /* Update for the next owner thread. */
                u32 next_value = 0;
                if (next_owner_thread != nullptr) {
                    /* Get the next tag value. */
                    next_value = next_owner_thread->GetAddressKeyValue();
                    if (num_waiters > 1) {
                        next_value |= ams::svc::HandleWaitMask;
                    }

                    /* Wake up the next owner. */
                    next_owner_thread->EndWait(ResultSuccess());
                }

                /* Write to the cv key. */
                {
                    const u32 has_waiter_flag = 1;
                    WriteToUser(key, std::addressof(has_waiter_flag));
                    cpu::DataMemoryBarrierInnerShareable();
                }

                /* Write the value to userspace. */
                if (!WriteToUser(addr, std::addressof(next_value))) {
                    slp.CancelSleep();
                    return svc::ResultInvalidCurrentMemory();
                }
            }

            /* If timeout is zero, time out. */
            R_UNLESS(timeout != 0, svc::ResultTimedOut());

            /* Update condition variable tracking. */
            cur_thread->SetConditionVariable(std::addressof(m_tree), addr, key, value);
            m_tree.insert(*cur_thread);

            /* Begin waiting. */
            wait_queue.SetHardwareTimer(timer);
            cur_thread->BeginWait(std::addressof(wait_queue));
        }

        /* Get the wait result. */
        return cur_thread->GetWaitResult();
    }

}
