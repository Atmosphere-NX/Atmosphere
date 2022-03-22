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
    #include "impl/os_internal_rw_busy_mutex_impl.os.horizon.hpp"
#else
    #error "Unknown OS for ams::os::impl::InternalReaderWriterBusyMutexImpl"
#endif

namespace ams::os {

    void InitalizeReaderWriterLockBusyMutex(ReaderWriterBusyMutexType *rw_mutex) {
        /* Create object. */
        util::ConstructAt(rw_mutex->_storage);
    }

    void AcquireReadLockBusyMutex(ReaderWriterBusyMutexType *rw_mutex) {
        /* Acquire read lock. */
        util::GetReference(rw_mutex->_storage).AcquireReadLock();
    }

    void ReleaseReadLockBusyMutex(ReaderWriterBusyMutexType *rw_mutex) {
        /* Release read lock. */
        util::GetReference(rw_mutex->_storage).ReleaseReadLock();
    }

    void AcquireWriteLockBusyMutex(ReaderWriterBusyMutexType *rw_mutex) {
        /* Acquire write lock. */
        util::GetReference(rw_mutex->_storage).AcquireWriteLock();
    }

    void ReleaseWriteLockBusyMutex(ReaderWriterBusyMutexType *rw_mutex) {
        /* Release write lock. */
        util::GetReference(rw_mutex->_storage).ReleaseWriteLock();
    }

}
