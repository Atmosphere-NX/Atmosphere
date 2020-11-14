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
#include <stratosphere/os/os_thread_local_storage.hpp>
#include <stratosphere/os/os_sdk_thread_local_storage_api.hpp>

namespace ams::os {

    class SdkThreadLocalStorage {
        NON_COPYABLE(SdkThreadLocalStorage);
        NON_MOVEABLE(SdkThreadLocalStorage);
        private:
            TlsSlot tls_slot;
        public:
            SdkThreadLocalStorage() {
                R_ABORT_UNLESS(os::SdkAllocateTlsSlot(std::addressof(this->tls_slot), nullptr));
            }

            explicit SdkThreadLocalStorage(TlsDestructor destructor) {
                R_ABORT_UNLESS(os::SdkAllocateTlsSlot(std::addressof(this->tls_slot), destructor));
            }

            ~SdkThreadLocalStorage() {
                os::FreeTlsSlot(this->tls_slot);
            }

            uintptr_t GetValue() const { return os::GetTlsValue(this->tls_slot); }
            void SetValue(uintptr_t value) { return os::SetTlsValue(this->tls_slot, value); }

            TlsSlot GetTlsSlot() const { return this->tls_slot; }
    };

}
