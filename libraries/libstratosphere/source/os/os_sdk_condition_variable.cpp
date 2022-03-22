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
#include <stratosphere.hpp>
#include "impl/os_thread_manager.hpp"
#include "impl/os_timeout_helper.hpp"

namespace ams::os {

    void SdkConditionVariableType::Wait(SdkMutexType &mutex) {
        /* Check that we own the mutex. */
        AMS_ABORT_UNLESS(os::IsSdkMutexLockedByCurrentThread(std::addressof(mutex)));

        /* Wait on the mutex. */
        GetReference(this->_storage).Wait(GetPointer(mutex._storage));
    }

    bool SdkConditionVariableType::TimedWait(SdkMutexType &mutex, TimeSpan timeout) {
        /* Check preconditions. */
        AMS_ASSERT(timeout.GetNanoSeconds() >= 0);
        AMS_ABORT_UNLESS(os::IsSdkMutexLockedByCurrentThread(std::addressof(mutex)));

        /* Handle zero timeout by unlocking and re-locking. */
        if (timeout == TimeSpan(0)) {
            GetReference(mutex._storage).Leave();
            GetReference(mutex._storage).Enter();
            return false;
        }

        /* Handle timed wait. */
        impl::TimeoutHelper timeout_helper(timeout);
        auto status = GetReference(this->_storage).TimedWait(GetPointer(mutex._storage), timeout_helper);
        return status == ConditionVariableStatus::Success;
    }

    void SdkConditionVariableType::Wait(SdkRecursiveMutexType &mutex) {
        /* Check preconditions. */
        AMS_ABORT_UNLESS(os::IsSdkRecursiveMutexLockedByCurrentThread(std::addressof(mutex)));
        AMS_ABORT_UNLESS(mutex.recursive_count == 1);

        /* Decrement the mutex's recursive count. */
        --mutex.recursive_count;

        /* Wait on the mutex. */
        GetReference(this->_storage).Wait(GetPointer(mutex._storage));

        /* Increment the mutex's recursive count. */
        ++mutex.recursive_count;

        /* Check that the mutex's recursive count is valid. */
        AMS_ABORT_UNLESS(mutex.recursive_count != 0);
    }

    bool SdkConditionVariableType::TimedWait(SdkRecursiveMutexType &mutex, TimeSpan timeout) {
        /* Check preconditions. */
        AMS_ASSERT(timeout.GetNanoSeconds() >= 0);
        AMS_ABORT_UNLESS(os::IsSdkRecursiveMutexLockedByCurrentThread(std::addressof(mutex)));

        /* Handle zero timeout by unlocking and re-locking. */
        if (timeout == TimeSpan(0)) {
            /* NOTE: Nintendo doesn't check recursive_count here...seems really suspicious? */
            /*       Not sure that this is correct, or if they just forgot to check. */
            GetReference(mutex._storage).Leave();
            GetReference(mutex._storage).Enter();
            return false;
        }

        /* Check that the mutex is held exactly once. */
        AMS_ABORT_UNLESS(mutex.recursive_count == 1);

        /* Decrement the mutex's recursive count. */
        --mutex.recursive_count;

        /* Create timeout helper. */
        impl::TimeoutHelper timeout_helper(timeout);

        /* Perform timed wait. */
        auto status = GetReference(this->_storage).TimedWait(GetPointer(mutex._storage), timeout_helper);

        /* Increment the mutex's recursive count. */
        ++mutex.recursive_count;

        /* Check that the mutex's recursive count is valid. */
        AMS_ABORT_UNLESS(mutex.recursive_count != 0);

        /* Return whether we succeeded. */
        return status == ConditionVariableStatus::Success;
    }

}
