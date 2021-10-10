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
#include "gpio_server_manager_impl.hpp"

namespace ams::gpio::server {

    ManagerImpl::ManagerImpl() {
        m_heap_handle = lmem::CreateExpHeap(m_heap_buffer, sizeof(m_heap_buffer), lmem::CreateOption_None);
        m_pad_allocator.Attach(m_heap_handle);
    }

    ManagerImpl::~ManagerImpl() {
        lmem::DestroyExpHeap(m_heap_handle);
    }

    Result ManagerImpl::OpenSessionForDev(ams::sf::Out<ams::sf::SharedPointer<gpio::sf::IPadSession>> out, s32 pad_descriptor) {
        /* TODO */
        AMS_UNUSED(out, pad_descriptor);
        AMS_ABORT();
    }

    Result ManagerImpl::OpenSession(ams::sf::Out<ams::sf::SharedPointer<gpio::sf::IPadSession>> out, gpio::GpioPadName pad_name) {
        return this->OpenSession2(out, ConvertToDeviceCode(pad_name), ddsf::AccessMode_ReadWrite);
    }

    Result ManagerImpl::OpenSessionForTest(ams::sf::Out<ams::sf::SharedPointer<gpio::sf::IPadSession>> out, gpio::GpioPadName pad_name) {
        /* TODO */
        AMS_UNUSED(out, pad_name);
        AMS_ABORT();
    }

    Result ManagerImpl::IsWakeEventActive(ams::sf::Out<bool> out, gpio::GpioPadName pad_name) {
        /* TODO */
        AMS_UNUSED(out, pad_name);
        AMS_ABORT();
    }

    Result ManagerImpl::GetWakeEventActiveFlagSet(ams::sf::Out<gpio::WakeBitFlag> out) {
        /* TODO */
        AMS_UNUSED(out);
        AMS_ABORT();
    }

    Result ManagerImpl::SetWakeEventActiveFlagSetForDebug(gpio::GpioPadName pad_name, bool is_enabled) {
        /* TODO */
        AMS_UNUSED(pad_name, is_enabled);
        AMS_ABORT();
    }

    Result ManagerImpl::SetWakePinDebugMode(s32 mode) {
        /* TODO */
        AMS_UNUSED(mode);
        AMS_ABORT();
    }

    Result ManagerImpl::OpenSession2(ams::sf::Out<ams::sf::SharedPointer<gpio::sf::IPadSession>> out, DeviceCode device_code, ddsf::AccessMode access_mode) {
        /* Allocate a session. */
        auto session = Factory::CreateSharedEmplaced<gpio::sf::IPadSession, PadSessionImpl>(std::addressof(m_pad_allocator), this);

        /* Open the session. */
        R_TRY(session.GetImpl().OpenSession(device_code, access_mode));

        /* We succeeded. */
        *out = std::move(session);
        return ResultSuccess();
    }

    Result ManagerImpl::IsWakeEventActive2(ams::sf::Out<bool> out, DeviceCode device_code) {
        /* TODO */
        AMS_UNUSED(out, device_code);
        AMS_ABORT();
    }

    Result ManagerImpl::SetWakeEventActiveFlagSetForDebug2(DeviceCode device_code, bool is_enabled) {
        /* TODO */
        AMS_UNUSED(device_code, is_enabled);
        AMS_ABORT();
    }

    Result ManagerImpl::SetRetryValues(u32 arg0, u32 arg1) {
        /* TODO */
        AMS_UNUSED(arg0, arg1);
        AMS_ABORT();
    }

}
