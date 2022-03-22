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
#include "impl/os_rw_lock_impl.hpp"

namespace ams::os {

    void InitalizeReaderWriterLock(ReaderWriterLockType *rw_lock) {
        /* Create objects. */
        util::ConstructAt(impl::GetLockCount(rw_lock).cs_storage);
        util::ConstructAt(rw_lock->cv_read_lock._storage);
        util::ConstructAt(rw_lock->cv_write_lock._storage);

        /* Set member variables. */
        impl::ClearReadLockCount(impl::GetLockCount(rw_lock));
        impl::ClearWriteLocked(impl::GetLockCount(rw_lock));
        impl::ClearReadLockWaiterCount(impl::GetLockCount(rw_lock));
        impl::ClearWriteLockWaiterCount(impl::GetLockCount(rw_lock));
        impl::ClearWriteLockCount(rw_lock);
        rw_lock->owner_thread = nullptr;

        /* Mark initialized. */
        rw_lock->state = ReaderWriterLockType::State_Initialized;
    }

    void FinalizeReaderWriterLock(ReaderWriterLockType *rw_lock) {
        AMS_ASSERT(rw_lock->state == ReaderWriterLockType::State_Initialized);

        /* Don't allow finalizing a locked lock. */
        AMS_ASSERT(impl::GetReadLockCount(impl::GetLockCount(rw_lock)) == 0);
        AMS_ASSERT(impl::GetWriteLocked(impl::GetLockCount(rw_lock))   == 0);

        /* Mark not initialized. */
        rw_lock->state = ReaderWriterLockType::State_NotInitialized;

        /* Destroy objects. */
        util::DestroyAt(rw_lock->cv_write_lock._storage);
        util::DestroyAt(rw_lock->cv_read_lock._storage);
        util::DestroyAt(impl::GetLockCount(rw_lock).cs_storage);
    }

    void AcquireReadLock(ReaderWriterLockType *rw_lock) {
        AMS_ASSERT(rw_lock->state == ReaderWriterLockType::State_Initialized);
        return impl::ReaderWriterLockImpl::AcquireReadLock(rw_lock);
    }

    bool TryAcquireReadLock(ReaderWriterLockType *rw_lock) {
        AMS_ASSERT(rw_lock->state == ReaderWriterLockType::State_Initialized);
        return impl::ReaderWriterLockImpl::TryAcquireReadLock(rw_lock);
    }

    void ReleaseReadLock(ReaderWriterLockType *rw_lock) {
        AMS_ASSERT(rw_lock->state == ReaderWriterLockType::State_Initialized);
        return impl::ReaderWriterLockImpl::ReleaseReadLock(rw_lock);
    }

    void AcquireWriteLock(ReaderWriterLockType *rw_lock) {
        AMS_ASSERT(rw_lock->state == ReaderWriterLockType::State_Initialized);
        return impl::ReaderWriterLockImpl::AcquireWriteLock(rw_lock);
    }

    bool TryAcquireWriteLock(ReaderWriterLockType *rw_lock) {
        AMS_ASSERT(rw_lock->state == ReaderWriterLockType::State_Initialized);
        return impl::ReaderWriterLockImpl::TryAcquireWriteLock(rw_lock);
    }

    void ReleaseWriteLock(ReaderWriterLockType *rw_lock) {
        AMS_ASSERT(rw_lock->state == ReaderWriterLockType::State_Initialized);
        AMS_ABORT_UNLESS(rw_lock->owner_thread == impl::GetCurrentThread());
        return impl::ReaderWriterLockImpl::ReleaseWriteLock(rw_lock);
    }

    bool IsReadLockHeld(const ReaderWriterLockType *rw_lock) {
        AMS_ASSERT(rw_lock->state == ReaderWriterLockType::State_Initialized);
        return impl::GetReadLockCount(impl::GetLockCount(rw_lock)) != 0;
    }

    bool IsWriteLockHeldByCurrentThread(const ReaderWriterLockType *rw_lock) {
        AMS_ASSERT(rw_lock->state == ReaderWriterLockType::State_Initialized);
        return rw_lock->owner_thread == impl::GetCurrentThread() && impl::GetWriteLockCount(*rw_lock) != 0;
    }

    bool IsReaderWriterLockOwnerThread(const ReaderWriterLockType *rw_lock) {
        AMS_ASSERT(rw_lock->state == ReaderWriterLockType::State_Initialized);
        return rw_lock->owner_thread == impl::GetCurrentThread();
    }

}
