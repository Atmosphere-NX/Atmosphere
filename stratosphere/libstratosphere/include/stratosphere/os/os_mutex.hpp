/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

    class ConditionVariable;

    class Mutex {
        NON_COPYABLE(Mutex);
        NON_MOVEABLE(Mutex);
        friend class ams::os::ConditionVariable;
        private:
            ::Mutex m;
        private:
            ::Mutex *GetMutex() {
                return &this->m;
            }
        public:
            Mutex() {
                mutexInit(GetMutex());
            }

            void lock() {
                mutexLock(GetMutex());
            }

            void unlock() {
                mutexUnlock(GetMutex());
            }

            bool try_lock() {
                return mutexTryLock(GetMutex());
            }

            void Lock() {
                lock();
            }

            void Unlock() {
                unlock();
            }

            bool TryLock() {
                return try_lock();
            }
    };

    class RecursiveMutex {
        private:
            ::RMutex m;
        private:
            ::RMutex *GetMutex() {
                return &this->m;
            }
        public:
            RecursiveMutex() {
                rmutexInit(GetMutex());
            }

            void lock() {
                rmutexLock(GetMutex());
            }

            void unlock() {
                rmutexUnlock(GetMutex());
            }

            bool try_lock() {
                return rmutexTryLock(GetMutex());
            }

            void Lock() {
                lock();
            }

            void Unlock() {
                unlock();
            }

            bool TryLock() {
                return try_lock();
            }
    };

}