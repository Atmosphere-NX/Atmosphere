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
#include <stratosphere.hpp>
#include "impl/os_thread_manager.hpp"
#include "impl/os_tls_manager.hpp"

namespace ams::os {

    #if defined(ATMOSPHERE_OS_HORIZON)
    namespace {

        using LibnxTlsDestructor = void (*)(void *);

    }

    Result AllocateTlsSlot(TlsSlot *out, TlsDestructor destructor) {
        s32 slot = ::threadTlsAlloc(reinterpret_cast<LibnxTlsDestructor>(reinterpret_cast<void *>(destructor)));
        R_UNLESS(slot >= 0, os::ResultOutOfResource());

        *out = { static_cast<u32>(slot) };
        R_SUCCEED();
    }

    void FreeTlsSlot(TlsSlot slot) {
        ::threadTlsFree(static_cast<s32>(slot._value));
    }

    uintptr_t GetTlsValue(TlsSlot slot) {
        return reinterpret_cast<uintptr_t>(::threadTlsGet(static_cast<s32>(slot._value)));
    }

    void SetTlsValue(TlsSlot slot, uintptr_t value) {
        ::threadTlsSet(static_cast<s32>(slot._value), reinterpret_cast<void *>(value));
    }
    #else

    Result AllocateTlsSlot(TlsSlot *out, TlsDestructor destructor) {
        R_UNLESS(impl::GetTlsManager().AllocateTlsSlot(out, destructor, false), os::ResultOutOfResource());
        R_SUCCEED();
    }

    void FreeTlsSlot(TlsSlot slot) {
        AMS_ASSERT(slot._value < impl::TotalTlsSlotCountMax);
        impl::GetTlsManager().FreeTlsSlot(slot);
    }

    uintptr_t GetTlsValue(TlsSlot slot) {
        AMS_ASSERT(slot._value < impl::TotalTlsSlotCountMax);
        return impl::GetCurrentThread()->tls_value_array[slot._value];
    }

    void SetTlsValue(TlsSlot slot, uintptr_t value) {
        AMS_ASSERT(slot._value < impl::TotalTlsSlotCountMax);
        impl::GetCurrentThread()->tls_value_array[slot._value] = value;
    }

    #endif

}
