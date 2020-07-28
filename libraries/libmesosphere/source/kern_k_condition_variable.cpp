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

    constinit KThread g_cv_arbiter_compare_thread;

    namespace {

        ALWAYS_INLINE bool ReadFromUser(u32 *out, KProcessAddress address) {
            return UserspaceAccess::CopyMemoryFromUserSize32Bit(out, GetVoidPointer(address));
        }

        ALWAYS_INLINE bool WriteToUser(KProcessAddress address, const u32 *p) {
            return UserspaceAccess::CopyMemoryToUserSize32Bit(GetVoidPointer(address), p);
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
                    owner_thread = GetCurrentProcess().GetHandleTable().GetObject<KThread>(handle);
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
        MESOSPHERE_UNIMPLEMENTED();
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
            g_cv_arbiter_compare_thread.SetupForConditionVariableCompare(cv_key, -1);

            auto it = this->tree.nfind(g_cv_arbiter_compare_thread);
            while ((it != this->tree.end()) && (count <= 0 || num_waiters < count) && (it->GetConditionVariableKey() == cv_key)) {
                KThread *target_thread = std::addressof(*it);

                if (KThread *thread = this->SignalImpl(target_thread); thread != nullptr) {
                    if (num_to_close < MaxThreads) {
                        thread_array[num_to_close++] = thread;
                    } else {
                        thread_list.push_back(*thread);
                    }
                }

                it = this->tree.erase(it);
                target_thread->ClearConditionVariableTree();
                ++num_waiters;
            }
        }

        /* Close threads in the array. */
        for (auto i = 0; i < num_to_close; ++i) {
            thread_array[i]->Close();
        }

        /* Close threads in the list. */
        if (num_waiters > MaxThreads) {
            auto it = thread_list.begin();
            while (it != thread_list.end()) {
                KThread *thread = std::addressof(*it);
                thread->Close();
                it = thread_list.erase(it);
            }
        }
    }

    Result KConditionVariable::Wait(KProcessAddress addr, uintptr_t key, u32 value, s64 timeout) {
        MESOSPHERE_UNIMPLEMENTED();
    }

}
