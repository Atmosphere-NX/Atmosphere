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
#include <stratosphere/os/os_mutex_types.hpp>
#include <stratosphere/os/os_condition_variable_common.hpp>
#include <stratosphere/os/os_condition_variable_types.hpp>
#include <stratosphere/os/os_condition_variable_api.hpp>

namespace ams::os {

    class ConditionVariable {
        NON_COPYABLE(ConditionVariable);
        NON_MOVEABLE(ConditionVariable);
        private:
            ConditionVariableType m_cv;
        public:
            constexpr ConditionVariable() : m_cv{::ams::os::ConditionVariableType::State_Initialized, {{0}}} { /* ... */ }

            ~ConditionVariable() { FinalizeConditionVariable(std::addressof(m_cv)); }

            void Signal() {
                SignalConditionVariable(std::addressof(m_cv));
            }

            void Broadcast() {
                BroadcastConditionVariable(std::addressof(m_cv));
            }

            void Wait(ams::os::MutexType &mutex) {
                WaitConditionVariable(std::addressof(m_cv), std::addressof(mutex));
            }

            ConditionVariableStatus TimedWait(ams::os::MutexType &mutex, TimeSpan timeout) {
                return TimedWaitConditionVariable(std::addressof(m_cv), std::addressof(mutex), timeout);
            }

            operator ConditionVariableType &() {
                return m_cv;
            }

            operator const ConditionVariableType &() const {
                return m_cv;
            }

            ConditionVariableType *GetBase() {
                return std::addressof(m_cv);
            }
    };

}
