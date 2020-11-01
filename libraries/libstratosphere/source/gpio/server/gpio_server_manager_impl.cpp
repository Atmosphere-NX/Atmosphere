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
#include "gpio_server_manager_impl.hpp"

namespace ams::gpio::server {

    ManagerImpl::ManagerImpl() : pad_session_memory_resource(), pad_allocator(std::addressof(pad_session_memory_resource)) {
        this->heap_handle = lmem::CreateExpHeap(this->heap_buffer, sizeof(this->heap_buffer), lmem::CreateOption_None);
        this->pad_session_memory_resource.Attach(this->heap_handle);
    }

    ManagerImpl::~ManagerImpl() {
        lmem::DestroyExpHeap(this->heap_handle);
    }

    Result ManagerImpl::OpenSessionForDev(ams::sf::Out<std::shared_ptr<gpio::sf::IPadSession>> out, s32 pad_descriptor) {
        /* TODO */
        AMS_ABORT();
    }

    Result ManagerImpl::OpenSession(ams::sf::Out<std::shared_ptr<gpio::sf::IPadSession>> out, gpio::GpioPadName pad_name) {
        return this->OpenSession2(out, ConvertToDeviceCode(pad_name), ddsf::AccessMode_ReadWrite);
    }

    Result ManagerImpl::OpenSessionForTest(ams::sf::Out<std::shared_ptr<gpio::sf::IPadSession>> out, gpio::GpioPadName pad_name) {
        /* TODO */
        AMS_ABORT();
    }

    Result ManagerImpl::IsWakeEventActive(ams::sf::Out<bool> out, gpio::GpioPadName pad_name) {
        /* TODO */
        AMS_ABORT();
    }

    Result ManagerImpl::GetWakeEventActiveFlagSet(ams::sf::Out<gpio::WakeBitFlag> out) {
        /* TODO */
        AMS_ABORT();
    }

    Result ManagerImpl::SetWakeEventActiveFlagSetForDebug(gpio::GpioPadName pad_name, bool is_enabled) {
        /* TODO */
        AMS_ABORT();
    }

    Result ManagerImpl::SetWakePinDebugMode(s32 mode) {
        /* TODO */
        AMS_ABORT();
    }

    Result ManagerImpl::OpenSession2(ams::sf::Out<std::shared_ptr<gpio::sf::IPadSession>> out, DeviceCode device_code, ddsf::AccessMode access_mode) {
        /* Allocate a session. */
        auto session = ams::sf::AllocateShared<gpio::sf::IPadSession, PadSessionImpl>(this->pad_allocator, this);

        /* Open the session. */
        R_TRY(session->GetImpl().OpenSession(device_code, access_mode));

        /* We succeeded. */
        out.SetValue(std::move(session));
        return ResultSuccess();
    }

    Result ManagerImpl::IsWakeEventActive2(ams::sf::Out<bool> out, DeviceCode device_code) {
        /* TODO */
        AMS_ABORT();
    }

    Result ManagerImpl::SetWakeEventActiveFlagSetForDebug2(DeviceCode device_code, bool is_enabled) {
        /* TODO */
        AMS_ABORT();
    }

    Result ManagerImpl::SetRetryValues(u32 arg0, u32 arg1) {
        /* TODO */
        AMS_ABORT();
    }

}
