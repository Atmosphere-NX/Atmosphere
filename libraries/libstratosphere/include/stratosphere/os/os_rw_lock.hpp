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
#include <stratosphere/os/os_rw_lock_common.hpp>
#include <stratosphere/os/os_rw_lock_types.hpp>
#include <stratosphere/os/os_rw_lock_api.hpp>

namespace ams::os {

    class ReaderWriterLock {
        NON_COPYABLE(ReaderWriterLock);
        NON_MOVEABLE(ReaderWriterLock);
        private:
            ReaderWriterLockType rw_lock;
        public:
            constexpr explicit ReaderWriterLock() : rw_lock{{}, 0, ::ams::os::ReaderWriterLockType::State_Initialized, nullptr, 0, {}, {}} { /* ... */ }

            ~ReaderWriterLock() { os::FinalizeReaderWriterLock(std::addressof(this->rw_lock)); }

            void AcquireReadLock() {
                return os::AcquireReadLock(std::addressof(this->rw_lock));
            }

            bool TryAcquireReadLock() {
                return os::TryAcquireReadLock(std::addressof(this->rw_lock));
            }

            void ReleaseReadLock() {
                return os::ReleaseReadLock(std::addressof(this->rw_lock));
            }

            void AcquireWriteLock() {
                return os::AcquireWriteLock(std::addressof(this->rw_lock));
            }

            bool TryAcquireWriteLock() {
                return os::TryAcquireWriteLock(std::addressof(this->rw_lock));
            }

            void ReleaseWriteLock() {
                return os::ReleaseWriteLock(std::addressof(this->rw_lock));
            }

            bool IsReadLockHeld() const {
                return os::IsReadLockHeld(std::addressof(this->rw_lock));
            }

            bool IsWriteLockHeldByCurrentThread() const {
                return os::IsWriteLockHeldByCurrentThread(std::addressof(this->rw_lock));
            }

            bool IsLockOwner() const {
                return os::IsReaderWriterLockOwnerThread(std::addressof(this->rw_lock));
            }

            void lock_shared() {
                return this->AcquireReadLock();
            }

            bool try_lock_shared() {
                return this->TryAcquireReadLock();
            }

            void unlock_shared() {
                return this->ReleaseReadLock();
            }

            void lock() {
                return this->AcquireWriteLock();
            }

            bool try_lock() {
                return this->TryAcquireWriteLock();
            }

            void unlock() {
                return this->ReleaseWriteLock();
            }

            operator ReaderWriterLockType &() {
                return this->rw_lock;
            }

            operator const ReaderWriterLockType &() const {
                return this->rw_lock;
            }

            ReaderWriterLockType *GetBase() {
                return std::addressof(this->rw_lock);
            }
    };

}