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

#pragma once
#include <vapours.hpp>
#include <stratosphere/os/os_rw_lock_common.hpp>

namespace ams::os {

    struct ReadWriteLockType;

    void InitalizeReadWriteLock(ReadWriteLockType *rw_lock);
    void FinalizeReadWriteLock(ReadWriteLockType *rw_lock);

    void AcquireReadLock(ReadWriteLockType *rw_lock);
    bool TryAcquireReadLock(ReadWriteLockType *rw_lock);
    void ReleaseReadLock(ReadWriteLockType *rw_lock);

    void AcquireWriteLock(ReadWriteLockType *rw_lock);
    bool TryAcquireWriteLock(ReadWriteLockType *rw_lock);
    void ReleaseWriteLock(ReadWriteLockType *rw_lock);

    bool IsReadLockHeld(const ReadWriteLockType *rw_lock);
    bool IsWriteLockHeldByCurrentThread(const ReadWriteLockType *rw_lock);
    bool IsReadWriteLockOwnerThread(const ReadWriteLockType *rw_lock);

}
