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
#include <stratosphere.hpp>

namespace ams::os {

    /* TODO: How will this work without libnx? */

    namespace {

        using LibnxTlsDestructor = void (*)(void *);

    }

    Result AllocateTlsSlot(TlsSlot *out, TlsDestructor destructor) {
        s32 slot = ::threadTlsAlloc(reinterpret_cast<LibnxTlsDestructor>(destructor));
        R_UNLESS(slot >= 0, os::ResultOutOfResource());

        *out = { static_cast<u32>(slot) };
        return ResultSuccess();
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

}
