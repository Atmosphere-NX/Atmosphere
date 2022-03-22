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
#include <stratosphere/os/os_busy_mutex_types.hpp>
#include <stratosphere/os/os_busy_mutex_api.hpp>

namespace ams::os {

    class BusyMutex {
        NON_COPYABLE(BusyMutex);
        NON_MOVEABLE(BusyMutex);
        private:
            BusyMutexType m_mutex;
        public:
            constexpr explicit BusyMutex() : m_mutex{::ams::os::BusyMutexType::State_Initialized, nullptr, {{}}} { /* ... */ }

            ~BusyMutex() { FinalizeBusyMutex(std::addressof(m_mutex)); }

            void lock() {
                return LockBusyMutex(std::addressof(m_mutex));
            }

            void unlock() {
                return UnlockBusyMutex(std::addressof(m_mutex));
            }

            bool try_lock() {
                return TryLockBusyMutex(std::addressof(m_mutex));
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

            operator BusyMutexType &() {
                return m_mutex;
            }

            operator const BusyMutexType &() const {
                return m_mutex;
            }

            BusyMutexType *GetBase() {
                return std::addressof(m_mutex);
            }
    };

}