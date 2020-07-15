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

        constinit KThread g_arbiter_compare_thread;

        ALWAYS_INLINE bool ReadFromUser(s32 *out, KProcessAddress address) {
            return UserspaceAccess::CopyMemoryFromUserSize32Bit(out, GetVoidPointer(address));
        }

    }

    Result KAddressArbiter::Signal(uintptr_t addr, s32 count) {
        MESOSPHERE_UNIMPLEMENTED();
    }

    Result KAddressArbiter::SignalAndIncrementIfEqual(uintptr_t addr, s32 value, s32 count) {
        MESOSPHERE_UNIMPLEMENTED();
    }

    Result KAddressArbiter::SignalAndModifyByWaitingCountIfEqual(uintptr_t addr, s32 value, s32 count) {
        MESOSPHERE_UNIMPLEMENTED();
    }

    Result KAddressArbiter::WaitIfLessThan(uintptr_t addr, s32 value, bool decrement, s64 timeout) {
        MESOSPHERE_UNIMPLEMENTED();
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
