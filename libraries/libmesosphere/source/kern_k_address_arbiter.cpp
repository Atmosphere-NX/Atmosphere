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

    }

    Result KAddressArbiter::Signal(uintptr_t addr, s32 count) {
        /* Perform signaling. */
        s32 num_waiters = 0;
        {
            KScopedSchedulerLock sl;

            auto it = this->tree.nfind_light({ addr, -1 });
            while ((it != this->tree.end()) && (count <= 0 || num_waiters < count) && (it->GetAddressArbiterKey() == addr)) {
                KThread *target_thread = std::addressof(*it);
                target_thread->SetSyncedObject(nullptr, ResultSuccess());

                AMS_ASSERT(target_thread->IsWaitingForAddressArbiter());
                target_thread->Wakeup();

                it = this->tree.erase(it);
                target_thread->ClearAddressArbiter();
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

            auto it = this->tree.nfind_light({ addr, -1 });
            while ((it != this->tree.end()) && (count <= 0 || num_waiters < count) && (it->GetAddressArbiterKey() == addr)) {
                KThread *target_thread = std::addressof(*it);
                target_thread->SetSyncedObject(nullptr, ResultSuccess());

                AMS_ASSERT(target_thread->IsWaitingForAddressArbiter());
                target_thread->Wakeup();

                it = this->tree.erase(it);
                target_thread->ClearAddressArbiter();
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

            auto it = this->tree.nfind_light({ addr, -1 });
            /* Determine the updated value. */
            s32 new_value;
            if (GetTargetFirmware() >= TargetFirmware_7_0_0) {
                if (count <= 0) {
                    if ((it != this->tree.end()) && (it->GetAddressArbiterKey() == addr)) {
                        new_value = value - 2;
                    } else {
                        new_value = value + 1;
                    }
                } else {
                    if ((it != this->tree.end()) && (it->GetAddressArbiterKey() == addr)) {
                        auto tmp_it = it;
                        s32 tmp_num_waiters = 0;
                        while ((++tmp_it != this->tree.end()) && (tmp_it->GetAddressArbiterKey() == addr)) {
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
            } else {
                if (count <= 0) {
                    if ((it != this->tree.end()) && (it->GetAddressArbiterKey() == addr)) {
                        new_value = value - 1;
                    } else {
                        new_value = value + 1;
                    }
                } else {
                    auto tmp_it = it;
                    s32 tmp_num_waiters = 0;
                    while ((tmp_it != this->tree.end()) && (tmp_it->GetAddressArbiterKey() == addr) && (tmp_num_waiters < count + 1)) {
                        ++tmp_num_waiters;
                        ++tmp_it;
                    }

                    if (tmp_num_waiters == 0) {
                        new_value = value + 1;
                    } else if (tmp_num_waiters <= count) {
                        new_value = value - 1;
                    } else {
                        new_value = value;
                    }
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

            while ((it != this->tree.end()) && (count <= 0 || num_waiters < count) && (it->GetAddressArbiterKey() == addr)) {
                KThread *target_thread = std::addressof(*it);
                target_thread->SetSyncedObject(nullptr, ResultSuccess());

                AMS_ASSERT(target_thread->IsWaitingForAddressArbiter());
                target_thread->Wakeup();

                it = this->tree.erase(it);
                target_thread->ClearAddressArbiter();
                ++num_waiters;
            }
        }
        return ResultSuccess();
    }

    Result KAddressArbiter::WaitIfLessThan(uintptr_t addr, s32 value, bool decrement, s64 timeout) {
        /* Prepare to wait. */
        KThread *cur_thread = GetCurrentThreadPointer();
        KHardwareTimer *timer;

        {
            KScopedSchedulerLockAndSleep slp(std::addressof(timer), cur_thread, timeout);

            /* Check that the thread isn't terminating. */
            if (cur_thread->IsTerminationRequested()) {
                slp.CancelSleep();
                return svc::ResultTerminationRequested();
            }

            /* Set the synced object. */
            cur_thread->SetSyncedObject(nullptr, ams::svc::ResultTimedOut());

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
            cur_thread->SetAddressArbiter(std::addressof(this->tree), addr);
            this->tree.insert(*cur_thread);
            cur_thread->SetState(KThread::ThreadState_Waiting);
        }

        /* Cancel the timer wait. */
        if (timer != nullptr) {
            timer->CancelTask(cur_thread);
        }

        /* Remove from the address arbiter. */
        {
            KScopedSchedulerLock sl;

            if (cur_thread->IsWaitingForAddressArbiter()) {
                this->tree.erase(this->tree.iterator_to(*cur_thread));
                cur_thread->ClearAddressArbiter();
            }
        }

        /* Get the result. */
        KSynchronizationObject *dummy;
        return cur_thread->GetWaitResult(std::addressof(dummy));
    }

    Result KAddressArbiter::WaitIfEqual(uintptr_t addr, s32 value, s64 timeout) {
        /* Prepare to wait. */
        KThread *cur_thread = GetCurrentThreadPointer();
        KHardwareTimer *timer;

        {
            KScopedSchedulerLockAndSleep slp(std::addressof(timer), cur_thread, timeout);

            /* Check that the thread isn't terminating. */
            if (cur_thread->IsTerminationRequested()) {
                slp.CancelSleep();
                return svc::ResultTerminationRequested();
            }

            /* Set the synced object. */
            cur_thread->SetSyncedObject(nullptr, ams::svc::ResultTimedOut());

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
            cur_thread->SetAddressArbiter(std::addressof(this->tree), addr);
            this->tree.insert(*cur_thread);
            cur_thread->SetState(KThread::ThreadState_Waiting);
        }

        /* Cancel the timer wait. */
        if (timer != nullptr) {
            timer->CancelTask(cur_thread);
        }

        /* Remove from the address arbiter. */
        {
            KScopedSchedulerLock sl;

            if (cur_thread->IsWaitingForAddressArbiter()) {
                this->tree.erase(this->tree.iterator_to(*cur_thread));
                cur_thread->ClearAddressArbiter();
            }
        }

        /* Get the result. */
        KSynchronizationObject *dummy;
        return cur_thread->GetWaitResult(std::addressof(dummy));
    }

}
