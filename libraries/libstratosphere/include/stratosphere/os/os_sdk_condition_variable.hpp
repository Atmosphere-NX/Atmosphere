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
#include <stratosphere/os/os_sdk_mutex.hpp>
#include <stratosphere/os/impl/os_internal_condition_variable.hpp>

namespace ams::os {

    struct SdkConditionVariableType {
        union {
            s32 _arr[sizeof(impl::InternalConditionVariableStorage) / sizeof(s32)];
            impl::InternalConditionVariableStorage _storage;
        };

        void Initialize() {
            GetReference(this->_storage).Initialize();
        }

        void Wait(SdkMutexType &mutex);
        bool TimedWait(SdkMutexType &mutex, TimeSpan timeout);

        /* TODO: SdkRecursiveMutexType */

        void Signal() {
            GetReference(this->_storage).Signal();
        }

        void Broadcast() {
            GetReference(this->_storage).Broadcast();
        }
    };
    static_assert(std::is_trivial<SdkConditionVariableType>::value);

    class SdkConditionVariable {
        private:
            SdkConditionVariableType cv;
        public:
            constexpr SdkConditionVariable() : cv{{0}} { /* ... */ }

            void Wait(SdkMutex &m) {
                return this->cv.Wait(m.mutex);
            }

            bool TimedWait(SdkMutex &m, TimeSpan timeout) {
                return this->cv.TimedWait(m.mutex, timeout);
            }

            /* TODO: SdkRecursiveMutexType */

            void Signal() {
                return this->cv.Signal();
            }

            void Broadcast() {
                return this->cv.Broadcast();
            }
    };

}
