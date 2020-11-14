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

        Result WaitProcessWideKeyAtomic(uintptr_t address, uintptr_t cv_key, uint32_t tag, int64_t timeout_ns) {
            /* Validate input. */
            R_UNLESS(AMS_LIKELY(!IsKernelAddress(address)),     svc::ResultInvalidCurrentMemory());
            R_UNLESS(util::IsAligned(address, sizeof(int32_t)), svc::ResultInvalidAddress());

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

            /* Wait on the condition variable. */
            return GetCurrentProcess().WaitConditionVariable(address, util::AlignDown(cv_key, sizeof(u32)), tag, timeout);
        }

        void SignalProcessWideKey(uintptr_t cv_key, int32_t count) {
            /* Signal the condition variable. */
            return GetCurrentProcess().SignalConditionVariable(util::AlignDown(cv_key, sizeof(u32)), count);
        }

    }

    /* =============================    64 ABI    ============================= */

    Result WaitProcessWideKeyAtomic64(ams::svc::Address address, ams::svc::Address cv_key, uint32_t tag, int64_t timeout_ns) {
        return WaitProcessWideKeyAtomic(address, cv_key, tag, timeout_ns);
    }

    void SignalProcessWideKey64(ams::svc::Address cv_key, int32_t count) {
        return SignalProcessWideKey(cv_key, count);
    }

    /* ============================= 64From32 ABI ============================= */

    Result WaitProcessWideKeyAtomic64From32(ams::svc::Address address, ams::svc::Address cv_key, uint32_t tag, int64_t timeout_ns) {
        return WaitProcessWideKeyAtomic(address, cv_key, tag, timeout_ns);
    }

    void SignalProcessWideKey64From32(ams::svc::Address cv_key, int32_t count) {
        return SignalProcessWideKey(cv_key, count);
    }

}
