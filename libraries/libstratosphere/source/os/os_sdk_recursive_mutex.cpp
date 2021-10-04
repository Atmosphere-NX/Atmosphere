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

namespace ams::os {

    void InitializeSdkRecursiveMutex(SdkRecursiveMutexType *rmutex) {
        /* Initialize the critical section. */
        GetReference(rmutex->_storage).Initialize();

        /* Set recursive count. */
        rmutex->recursive_count = 0;
    }

    bool IsSdkRecursiveMutexLockedByCurrentThread(const SdkRecursiveMutexType *rmutex) {
        /* Check whether the critical section is held. */
        return GetReference(rmutex->_storage).IsLockedByCurrentThread();
    }

    void LockSdkRecursiveMutex(SdkRecursiveMutexType *rmutex) {
        /* If we don't hold the mutex, enter the critical section. */
        if (!IsSdkRecursiveMutexLockedByCurrentThread(rmutex)) {
            GetReference(rmutex->_storage).Enter();
        }

        /* Increment (and check) recursive count. */
        ++rmutex->recursive_count;
        AMS_ABORT_UNLESS(rmutex->recursive_count != 0);
    }

    bool TryLockSdkRecursiveMutex(SdkRecursiveMutexType *rmutex) {
        /* If we don't hold the mutex, try to enter the critical section. */
        if (!IsSdkRecursiveMutexLockedByCurrentThread(rmutex)) {
            if (!GetReference(rmutex->_storage).TryEnter()) {
                return false;
            }
        }

        /* Increment (and check) recursive count. */
        ++rmutex->recursive_count;
        AMS_ABORT_UNLESS(rmutex->recursive_count != 0);

        return true;
    }

    void UnlockSdkRecursiveMutex(SdkRecursiveMutexType *rmutex) {
        /* Check pre-conditions. */
        AMS_ABORT_UNLESS(IsSdkRecursiveMutexLockedByCurrentThread(rmutex));

        /* Decrement recursive count, and leave critical section if we no longer hold the mutex. */
        if ((--rmutex->recursive_count) == 0) {
            GetReference(rmutex->_storage).Leave();
        }
    }

}
