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
#if !defined(AMS_OS_INTERNAL_CRITICAL_SECTION_IMPL_CAN_CHECK_LOCKED_BY_CURRENT_THREAD)
#include "impl/os_thread_manager.hpp"
#endif

namespace ams::os {

    void InitializeSdkRecursiveMutex(SdkRecursiveMutexType *rmutex) {
        /* Initialize the critical section. */
        GetReference(rmutex->_storage).Initialize();

        /* Set recursive count. */
        rmutex->recursive_count = 0;

        /* If the underlying critical section can't readily be checked, set owner thread. */
        #if !defined(AMS_OS_INTERNAL_CRITICAL_SECTION_IMPL_CAN_CHECK_LOCKED_BY_CURRENT_THREAD)
        rmutex->owner_thread = nullptr;
        #endif
    }

    bool IsSdkRecursiveMutexLockedByCurrentThread(const SdkRecursiveMutexType *rmutex) {
        #if defined(AMS_OS_INTERNAL_CRITICAL_SECTION_IMPL_CAN_CHECK_LOCKED_BY_CURRENT_THREAD)
        /* Check whether the critical section is held. */
        return GetReference(rmutex->_storage).IsLockedByCurrentThread();
        #else
        /* Check if the current thread is owner. */
        return rmutex->owner_thread == os::impl::GetCurrentThread();
        #endif
    }

    void LockSdkRecursiveMutex(SdkRecursiveMutexType *rmutex) {
        /* If we don't hold the mutex, enter the critical section. */
        if (!IsSdkRecursiveMutexLockedByCurrentThread(rmutex)) {
            GetReference(rmutex->_storage).Enter();

            /* If necessary, set owner thread. */
            #if !defined(AMS_OS_INTERNAL_CRITICAL_SECTION_IMPL_CAN_CHECK_LOCKED_BY_CURRENT_THREAD)
            rmutex->owner_thread = os::impl::GetCurrentThread();
            #endif
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

            /* If necessary, set owner thread. */
            #if !defined(AMS_OS_INTERNAL_CRITICAL_SECTION_IMPL_CAN_CHECK_LOCKED_BY_CURRENT_THREAD)
            rmutex->owner_thread = os::impl::GetCurrentThread();
            #endif
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
            /* If necessary, clear owner thread. */
            #if !defined(AMS_OS_INTERNAL_CRITICAL_SECTION_IMPL_CAN_CHECK_LOCKED_BY_CURRENT_THREAD)
            rmutex->owner_thread = nullptr;
            #endif

            GetReference(rmutex->_storage).Leave();
        }
    }

}
