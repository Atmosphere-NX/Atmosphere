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
#include <stratosphere/os/os_sdk_mutex.hpp>
#include <stratosphere/os/os_sdk_recursive_mutex.hpp>
#include <stratosphere/os/impl/os_internal_condition_variable.hpp>

namespace ams::os {

    struct SdkConditionVariableType {
        union {
            s32 _arr[sizeof(impl::InternalConditionVariableStorage) / sizeof(s32)];
            impl::InternalConditionVariableStorage _storage;
        };

        ALWAYS_INLINE void Initialize() {
            GetReference(this->_storage).Initialize();
        }

        void Wait(SdkMutexType &mutex);
        bool TimedWait(SdkMutexType &mutex, TimeSpan timeout);

        void Wait(SdkRecursiveMutexType &mutex);
        bool TimedWait(SdkRecursiveMutexType &mutex, TimeSpan timeout);

        ALWAYS_INLINE void Signal() {
            GetReference(this->_storage).Signal();
        }

        ALWAYS_INLINE void Broadcast() {
            GetReference(this->_storage).Broadcast();
        }
    };
    static_assert(std::is_trivial<SdkConditionVariableType>::value);

    class SdkConditionVariable {
        private:
            SdkConditionVariableType m_cv;
        public:
            constexpr SdkConditionVariable() : m_cv{{0}} { /* ... */ }

            ALWAYS_INLINE void Wait(SdkMutex &m) {
                return m_cv.Wait(m.m_mutex);
            }

            ALWAYS_INLINE bool TimedWait(SdkMutex &m, TimeSpan timeout) {
                return m_cv.TimedWait(m.m_mutex, timeout);
            }

            ALWAYS_INLINE void Wait(SdkRecursiveMutex &m) {
                return m_cv.Wait(m.m_mutex);
            }

            ALWAYS_INLINE bool TimedWait(SdkRecursiveMutex &m, TimeSpan timeout) {
                return m_cv.TimedWait(m.m_mutex, timeout);
            }

            ALWAYS_INLINE void Signal() {
                return m_cv.Signal();
            }

            ALWAYS_INLINE void Broadcast() {
                return m_cv.Broadcast();
            }
    };

}
