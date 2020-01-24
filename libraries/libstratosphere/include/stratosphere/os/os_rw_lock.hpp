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
#include "os_common_types.hpp"

namespace ams::os {

    class ReadWriteLock {
        NON_COPYABLE(ReadWriteLock);
        NON_MOVEABLE(ReadWriteLock);
        private:
            ::RwLock r;
        public:
            ReadWriteLock() {
                rwlockInit(&this->r);
            }

            bool IsWriteLockHeldByCurrentThread() const {
                return rwlockIsWriteLockHeldByCurrentThread(const_cast<::RwLock *>(&this->r));
            }

            bool IsLockOwner() const {
                return rwlockIsOwnedByCurrentThread(const_cast<::RwLock *>(&this->r));
            }

            void AcquireReadLock() {
                rwlockReadLock(&this->r);
            }

            void ReleaseReadLock() {
                rwlockReadUnlock(&this->r);
            }

            bool TryAcquireReadLock() {
                return rwlockTryReadLock(&this->r);
            }

            void AcquireWriteLock() {
                rwlockWriteLock(&this->r);
            }

            void ReleaseWriteLock() {
                rwlockWriteUnlock(&this->r);
            }

            bool TryAcquireWriteLock() {
                return rwlockTryWriteLock(&this->r);
            }

            void lock_shared() {
                this->AcquireReadLock();
            }

            void unlock_shared() {
                this->ReleaseReadLock();
            }

            bool try_lock_shared() {
                return this->TryAcquireReadLock();
            }

            void lock() {
                this->AcquireWriteLock();
            }

            void unlock() {
                this->ReleaseWriteLock();
            }

            bool try_lock() {
                return this->TryAcquireWriteLock();
            }
    };

}