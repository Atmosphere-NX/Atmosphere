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
#include <stratosphere/os/os_mutex_common.hpp>
#include <stratosphere/os/os_mutex_types.hpp>
#include <stratosphere/os/os_mutex_api.hpp>

namespace ams::os {

    class Mutex {
        NON_COPYABLE(Mutex);
        NON_MOVEABLE(Mutex);
        private:
            MutexType m_mutex;
        public:
            constexpr explicit Mutex(bool recursive) : m_mutex{::ams::os::MutexType::State_Initialized, recursive, 0, 0, nullptr, {{0}}} { /* ... */ }

            ~Mutex() { FinalizeMutex(std::addressof(m_mutex)); }

            void lock() {
                return LockMutex(std::addressof(m_mutex));
            }

            void unlock() {
                return UnlockMutex(std::addressof(m_mutex));
            }

            bool try_lock() {
                return TryLockMutex(std::addressof(m_mutex));
            }

            bool IsLockedByCurrentThread() const {
                return IsMutexLockedByCurrentThread(std::addressof(m_mutex));
            }

            ALWAYS_INLINE void Lock() {
                return this->lock();
            }

            ALWAYS_INLINE void Unlock() {
                return this->unlock();
            }

            ALWAYS_INLINE bool TryLock() {
                return this->try_lock();
            }

            operator MutexType &() {
                return m_mutex;
            }

            operator const MutexType &() const {
                return m_mutex;
            }

            MutexType *GetBase() {
                return std::addressof(m_mutex);
            }
    };

}