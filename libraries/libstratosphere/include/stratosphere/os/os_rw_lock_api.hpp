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

#pragma once
#include <vapours.hpp>
#include <stratosphere/os/os_rw_lock_common.hpp>

namespace ams::os {

    struct ReaderWriterLockType;

    void InitalizeReaderWriterLock(ReaderWriterLockType *rw_lock);
    void FinalizeReaderWriterLock(ReaderWriterLockType *rw_lock);

    void AcquireReadLock(ReaderWriterLockType *rw_lock);
    bool TryAcquireReadLock(ReaderWriterLockType *rw_lock);
    void ReleaseReadLock(ReaderWriterLockType *rw_lock);

    void AcquireWriteLock(ReaderWriterLockType *rw_lock);
    bool TryAcquireWriteLock(ReaderWriterLockType *rw_lock);
    void ReleaseWriteLock(ReaderWriterLockType *rw_lock);

    bool IsReadLockHeld(const ReaderWriterLockType *rw_lock);
    bool IsWriteLockHeldByCurrentThread(const ReaderWriterLockType *rw_lock);
    bool IsReaderWriterLockOwnerThread(const ReaderWriterLockType *rw_lock);

}
