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

        Result ArbitrateLock(ams::svc::Handle thread_handle, uintptr_t address, uint32_t tag) {
            /* Validate the input address. */
            R_UNLESS(!IsKernelAddress(address),             svc::ResultInvalidCurrentMemory());
            R_UNLESS(util::IsAligned(address, sizeof(u32)), svc::ResultInvalidAddress());

            return GetCurrentProcess().WaitForAddress(thread_handle, address, tag);
        }

        Result ArbitrateUnlock(uintptr_t address) {
            /* Validate the input address. */
            R_UNLESS(!IsKernelAddress(address),             svc::ResultInvalidCurrentMemory());
            R_UNLESS(util::IsAligned(address, sizeof(u32)), svc::ResultInvalidAddress());

            return GetCurrentProcess().SignalToAddress(address);
        }

    }

    /* =============================    64 ABI    ============================= */

    Result ArbitrateLock64(ams::svc::Handle thread_handle, ams::svc::Address address, uint32_t tag) {
        return ArbitrateLock(thread_handle, address, tag);
    }

    Result ArbitrateUnlock64(ams::svc::Address address) {
        return ArbitrateUnlock(address);
    }

    /* ============================= 64From32 ABI ============================= */

    Result ArbitrateLock64From32(ams::svc::Handle thread_handle, ams::svc::Address address, uint32_t tag) {
        return ArbitrateLock(thread_handle, address, tag);
    }

    Result ArbitrateUnlock64From32(ams::svc::Address address) {
        return ArbitrateUnlock(address);
    }

}
