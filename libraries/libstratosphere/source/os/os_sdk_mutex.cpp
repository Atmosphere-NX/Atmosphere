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

    void InitializeSdkMutex(SdkMutexType *mutex) {
        /* Initialize the critical section. */
        GetReference(mutex->_storage).Initialize();

        /* If the underlying critical section can't readily be checked, set owner thread. */
        #if !defined(AMS_OS_INTERNAL_CRITICAL_SECTION_IMPL_CAN_CHECK_LOCKED_BY_CURRENT_THREAD)
        mutex->owner_thread = nullptr;
        #endif
    }

    bool IsSdkMutexLockedByCurrentThread(const SdkMutexType *mutex) {
        #if defined(AMS_OS_INTERNAL_CRITICAL_SECTION_IMPL_CAN_CHECK_LOCKED_BY_CURRENT_THREAD)
        /* Check whether the critical section is held. */
        return GetReference(mutex->_storage).IsLockedByCurrentThread();
        #else
        /* Check if the current thread is owner. */
        return mutex->owner_thread == os::impl::GetCurrentThread();
        #endif
    }

    void LockSdkMutex(SdkMutexType *mutex) {
        /* Check pre-conditions. */
        AMS_ABORT_UNLESS(!IsSdkMutexLockedByCurrentThread(mutex));

        /* Enter the critical section. */
        GetReference(mutex->_storage).Enter();

        /* If necessary, set owner thread. */
        #if !defined(AMS_OS_INTERNAL_CRITICAL_SECTION_IMPL_CAN_CHECK_LOCKED_BY_CURRENT_THREAD)
        mutex->owner_thread = os::impl::GetCurrentThread();
        #endif
    }

    bool TryLockSdkMutex(SdkMutexType *mutex) {
        /* Check pre-conditions. */
        AMS_ABORT_UNLESS(!IsSdkMutexLockedByCurrentThread(mutex));

        /* Try to enter the critical section. */
        const bool res = GetReference(mutex->_storage).TryEnter();

        /* If necessary, set owner thread. */
        #if !defined(AMS_OS_INTERNAL_CRITICAL_SECTION_IMPL_CAN_CHECK_LOCKED_BY_CURRENT_THREAD)
        if (res) {
            mutex->owner_thread = os::impl::GetCurrentThread();
        }
        #endif

        return res;
    }

    void UnlockSdkMutex(SdkMutexType *mutex) {
        /* Check pre-conditions. */
        AMS_ABORT_UNLESS(IsSdkMutexLockedByCurrentThread(mutex));

        /* If necessary, clear owner thread. */
        #if !defined(AMS_OS_INTERNAL_CRITICAL_SECTION_IMPL_CAN_CHECK_LOCKED_BY_CURRENT_THREAD)
        mutex->owner_thread = nullptr;
        #endif

        /* Leave the critical section. */
        GetReference(mutex->_storage).Leave();
    }

}
