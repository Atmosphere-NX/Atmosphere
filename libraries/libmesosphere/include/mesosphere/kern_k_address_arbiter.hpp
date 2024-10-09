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
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_k_condition_variable.hpp>

namespace ams::kern {

    class KAddressArbiter {
        public:
            using ThreadTree = KConditionVariable::ThreadTree;
        private:
            ThreadTree m_tree;
        public:
            constexpr KAddressArbiter() = default;

            Result SignalToAddress(uintptr_t addr, ams::svc::SignalType type, s32 value, s32 count) {
                switch (type) {
                    case ams::svc::SignalType_Signal:
                        R_RETURN(this->Signal(addr, count));
                    case ams::svc::SignalType_SignalAndIncrementIfEqual:
                        R_RETURN(this->SignalAndIncrementIfEqual(addr, value, count));
                    case ams::svc::SignalType_SignalAndModifyByWaitingCountIfEqual:
                        R_RETURN(this->SignalAndModifyByWaitingCountIfEqual(addr, value, count));
                    MESOSPHERE_UNREACHABLE_DEFAULT_CASE();
                }
            }

            Result WaitForAddress(uintptr_t addr, ams::svc::ArbitrationType type, s64 value, s64 timeout) {
                switch (type) {
                    case ams::svc::ArbitrationType_WaitIfLessThan:
                        R_RETURN(this->WaitIfLessThan(addr, static_cast<s32>(value), false, timeout));
                    case ams::svc::ArbitrationType_DecrementAndWaitIfLessThan:
                        R_RETURN(this->WaitIfLessThan(addr, static_cast<s32>(value), true, timeout));
                    case ams::svc::ArbitrationType_WaitIfEqual:
                        R_RETURN(this->WaitIfEqual(addr, static_cast<s32>(value), timeout));
                    case ams::svc::ArbitrationType_WaitIfEqual64:
                        R_RETURN(this->WaitIfEqual64(addr, value, timeout));
                    MESOSPHERE_UNREACHABLE_DEFAULT_CASE();
                }
            }
        private:
            Result Signal(uintptr_t addr, s32 count);
            Result SignalAndIncrementIfEqual(uintptr_t addr, s32 value, s32 count);
            Result SignalAndModifyByWaitingCountIfEqual(uintptr_t addr, s32 value, s32 count);
            Result WaitIfLessThan(uintptr_t addr, s32 value, bool decrement, s64 timeout);
            Result WaitIfEqual(uintptr_t addr, s32 value, s64 timeout);
            Result WaitIfEqual64(uintptr_t addr, s64 value, s64 timeout);
    };

}
