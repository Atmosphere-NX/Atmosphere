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
#include <stratosphere/os/os_mutex_common.hpp>
#include <stratosphere/os/os_mutex_types.hpp>
#include <stratosphere/os/os_mutex_api.hpp>

namespace ams::os {

    class Mutex {
        NON_COPYABLE(Mutex);
        NON_MOVEABLE(Mutex);
        private:
            MutexType mutex;
        public:
            constexpr explicit Mutex(bool recursive) : mutex{::ams::os::MutexType::State_Initialized, recursive, 0, 0, nullptr, {{0}}} { /* ... */ }

            ~Mutex() { FinalizeMutex(std::addressof(this->mutex)); }

            void lock() {
                return LockMutex(std::addressof(this->mutex));
            }

            void unlock() {
                return UnlockMutex(std::addressof(this->mutex));
            }

            bool try_lock() {
                return TryLockMutex(std::addressof(this->mutex));
            }

            bool IsLockedByCurrentThread() const {
                return IsMutexLockedByCurrentThread(std::addressof(this->mutex));
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
                return this->mutex;
            }

            operator const MutexType &() const {
                return this->mutex;
            }

            MutexType *GetBase() {
                return std::addressof(this->mutex);
            }
    };

}