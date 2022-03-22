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
#include <stratosphere/os/os_mutex_common.hpp>

namespace ams::os {

    struct MutexType;

    void InitializeMutex(MutexType *mutex, bool recursive, int lock_level);
    void FinalizeMutex(MutexType *mutex);

    void LockMutex(MutexType *mutex);
    bool TryLockMutex(MutexType *mutex);
    void UnlockMutex(MutexType *mutex);

    bool IsMutexLockedByCurrentThread(const MutexType *mutex);

}
