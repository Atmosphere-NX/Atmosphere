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

namespace ams::os::impl {

    #if defined(ATMOSPHERE_OS_HORIZON)
        class ReadWriteLockHorizonImpl;
    #endif

    class InternalConditionVariableImpl;

    class InternalCriticalSectionImpl {
        private:
            #if defined(ATMOSPHERE_OS_HORIZON)
                friend class ReadWriteLockHorizonImpl;
            #endif

            friend class InternalConditionVariableImpl;
        private:
            u32 thread_handle;
        public:
            constexpr InternalCriticalSectionImpl() : thread_handle(svc::InvalidHandle) { /* ... */ }

            constexpr void Initialize() { this->thread_handle = svc::InvalidHandle; }
            constexpr void Finalize() { /* ... */}

            void Enter();
            bool TryEnter();
            void Leave();

            bool IsLockedByCurrentThread() const;

            ALWAYS_INLINE void Lock()    { return this->Enter(); }
            ALWAYS_INLINE bool TryLock() { return this->TryEnter(); }
            ALWAYS_INLINE void Unlock()  { return this->Leave(); }

            ALWAYS_INLINE void lock()     { return this->Lock(); }
            ALWAYS_INLINE bool try_lock() { return this->TryLock(); }
            ALWAYS_INLINE void unlock()   { return this->Unlock(); }
    };

}
