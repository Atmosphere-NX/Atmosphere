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

namespace ams::fssystem::buffers {

    namespace {

        /* TODO: os::SdkThreadLocalStorage g_buffer_manager_context_tls_slot; */

        class ThreadLocalStorageWrapper {
            private:
                os::TlsSlot tls_slot;
            public:
                ThreadLocalStorageWrapper() { R_ABORT_UNLESS(os::AllocateTlsSlot(std::addressof(this->tls_slot), nullptr)); }
                ~ThreadLocalStorageWrapper() { os::FreeTlsSlot(this->tls_slot); }

                void SetValue(uintptr_t value) { os::SetTlsValue(this->tls_slot, value); }
                uintptr_t GetValue() const { return os::GetTlsValue(this->tls_slot); }
                os::TlsSlot GetTlsSlot() const { return this->tls_slot; }
        } g_buffer_manager_context_tls_slot;

    }

    void RegisterBufferManagerContext(const BufferManagerContext *context) {
        g_buffer_manager_context_tls_slot.SetValue(reinterpret_cast<uintptr_t>(context));
    }

    BufferManagerContext *GetBufferManagerContext() {
        return reinterpret_cast<BufferManagerContext *>(g_buffer_manager_context_tls_slot.GetValue());
    }

    void EnableBlockingBufferManagerAllocation() {
        if (auto context = GetBufferManagerContext(); context != nullptr) {
            context->SetNeedBlocking(true);
        }
    }

}
