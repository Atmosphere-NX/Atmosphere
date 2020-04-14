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
#include <stratosphere.hpp>

namespace ams::os::impl {

    class ReadWriteLockHorizonImpl {
        private:
            using LockCount = os::ReadWriteLockType::LockCount;
        private:
            static ALWAYS_INLINE u32 GetThreadHandle(const LockCount &lc) {
                return GetReference(lc.cs_storage).Get()->thread_handle;
            }

            static ALWAYS_INLINE uintptr_t GetThreadHandleAddress(LockCount &lc) {
                return reinterpret_cast<uintptr_t>(std::addressof(GetReference(lc.cs_storage).Get()->thread_handle));
            }

            static ALWAYS_INLINE void SetThreadHandle(LockCount &lc, u32 handle) {
                GetReference(lc.cs_storage).Get()->thread_handle = handle;
            }

            static void AcquireReadLockWriteLocked(os::ReadWriteLockType *rw_lock);
            static void ReleaseReadLockWriteLocked(os::ReadWriteLockType *rw_lock);
            static void ReleaseWriteLockImpl(os::ReadWriteLockType *rw_lock);
        public:
            static void AcquireReadLock(os::ReadWriteLockType *rw_lock);
            static bool TryAcquireReadLock(os::ReadWriteLockType *rw_lock);
            static void ReleaseReadLock(os::ReadWriteLockType *rw_lock);

            static void AcquireWriteLock(os::ReadWriteLockType *rw_lock);
            static bool TryAcquireWriteLock(os::ReadWriteLockType *rw_lock);
            static void ReleaseWriteLock(os::ReadWriteLockType *rw_lock);
    };

    using ReadWriteLockTargetImpl = ReadWriteLockHorizonImpl;

}
