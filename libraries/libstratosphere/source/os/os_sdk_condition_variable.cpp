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

}
