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
#include <stratosphere/os/os_condition_variable_common.hpp>

#if defined(ATMOSPHERE_OS_HORIZON)
    #include <stratosphere/os/impl/os_internal_condition_variable_impl.os.horizon.hpp>
#else
    #error "Unknown OS for ams::os::impl::InternalConditionVariableImpl"
#endif

namespace ams::os::impl {

    class InternalConditionVariable {
        private:
            InternalConditionVariableImpl impl;
        public:
            constexpr InternalConditionVariable() : impl() { /* ... */ }

            constexpr void Initialize() {
                this->impl.Initialize();
            }

            void Signal() {
                this->impl.Signal();
            }

            void Broadcast() {
                this->impl.Broadcast();
            }

            void Wait(InternalCriticalSection *cs) {
                this->impl.Wait(cs);
            }

            ConditionVariableStatus TimedWait(InternalCriticalSection *cs, const TimeoutHelper &timeout_helper) {
                return this->impl.TimedWait(cs, timeout_helper);
            }
    };

    using InternalConditionVariableStorage = TYPED_STORAGE(InternalConditionVariable);

}
