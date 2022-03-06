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
#include <stratosphere/os/os_condition_variable_common.hpp>
#include <stratosphere/os/impl/os_internal_critical_section.hpp>

namespace ams::os::impl {

    class TimeoutHelper;

    class InternalConditionVariableImpl {
        private:
            pthread_cond_t m_pthread_cond = PTHREAD_COND_INITIALIZER;
        public:
            constexpr InternalConditionVariableImpl() = default;

            constexpr ~InternalConditionVariableImpl() {
                if (!std::is_constant_evaluated()) {
                    this->Finalize();
                }
            }

            void Initialize();
            void Finalize();

            void Signal();
            void Broadcast();

            void Wait(InternalCriticalSection *cs);
            ConditionVariableStatus TimedWait(InternalCriticalSection *cs, const TimeoutHelper &timeout_helper);
    };

    using InternalConditionVariableStorageTypeForConstantInitialize = pthread_cond_t;

    #define AMS_OS_INTERNAL_CONDITION_VARIABLE_IMPL_CONSTANT_INITIALIZER ._storage_for_constant_initialize = PTHREAD_COND_INITIALIZER

}
