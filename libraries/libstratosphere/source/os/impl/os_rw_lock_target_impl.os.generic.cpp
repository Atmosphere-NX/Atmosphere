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
#include "os_rw_lock_impl.hpp"
#include "os_thread_manager.hpp"

namespace ams::os::impl {

    void ReaderWriterLockHorizonImpl::AcquireReadLock(os::ReaderWriterLockType *rw_lock) {
        /* Acquire exclusive access. */
        std::scoped_lock lk(util::GetReference(GetLockCount(rw_lock).cs_storage));

        /* Get the lock into a state where we can safely increment the read lock count. */
        if (rw_lock->owner_thread == impl::GetCurrentThread()) {
            /* If we're the owner thread, we should hold the write lock. */
            AMS_ASSERT(GetWriteLocked(GetLockCount(rw_lock)) == 1);
        } else {
            /* Wait until we're not write locked, and there are no write lock waiters. */
            while (true) {
                /* Check if we're write locked. */
                if (GetWriteLocked(GetLockCount(rw_lock)) != 1) {
                    /* We're not, so check if we have write lock waiters. */
                    if (GetWriteLockWaiterCount(GetLockCount(rw_lock)) == 0) {
                        break;
                    }
                }

                /* We're write locked, or we have write lock waiters. */
                IncReadLockWaiterCount(GetLockCount(rw_lock));
                util::GetReference(rw_lock->cv_read_lock._storage).Wait(util::GetPointer(GetLockCount(rw_lock).cs_storage));
                DecReadLockWaiterCount(GetLockCount(rw_lock));
            }

            /* Verify we're in the desired state. */
            AMS_ASSERT(GetWriteLockCount(*rw_lock) == 0);
            AMS_ASSERT(rw_lock->owner_thread == nullptr);
        }

        /* Increment the read lock count. */
        IncReadLockCount(GetLockCount(rw_lock));
    }

    bool ReaderWriterLockHorizonImpl::TryAcquireReadLock(os::ReaderWriterLockType *rw_lock) {
        /* Acquire exclusive access. */
        std::scoped_lock lk(util::GetReference(GetLockCount(rw_lock).cs_storage));

        /* Try to get the lock into a state where we can safely increment the read lock count. */
        if (rw_lock->owner_thread == impl::GetCurrentThread()) {
            /* If we're the owner thread, we should hold the write lock. */
            AMS_ASSERT(GetWriteLocked(GetLockCount(rw_lock)) == 1);
        } else {
            /* We can only read lock if we're not write locked, and there are no write lock waiters. */
            if (GetWriteLocked(GetLockCount(rw_lock)) == 1) {
                return false;
            }
            if (GetWriteLockWaiterCount(GetLockCount(rw_lock)) != 0) {
                return false;
            }

            /* Verify we're in the desired state. */
            AMS_ASSERT(GetWriteLockCount(*rw_lock) == 0);
            AMS_ASSERT(rw_lock->owner_thread == nullptr);
        }

        /* Increment the read lock count. */
        IncReadLockCount(GetLockCount(rw_lock));
        return true;
    }

    void ReaderWriterLockHorizonImpl::ReleaseReadLock(os::ReaderWriterLockType *rw_lock) {
        /* Acquire exclusive access. */
        std::scoped_lock lk(util::GetReference(GetLockCount(rw_lock).cs_storage));

        /* Sanity check that we hold the read lock. */
        AMS_ASSERT(GetReadLockCount(GetLockCount(rw_lock)) > 0);

        /* Decrement the read lock count. */
        DecReadLockCount(GetLockCount(rw_lock));

        /* If we're the owner of the write lock, we may need to check if this causes a full release/signal to waiters. */
        if (rw_lock->owner_thread == impl::GetCurrentThread()) {
            /* Sanity check that we're write locked. */
            AMS_ASSERT(GetWriteLocked(GetLockCount(rw_lock)) == 1);

            /* If we've called ReleaseWriteLock and have no remaining read locks, we may need to signal/broadcast. */
            if (GetWriteLockCount(*rw_lock) == 0 && GetReadLockCount(GetLockCount(rw_lock)) == 0) {
                /* Clear lock owner. */
                rw_lock->owner_thread = nullptr;

                /* Clear write locked. */
                ClearWriteLocked(GetLockCount(rw_lock));

                /* If we have lock waiters, signal them. */
                if (GetWriteLockWaiterCount(GetLockCount(rw_lock)) != 0) {
                    util::GetReference(rw_lock->cv_write_lock._storage).Signal();
                } else if (GetReadLockWaiterCount(GetLockCount(rw_lock)) != 0) {
                    util::GetReference(rw_lock->cv_read_lock._storage).Broadcast();
                }
            }
        } else {
            /* Sanity check that our read lock release was fine. */
            AMS_ASSERT(GetWriteLockCount(*rw_lock) == 0);
            AMS_ASSERT(GetWriteLocked(GetLockCount(rw_lock)) == 0);
            AMS_ASSERT(rw_lock->owner_thread == nullptr);

            /* We need to signal if there are no remaining read locks. */
            if (GetReadLockCount(GetLockCount(rw_lock)) == 0) {
                if (GetWriteLockWaiterCount(GetLockCount(rw_lock)) != 0) {
                    util::GetReference(rw_lock->cv_write_lock._storage).Signal();
                }
            }
        }
    }

    void ReaderWriterLockHorizonImpl::AcquireWriteLock(os::ReaderWriterLockType *rw_lock) {
        /* Acquire exclusive access. */
        std::scoped_lock lk(util::GetReference(GetLockCount(rw_lock).cs_storage));

        /* Get the lock into a state where we can safely increment the read lock count. */
        if (rw_lock->owner_thread == impl::GetCurrentThread()) {
            /* If we're the owner thread, we should hold the write lock. */
            AMS_ASSERT(GetWriteLocked(GetLockCount(rw_lock)) == 1);

            /* Increment the write lock count. */
            IncWriteLockCount(*rw_lock);
        } else {
            /* Wait until we're not read locked and not write locked. */
            while (true) {
                /* Check if we're read locked. */
                if (GetReadLockCount(GetLockCount(rw_lock)) == 0) {
                    /* We're not, so check if we're write locked. */
                    if (GetWriteLocked(GetLockCount(rw_lock)) != 1) {
                        break;
                    }
                }

                /* We're write locked or read locked. */
                IncWriteLockWaiterCount(GetLockCount(rw_lock));
                util::GetReference(rw_lock->cv_write_lock._storage).Wait(util::GetPointer(GetLockCount(rw_lock).cs_storage));
                DecWriteLockWaiterCount(GetLockCount(rw_lock));
            }

            /* Verify we're in the desired state. */
            AMS_ASSERT(GetWriteLockCount(*rw_lock) == 0);
            AMS_ASSERT(rw_lock->owner_thread == nullptr);

            /* Increment the write lock count. */
            IncWriteLockCount(*rw_lock);

            /* Set write locked. */
            SetWriteLocked(GetLockCount(rw_lock));

            /* Set ourselves as the owner. */
            rw_lock->owner_thread = impl::GetCurrentThread();
        }
    }

    bool ReaderWriterLockHorizonImpl::TryAcquireWriteLock(os::ReaderWriterLockType *rw_lock) {
        /* Acquire exclusive access. */
        std::scoped_lock lk(util::GetReference(GetLockCount(rw_lock).cs_storage));

        /* Get the lock into a state where we can safely increment the read lock count. */
        if (rw_lock->owner_thread == impl::GetCurrentThread()) {
            /* If we're the owner thread, we should hold the write lock. */
            AMS_ASSERT(GetWriteLocked(GetLockCount(rw_lock)) == 1);

            /* Increment the write lock count. */
            IncWriteLockCount(*rw_lock);
        } else {
            /* Check if we're read locked. */
            if (GetReadLockCount(GetLockCount(rw_lock)) != 0) {
                return false;
            }
            /* We're not, so check if we're write locked. */
            if (GetWriteLocked(GetLockCount(rw_lock)) == 1) {
                return false;
            }

            /* Verify we're in the desired state. */
            AMS_ASSERT(GetWriteLockCount(*rw_lock) == 0);
            AMS_ASSERT(rw_lock->owner_thread == nullptr);

            /* Increment the write lock count. */
            IncWriteLockCount(*rw_lock);

            /* Set write locked. */
            SetWriteLocked(GetLockCount(rw_lock));

            /* Set ourselves as the owner. */
            rw_lock->owner_thread = impl::GetCurrentThread();
        }

        return true;
    }

    void ReaderWriterLockHorizonImpl::ReleaseWriteLock(os::ReaderWriterLockType *rw_lock) {
        /* Acquire exclusive access. */
        std::scoped_lock lk(util::GetReference(GetLockCount(rw_lock).cs_storage));

        /* Sanity check our state. */
        AMS_ASSERT(GetWriteLockCount(*rw_lock) > 0);
        AMS_ASSERT(GetWriteLocked(GetLockCount(rw_lock)) != 0);
        AMS_ASSERT(rw_lock->owner_thread == impl::GetCurrentThread());

        /* Decrement the write lock count. */
        DecWriteLockCount(*rw_lock);

        /* If we have no remaining write locks, we may need to signal/release. */
        if (GetWriteLockCount(*rw_lock) == 0 && GetReadLockCount(GetLockCount(rw_lock)) == 0) {
            /* Clear lock owner. */
            rw_lock->owner_thread = nullptr;

            /* Clear write locked. */
            ClearWriteLocked(GetLockCount(rw_lock));

            /* If we have lock waiters, signal them. */
            if (GetWriteLockWaiterCount(GetLockCount(rw_lock)) != 0) {
                util::GetReference(rw_lock->cv_write_lock._storage).Signal();
            } else if (GetReadLockWaiterCount(GetLockCount(rw_lock)) != 0) {
                util::GetReference(rw_lock->cv_read_lock._storage).Broadcast();
            }
        }
    }

}
