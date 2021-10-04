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

#if defined(ATMOSPHERE_OS_HORIZON)
    #include "impl/os_internal_busy_mutex_impl.os.horizon.hpp"
#else
    #error "Unknown OS for ams::os::impl::InternalBusyMutexImpl"
#endif

namespace ams::os {

    void InitializeBusyMutex(BusyMutexType *mutex) {
        /* Create object. */
        util::ConstructAt(mutex->_storage);

        /* Set member variables. */
        mutex->owner_thread = nullptr;

        /* Mark initialized. */
        mutex->state = BusyMutexType::State_Initialized;
    }

    void FinalizeBusyMutex(BusyMutexType *mutex) {
        /* Check pre-conditions. */
        AMS_ASSERT(mutex->state == BusyMutexType::State_Initialized);
        AMS_ASSERT(!util::GetReference(mutex->_storage).IsLocked());

        /* Mark not intialized. */
        mutex->state = MutexType::State_NotInitialized;

        /* Destroy object. */
        util::DestroyAt(mutex->_storage);
    }

    void LockBusyMutex(BusyMutexType *mutex) {
        /* Check pre-conditions. */
        AMS_ASSERT(mutex->state == BusyMutexType::State_Initialized);

        /* Lock mutex. */
        util::GetReference(mutex->_storage).Lock();

        /* Set owner thread. */
        mutex->owner_thread = impl::GetCurrentThread();
    }

    bool TryLockBusyMutex(BusyMutexType *mutex) {
        /* Check pre-conditions. */
        AMS_ASSERT(mutex->state == BusyMutexType::State_Initialized);

        /* Try to lock mutex. */
        const bool locked = util::GetReference(mutex->_storage).TryLock();

        /* Set owner thread. */
        if (locked) {
            mutex->owner_thread = impl::GetCurrentThread();
        }

        return locked;
    }

    void UnlockBusyMutex(BusyMutexType *mutex) {
        /* Check pre-conditions. */
        AMS_ASSERT(mutex->state == BusyMutexType::State_Initialized);
        AMS_ASSERT(util::GetReference(mutex->_storage).IsLocked() && mutex->owner_thread == impl::GetCurrentThread());

        /* Unlock. */
        util::GetReference(mutex->_storage).Unlock();
    }

}
