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

    }

    Result KConditionVariable::SignalToAddress(KProcessAddress addr) {
        KThread *owner_thread = std::addressof(GetCurrentThread());

        /* Signal the address. */
        {
            KScopedSchedulerLock sl;

            /* Remove waiter thread. */
            s32 num_waiters;
            KThread *next_owner_thread = owner_thread->RemoveWaiterByKey(std::addressof(num_waiters), addr);

            /* Determine the next tag. */
            u32 next_value = 0;
            if (next_owner_thread) {
                next_value = next_owner_thread->GetAddressKeyValue();
                if (num_waiters > 1) {
                    next_value |= ams::svc::HandleWaitMask;
                }

                next_owner_thread->SetSyncedObject(nullptr, ResultSuccess());
                next_owner_thread->Wakeup();
            }

            /* Write the value to userspace. */
            if (!WriteToUser(addr, std::addressof(next_value))) {
                if (next_owner_thread) {
                    next_owner_thread->SetSyncedObject(nullptr, svc::ResultInvalidCurrentMemory());
                }

                return svc::ResultInvalidCurrentMemory();
            }
        }

        return ResultSuccess();
    }

    Result KConditionVariable::WaitForAddress(ams::svc::Handle handle, KProcessAddress addr, u32 value) {
        KThread *cur_thread = std::addressof(GetCurrentThread());

        /* Wait for the address. */
        {
            KScopedAutoObject<KThread> owner_thread;
            MESOSPHERE_ASSERT(owner_thread.IsNull());
            {
                KScopedSchedulerLock sl;
                cur_thread->SetSyncedObject(nullptr, ResultSuccess());

                /* Check if the thread should terminate. */
                R_UNLESS(!cur_thread->IsTerminationRequested(), svc::ResultTerminationRequested());

                {
                    /* Read the tag from userspace. */
                    u32 test_tag;
                    R_UNLESS(ReadFromUser(std::addressof(test_tag), addr), svc::ResultInvalidCurrentMemory());

                    /* If the tag isn't the handle (with wait mask), we're done. */
                    R_SUCCEED_IF(test_tag != (handle | ams::svc::HandleWaitMask));

                    /* Get the lock owner thread. */
                    owner_thread = GetCurrentProcess().GetHandleTable().GetObjectWithoutPseudoHandle<KThread>(handle);
                    R_UNLESS(owner_thread.IsNotNull(), svc::ResultInvalidHandle());

                    /* Update the lock. */
                    cur_thread->SetAddressKey(addr, value);
                    owner_thread->AddWaiter(cur_thread);
                    cur_thread->SetState(KThread::ThreadState_Waiting);
                }
            }
            MESOSPHERE_ASSERT(owner_thread.IsNotNull());
        }

        /* Remove the thread as a waiter from the lock owner. */
        {
            KScopedSchedulerLock sl;
            KThread *owner_thread = cur_thread->GetLockOwner();
            if (owner_thread != nullptr) {
                owner_thread->RemoveWaiter(cur_thread);
            }
        }

        /* Get the wait result. */
        KSynchronizationObject *dummy;
        return cur_thread->GetWaitResult(std::addressof(dummy));
    }

    KThread *KConditionVariable::SignalImpl(KThread *thread) {
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
                UpdateLockAtomic(std::addressof(prev_tag), address, own_tag, ams::svc::HandleWaitMask);
            }
        }

        KThread *thread_to_close = nullptr;
        if (AMS_LIKELY(can_access)) {
            if (prev_tag == ams::svc::InvalidHandle) {
                /* If nobody held the lock previously, we're all good. */
                thread->SetSyncedObject(nullptr, ResultSuccess());
                thread->Wakeup();
            } else {
                /* Get the previous owner. */
                KThread *owner_thread = GetCurrentProcess().GetHandleTable().GetObjectWithoutPseudoHandle<KThread>(static_cast<ams::svc::Handle>(prev_tag & ~ams::svc::HandleWaitMask))
                                                                            .ReleasePointerUnsafe();
                if (AMS_LIKELY(owner_thread != nullptr)) {
                    /* Add the thread as a waiter on the owner. */
                    owner_thread->AddWaiter(thread);
                    thread_to_close = owner_thread;
                } else {
                    /* The lock was tagged with a thread that doesn't exist. */
                    thread->SetSyncedObject(nullptr, svc::ResultInvalidState());
                    thread->Wakeup();
                }
            }
        } else {
            /* If the address wasn't accessible, note so. */
            thread->SetSyncedObject(nullptr, svc::ResultInvalidCurrentMemory());
            thread->Wakeup();
        }

        return thread_to_close;
    }

    void KConditionVariable::Signal(uintptr_t cv_key, s32 count) {
        /* Prepare for signaling. */
        constexpr int MaxThreads = 16;
        KLinkedList<KThread> thread_list;
        KThread *thread_array[MaxThreads];
        int num_to_close = 0;

        /* Perform signaling. */
        int num_waiters = 0;
        {
            KScopedSchedulerLock sl;

            auto it = m_tree.nfind_light({ cv_key, -1 });
            while ((it != m_tree.end()) && (count <= 0 || num_waiters < count) && (it->GetConditionVariableKey() == cv_key)) {
                KThread *target_thread = std::addressof(*it);

                if (KThread *thread = this->SignalImpl(target_thread); thread != nullptr) {
                    if (num_to_close < MaxThreads) {
                        thread_array[num_to_close++] = thread;
                    } else {
                        thread_list.push_back(*thread);
                    }
                }

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

        /* Close threads in the array. */
        for (auto i = 0; i < num_to_close; ++i) {
            thread_array[i]->Close();
        }

        /* Close threads in the list. */
        for (auto it = thread_list.begin(); it != thread_list.end(); it = thread_list.erase(it)) {
            (*it).Close();
        }
    }

    Result KConditionVariable::Wait(KProcessAddress addr, uintptr_t key, u32 value, s64 timeout) {
        /* Prepare to wait. */
        KThread *cur_thread = GetCurrentThreadPointer();
        KHardwareTimer *timer;

        {
            KScopedSchedulerLockAndSleep slp(std::addressof(timer), cur_thread, timeout);

            /* Set the synced object. */
            cur_thread->SetSyncedObject(nullptr, ams::svc::ResultTimedOut());

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
                    next_owner_thread->SetSyncedObject(nullptr, ResultSuccess());
                    next_owner_thread->Wakeup();
                }

                /* Write to the cv key. */
                {
                    const u32 has_waiter_flag = 1;
                    WriteToUser(key, std::addressof(has_waiter_flag));
                    cpu::DataMemoryBarrier();
                }

                /* Write the value to userspace. */
                if (!WriteToUser(addr, std::addressof(next_value))) {
                    slp.CancelSleep();
                    return svc::ResultInvalidCurrentMemory();
                }
            }

            /* Update condition variable tracking. */
            {
                cur_thread->SetConditionVariable(std::addressof(m_tree), addr, key, value);
                m_tree.insert(*cur_thread);
            }

            /* If the timeout is non-zero, set the thread as waiting. */
            if (timeout != 0) {
                cur_thread->SetState(KThread::ThreadState_Waiting);
            }
        }

        /* Cancel the timer wait. */
        if (timer != nullptr) {
            timer->CancelTask(cur_thread);
        }

        /* Remove from the condition variable. */
        {
            KScopedSchedulerLock sl;

            if (KThread *owner = cur_thread->GetLockOwner(); owner != nullptr) {
                owner->RemoveWaiter(cur_thread);
            }

            if (cur_thread->IsWaitingForConditionVariable()) {
                m_tree.erase(m_tree.iterator_to(*cur_thread));
                cur_thread->ClearConditionVariable();
            }
        }

        /* Get the result. */
        KSynchronizationObject *dummy;
        return cur_thread->GetWaitResult(std::addressof(dummy));
    }

}
