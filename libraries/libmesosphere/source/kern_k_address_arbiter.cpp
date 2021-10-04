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

        ALWAYS_INLINE bool ReadFromUser(s32 *out, KProcessAddress address) {
            return UserspaceAccess::CopyMemoryFromUserSize32Bit(out, GetVoidPointer(address));
        }

        ALWAYS_INLINE bool DecrementIfLessThan(s32 *out, KProcessAddress address, s32 value) {
            KScopedInterruptDisable di;

            if (!cpu::CanAccessAtomic(address)) {
                return false;
            }

            return UserspaceAccess::DecrementIfLessThanAtomic(out, GetPointer<s32>(address), value);
        }

        ALWAYS_INLINE bool UpdateIfEqual(s32 *out, KProcessAddress address, s32 value, s32 new_value) {
            KScopedInterruptDisable di;

            if (!cpu::CanAccessAtomic(address)) {
                return false;
            }

            return UserspaceAccess::UpdateIfEqualAtomic(out, GetPointer<s32>(address), value, new_value);
        }

        class ThreadQueueImplForKAddressArbiter final : public KThreadQueue {
            private:
                KAddressArbiter::ThreadTree *m_tree;
            public:
                constexpr ThreadQueueImplForKAddressArbiter(KAddressArbiter::ThreadTree *t) : KThreadQueue(), m_tree(t) { /* ... */ }

                virtual void CancelWait(KThread *waiting_thread, Result wait_result, bool cancel_timer_task) override {
                    /* If the thread is waiting on an address arbiter, remove it from the tree. */
                    if (waiting_thread->IsWaitingForAddressArbiter()) {
                        m_tree->erase(m_tree->iterator_to(*waiting_thread));
                        waiting_thread->ClearAddressArbiter();
                    }

                    /* Invoke the base cancel wait handler. */
                    KThreadQueue::CancelWait(waiting_thread, wait_result, cancel_timer_task);
                }
        };

    }

    Result KAddressArbiter::Signal(uintptr_t addr, s32 count) {
        /* Perform signaling. */
        s32 num_waiters = 0;
        {
            KScopedSchedulerLock sl;

            auto it = m_tree.nfind_key({ addr, -1 });
            while ((it != m_tree.end()) && (count <= 0 || num_waiters < count) && (it->GetAddressArbiterKey() == addr)) {
                /* End the thread's wait. */
                KThread *target_thread = std::addressof(*it);
                target_thread->EndWait(ResultSuccess());

                MESOSPHERE_ASSERT(target_thread->IsWaitingForAddressArbiter());
                target_thread->ClearAddressArbiter();

                it = m_tree.erase(it);
                ++num_waiters;
            }
        }
        return ResultSuccess();
    }

    Result KAddressArbiter::SignalAndIncrementIfEqual(uintptr_t addr, s32 value, s32 count) {
        /* Perform signaling. */
        s32 num_waiters = 0;
        {
            KScopedSchedulerLock sl;

            /* Check the userspace value. */
            s32 user_value;
            R_UNLESS(UpdateIfEqual(std::addressof(user_value), addr, value, value + 1), svc::ResultInvalidCurrentMemory());
            R_UNLESS(user_value == value,                                               svc::ResultInvalidState());

            auto it = m_tree.nfind_key({ addr, -1 });
            while ((it != m_tree.end()) && (count <= 0 || num_waiters < count) && (it->GetAddressArbiterKey() == addr)) {
                /* End the thread's wait. */
                KThread *target_thread = std::addressof(*it);
                target_thread->EndWait(ResultSuccess());

                MESOSPHERE_ASSERT(target_thread->IsWaitingForAddressArbiter());
                target_thread->ClearAddressArbiter();

                it = m_tree.erase(it);
                ++num_waiters;
            }
        }
        return ResultSuccess();
    }

    Result KAddressArbiter::SignalAndModifyByWaitingCountIfEqual(uintptr_t addr, s32 value, s32 count) {
        /* Perform signaling. */
        s32 num_waiters = 0;
        {
            KScopedSchedulerLock sl;

            auto it = m_tree.nfind_key({ addr, -1 });
            /* Determine the updated value. */
            s32 new_value;
            if (count <= 0) {
                if ((it != m_tree.end()) && (it->GetAddressArbiterKey() == addr)) {
                    new_value = value - 2;
                } else {
                    new_value = value + 1;
                }
            } else {
                if ((it != m_tree.end()) && (it->GetAddressArbiterKey() == addr)) {
                    auto tmp_it = it;
                    s32 tmp_num_waiters = 0;
                    while ((++tmp_it != m_tree.end()) && (tmp_it->GetAddressArbiterKey() == addr)) {
                        if ((tmp_num_waiters++) >= count) {
                            break;
                        }
                    }

                    if (tmp_num_waiters < count) {
                        new_value = value - 1;
                    } else {
                        new_value = value;
                    }
                } else {
                    new_value = value + 1;
                }
            }

            /* Check the userspace value. */
            s32 user_value;
            bool succeeded;
            if (value != new_value) {
                succeeded = UpdateIfEqual(std::addressof(user_value), addr, value, new_value);
            } else {
                succeeded = ReadFromUser(std::addressof(user_value), addr);
            }

            R_UNLESS(succeeded,           svc::ResultInvalidCurrentMemory());
            R_UNLESS(user_value == value, svc::ResultInvalidState());

            while ((it != m_tree.end()) && (count <= 0 || num_waiters < count) && (it->GetAddressArbiterKey() == addr)) {
                /* End the thread's wait. */
                KThread *target_thread = std::addressof(*it);
                target_thread->EndWait(ResultSuccess());

                MESOSPHERE_ASSERT(target_thread->IsWaitingForAddressArbiter());
                target_thread->ClearAddressArbiter();

                it = m_tree.erase(it);
                ++num_waiters;
            }
        }
        return ResultSuccess();
    }

    Result KAddressArbiter::WaitIfLessThan(uintptr_t addr, s32 value, bool decrement, s64 timeout) {
        /* Prepare to wait. */
        KThread *cur_thread = GetCurrentThreadPointer();
        KHardwareTimer *timer;
        ThreadQueueImplForKAddressArbiter wait_queue(std::addressof(m_tree));

        {
            KScopedSchedulerLockAndSleep slp(std::addressof(timer), cur_thread, timeout);

            /* Check that the thread isn't terminating. */
            if (cur_thread->IsTerminationRequested()) {
                slp.CancelSleep();
                return svc::ResultTerminationRequested();
            }

            /* Read the value from userspace. */
            s32 user_value;
            bool succeeded;
            if (decrement) {
                succeeded = DecrementIfLessThan(std::addressof(user_value), addr, value);
            } else {
                succeeded = ReadFromUser(std::addressof(user_value), addr);
            }

            if (!succeeded) {
                slp.CancelSleep();
                return svc::ResultInvalidCurrentMemory();
            }

            /* Check that the value is less than the specified one. */
            if (user_value >= value) {
                slp.CancelSleep();
                return svc::ResultInvalidState();
            }

            /* Check that the timeout is non-zero. */
            if (timeout == 0) {
                slp.CancelSleep();
                return svc::ResultTimedOut();
            }

            /* Set the arbiter. */
            cur_thread->SetAddressArbiter(std::addressof(m_tree), addr);
            m_tree.insert(*cur_thread);

            /* Wait for the thread to finish. */
            wait_queue.SetHardwareTimer(timer);
            cur_thread->BeginWait(std::addressof(wait_queue));
        }

        /* Get the wait result. */
        return cur_thread->GetWaitResult();
    }

    Result KAddressArbiter::WaitIfEqual(uintptr_t addr, s32 value, s64 timeout) {
        /* Prepare to wait. */
        KThread *cur_thread = GetCurrentThreadPointer();
        KHardwareTimer *timer;
        ThreadQueueImplForKAddressArbiter wait_queue(std::addressof(m_tree));

        {
            KScopedSchedulerLockAndSleep slp(std::addressof(timer), cur_thread, timeout);

            /* Check that the thread isn't terminating. */
            if (cur_thread->IsTerminationRequested()) {
                slp.CancelSleep();
                return svc::ResultTerminationRequested();
            }

            /* Read the value from userspace. */
            s32 user_value;
            if (!ReadFromUser(std::addressof(user_value), addr)) {
                slp.CancelSleep();
                return svc::ResultInvalidCurrentMemory();
            }

            /* Check that the value is equal. */
            if (value != user_value) {
                slp.CancelSleep();
                return svc::ResultInvalidState();
            }

            /* Check that the timeout is non-zero. */
            if (timeout == 0) {
                slp.CancelSleep();
                return svc::ResultTimedOut();
            }

            /* Set the arbiter. */
            cur_thread->SetAddressArbiter(std::addressof(m_tree), addr);
            m_tree.insert(*cur_thread);

            /* Wait for the thread to finish. */
            wait_queue.SetHardwareTimer(timer);
            cur_thread->BeginWait(std::addressof(wait_queue));
        }

        /* Get the wait result. */
        return cur_thread->GetWaitResult();
    }

}
