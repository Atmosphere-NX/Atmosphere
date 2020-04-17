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

namespace ams::os {

    void InitializeSdkMutex(SdkMutexType *mutex) {
        GetReference(mutex->_storage).Initialize();
    }

    bool IsSdkMutexLockedByCurrentThread(const SdkMutexType *mutex) {
        return GetReference(mutex->_storage).IsLockedByCurrentThread();
    }

    void LockSdkMutex(SdkMutexType *mutex) {
        AMS_ABORT_UNLESS(!IsSdkMutexLockedByCurrentThread(mutex));
        return GetReference(mutex->_storage).Enter();
    }

    bool TryLockSdkMutex(SdkMutexType *mutex) {
        AMS_ABORT_UNLESS(!IsSdkMutexLockedByCurrentThread(mutex));
        return GetReference(mutex->_storage).TryEnter();
    }

    void UnlockSdkMutex(SdkMutexType *mutex) {
        AMS_ABORT_UNLESS(IsSdkMutexLockedByCurrentThread(mutex));
        return GetReference(mutex->_storage).Leave();
    }

}
