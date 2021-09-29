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
#include <stratosphere/os/os_rw_busy_mutex_types.hpp>
#include <stratosphere/os/os_rw_busy_mutex_api.hpp>

namespace ams::os {

    class ReadWriteBusyMutex {
        NON_COPYABLE(ReadWriteBusyMutex);
        NON_MOVEABLE(ReadWriteBusyMutex);
        private:
            ReadWriteBusyMutexType m_rw_mutex;
        public:
            constexpr explicit ReadWriteBusyMutex() : m_rw_mutex{{0}} { /* ... */ }

            void AcquireReadLock() {
                return os::AcquireReadLockBusyMutex(std::addressof(m_rw_mutex));
            }

            void ReleaseReadLock() {
                return os::ReleaseReadLockBusyMutex(std::addressof(m_rw_mutex));
            }

            void AcquireWriteLock() {
                return os::AcquireWriteLockBusyMutex(std::addressof(m_rw_mutex));
            }

            void ReleaseWriteLock() {
                return os::ReleaseWriteLockBusyMutex(std::addressof(m_rw_mutex));
            }

            void lock_shared() {
                return this->AcquireReadLock();
            }

            void unlock_shared() {
                return this->ReleaseReadLock();
            }

            void lock() {
                return this->AcquireWriteLock();
            }

            void unlock() {
                return this->ReleaseWriteLock();
            }

            operator ReadWriteBusyMutexType &() {
                return m_rw_mutex;
            }

            operator const ReadWriteBusyMutexType &() const {
                return m_rw_mutex;
            }

            ReadWriteBusyMutexType *GetBase() {
                return std::addressof(m_rw_mutex);
            }
    };

}