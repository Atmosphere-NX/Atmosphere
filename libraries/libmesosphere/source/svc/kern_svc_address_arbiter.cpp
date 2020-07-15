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
#include <mesosphere.hpp>

namespace ams::kern::svc {

    /* =============================    Common    ============================= */

    namespace {

        constexpr bool IsKernelAddress(uintptr_t address) {
            return KernelVirtualAddressSpaceBase <= address && address < KernelVirtualAddressSpaceEnd;
        }

        constexpr bool IsValidSignalType(ams::svc::SignalType type) {
            switch (type) {
                case ams::svc::SignalType_Signal:
                case ams::svc::SignalType_SignalAndIncrementIfEqual:
                case ams::svc::SignalType_SignalAndModifyByWaitingCountIfEqual:
                    return true;
                default:
                    return false;
            }
        }

        constexpr bool IsValidArbitrationType(ams::svc::ArbitrationType type) {
            switch (type) {
                case ams::svc::ArbitrationType_WaitIfLessThan:
                case ams::svc::ArbitrationType_DecrementAndWaitIfLessThan:
                case ams::svc::ArbitrationType_WaitIfEqual:
                    return true;
                default:
                    return false;
            }
        }

        Result WaitForAddress(uintptr_t address, ams::svc::ArbitrationType arb_type, int32_t value, int64_t timeout_ns) {
            /* Validate input. */
            R_UNLESS(AMS_LIKELY(!IsKernelAddress(address)),     svc::ResultInvalidCurrentMemory());
            R_UNLESS(util::IsAligned(address, sizeof(int32_t)), svc::ResultInvalidAddress());
            R_UNLESS(IsValidArbitrationType(arb_type),          svc::ResultInvalidEnumValue());

            /* Convert timeout from nanoseconds to ticks. */
            s64 timeout;
            if (timeout_ns > 0) {
                const ams::svc::Tick offset_tick(TimeSpan::FromNanoSeconds(timeout_ns));
                if (AMS_LIKELY(offset_tick > 0)) {
                    timeout = KHardwareTimer::GetTick() + offset_tick + 2;
                    if (AMS_UNLIKELY(timeout <= 0)) {
                        timeout = std::numeric_limits<s64>::max();
                    }
                } else {
                    timeout = std::numeric_limits<s64>::max();
                }
            } else {
                timeout = timeout_ns;
            }

            return GetCurrentProcess().WaitAddressArbiter(address, arb_type, value, timeout);
        }

        Result SignalToAddress(uintptr_t address, ams::svc::SignalType signal_type, int32_t value, int32_t count) {
            /* Validate input. */
            R_UNLESS(AMS_LIKELY(!IsKernelAddress(address)),     svc::ResultInvalidCurrentMemory());
            R_UNLESS(util::IsAligned(address, sizeof(int32_t)), svc::ResultInvalidAddress());
            R_UNLESS(IsValidSignalType(signal_type),            svc::ResultInvalidEnumValue());

            return GetCurrentProcess().SignalAddressArbiter(address, signal_type, value, count);
        }

    }

    /* =============================    64 ABI    ============================= */

    Result WaitForAddress64(ams::svc::Address address, ams::svc::ArbitrationType arb_type, int32_t value, int64_t timeout_ns) {
        return WaitForAddress(address, arb_type, value, timeout_ns);
    }

    Result SignalToAddress64(ams::svc::Address address, ams::svc::SignalType signal_type, int32_t value, int32_t count) {
        return SignalToAddress(address, signal_type, value, count);
    }

    /* ============================= 64From32 ABI ============================= */

    Result WaitForAddress64From32(ams::svc::Address address, ams::svc::ArbitrationType arb_type, int32_t value, int64_t timeout_ns) {
        return WaitForAddress(address, arb_type, value, timeout_ns);
    }

    Result SignalToAddress64From32(ams::svc::Address address, ams::svc::SignalType signal_type, int32_t value, int32_t count) {
        return SignalToAddress(address, signal_type, value, count);
    }

}
