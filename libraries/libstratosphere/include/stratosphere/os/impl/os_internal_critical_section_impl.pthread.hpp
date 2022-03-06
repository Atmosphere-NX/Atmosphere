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
#include <stratosphere/os/os_common_types.hpp>

namespace ams::os::impl {

    class InternalConditionVariableImpl;

    /* NOTE: macOS (and possibly other targets) do not provide adaptive mutex. */
    #if defined(PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP)
    #define AMS_OS_IMPL_PTHREAD_MUTEX_INITIALIZER PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP
    #else
    #define AMS_OS_IMPL_PTHREAD_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER
    #endif

    #if defined(ATMOSPHERE_OS_LINUX)
        #define AMS_OS_INTERNAL_CRITICAL_SECTION_IMPL_CAN_CHECK_LOCKED_BY_CURRENT_THREAD
    #else
        //#define AMS_OS_INTERNAL_CRITICAL_SECTION_IMPL_CAN_CHECK_LOCKED_BY_CURRENT_THREAD
    #endif

    class InternalCriticalSectionImpl {
        private:
            friend class InternalConditionVariableImpl;
        private:
            pthread_mutex_t m_pthread_mutex = AMS_OS_IMPL_PTHREAD_MUTEX_INITIALIZER;
        public:
            constexpr InternalCriticalSectionImpl() = default;
            constexpr ~InternalCriticalSectionImpl() {
                if (!std::is_constant_evaluated()) {
                    this->Finalize();
                }
            }

            void Initialize();
            void Finalize();

            void Enter();
            bool TryEnter();
            void Leave();

            #if defined(AMS_OS_INTERNAL_CRITICAL_SECTION_IMPL_CAN_CHECK_LOCKED_BY_CURRENT_THREAD)
            bool IsLockedByCurrentThread() const;
            #endif

            ALWAYS_INLINE void Lock()    { return this->Enter(); }
            ALWAYS_INLINE bool TryLock() { return this->TryEnter(); }
            ALWAYS_INLINE void Unlock()  { return this->Leave(); }

            ALWAYS_INLINE void lock()     { return this->Lock(); }
            ALWAYS_INLINE bool try_lock() { return this->TryLock(); }
            ALWAYS_INLINE void unlock()   { return this->Unlock(); }
    };

    using InternalCriticalSectionStorageTypeForConstantInitialize = pthread_mutex_t;

    #define AMS_OS_INTERNAL_CRITICAL_SECTION_IMPL_CONSTANT_INITIALIZER ._storage_for_constant_initialize = AMS_OS_IMPL_PTHREAD_MUTEX_INITIALIZER

}
