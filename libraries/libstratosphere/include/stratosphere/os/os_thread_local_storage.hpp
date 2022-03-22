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
#include <stratosphere/os/os_thread_local_storage_common.hpp>
#include <stratosphere/os/os_thread_local_storage_api.hpp>

namespace ams::os {

    class ThreadLocalStorage {
        NON_COPYABLE(ThreadLocalStorage);
        NON_MOVEABLE(ThreadLocalStorage);
        private:
            TlsSlot m_tls_slot;
        public:
            ThreadLocalStorage() {
                R_ABORT_UNLESS(os::AllocateTlsSlot(std::addressof(m_tls_slot), nullptr));
            }

            explicit ThreadLocalStorage(TlsDestructor destructor) {
                R_ABORT_UNLESS(os::AllocateTlsSlot(std::addressof(m_tls_slot), destructor));
            }

            ~ThreadLocalStorage() {
                os::FreeTlsSlot(m_tls_slot);
            }

            uintptr_t GetValue() const { return os::GetTlsValue(m_tls_slot); }
            void SetValue(uintptr_t value) { return os::SetTlsValue(m_tls_slot, value); }

            TlsSlot GetTlsSlot() const { return m_tls_slot; }
    };

}
