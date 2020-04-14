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
#include "impl/os_rw_lock_impl.hpp"

namespace ams::os {

    void InitalizeReadWriteLock(ReadWriteLockType *rw_lock) {
        /* Create objects. */
        new (GetPointer(impl::GetLockCount(rw_lock).cs_storage)) impl::InternalCriticalSection;
        new (GetPointer(rw_lock->cv_read_lock._storage))          impl::InternalConditionVariable;
        new (GetPointer(rw_lock->cv_write_lock._storage))         impl::InternalConditionVariable;

        /* Set member variables. */
        impl::ClearReadLockCount(impl::GetLockCount(rw_lock));
        impl::ClearWriteLocked(impl::GetLockCount(rw_lock));
        impl::ClearReadLockWaiterCount(impl::GetLockCount(rw_lock));
        impl::ClearWriteLockWaiterCount(impl::GetLockCount(rw_lock));
        impl::ClearWriteLockCount(rw_lock);
        rw_lock->owner_thread = nullptr;

        /* Mark initialized. */
        rw_lock->state = ReadWriteLockType::State_Initialized;
    }

    void FinalizeReadWriteLock(ReadWriteLockType *rw_lock) {
        AMS_ASSERT(rw_lock->state == ReadWriteLockType::State_Initialized);

        /* Don't allow finalizing a locked lock. */
        AMS_ASSERT(impl::GetReadLockCount(impl::GetLockCount(rw_lock)) == 0);
        AMS_ASSERT(impl::GetWriteLocked(impl::GetLockCount(rw_lock))   == 0);

        /* Mark not initialized. */
        rw_lock->state = ReadWriteLockType::State_NotInitialized;

        /* Destroy objects. */
        GetReference(rw_lock->cv_write_lock._storage).~InternalConditionVariable();
        GetReference(rw_lock->cv_read_lock._storage).~InternalConditionVariable();
        GetReference(impl::GetLockCount(rw_lock).cs_storage).~InternalCriticalSection();
    }

    void AcquireReadLock(ReadWriteLockType *rw_lock) {
        AMS_ASSERT(rw_lock->state == ReadWriteLockType::State_Initialized);
        return impl::ReadWriteLockImpl::AcquireReadLock(rw_lock);
    }

    bool TryAcquireReadLock(ReadWriteLockType *rw_lock) {
        AMS_ASSERT(rw_lock->state == ReadWriteLockType::State_Initialized);
        return impl::ReadWriteLockImpl::TryAcquireReadLock(rw_lock);
    }

    void ReleaseReadLock(ReadWriteLockType *rw_lock) {
        AMS_ASSERT(rw_lock->state == ReadWriteLockType::State_Initialized);
        return impl::ReadWriteLockImpl::ReleaseReadLock(rw_lock);
    }

    void AcquireWriteLock(ReadWriteLockType *rw_lock) {
        AMS_ASSERT(rw_lock->state == ReadWriteLockType::State_Initialized);
        return impl::ReadWriteLockImpl::AcquireWriteLock(rw_lock);
    }

    bool TryAcquireWriteLock(ReadWriteLockType *rw_lock) {
        AMS_ASSERT(rw_lock->state == ReadWriteLockType::State_Initialized);
        return impl::ReadWriteLockImpl::TryAcquireWriteLock(rw_lock);
    }

    void ReleaseWriteLock(ReadWriteLockType *rw_lock) {
        AMS_ASSERT(rw_lock->state == ReadWriteLockType::State_Initialized);
        AMS_ABORT_UNLESS(rw_lock->owner_thread == impl::GetCurrentThread());
        return impl::ReadWriteLockImpl::ReleaseWriteLock(rw_lock);
    }

    bool IsReadLockHeld(const ReadWriteLockType *rw_lock) {
        AMS_ASSERT(rw_lock->state == ReadWriteLockType::State_Initialized);
        return impl::GetReadLockCount(impl::GetLockCount(rw_lock)) != 0;
    }

    bool IsWriteLockHeldByCurrentThread(const ReadWriteLockType *rw_lock) {
        AMS_ASSERT(rw_lock->state == ReadWriteLockType::State_Initialized);
        return rw_lock->owner_thread == impl::GetCurrentThread() && impl::GetWriteLockCount(*rw_lock) != 0;
    }

    bool IsReadWriteLockOwnerThread(const ReadWriteLockType *rw_lock) {
        AMS_ASSERT(rw_lock->state == ReadWriteLockType::State_Initialized);
        return rw_lock->owner_thread == impl::GetCurrentThread();
    }

}
