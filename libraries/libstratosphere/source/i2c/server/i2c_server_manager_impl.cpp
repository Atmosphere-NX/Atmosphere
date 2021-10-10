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
#include "i2c_server_manager_impl.hpp"

namespace ams::i2c::server {

    ManagerImpl::ManagerImpl() {
        m_heap_handle = lmem::CreateExpHeap(m_heap_buffer, sizeof(m_heap_buffer), lmem::CreateOption_None);
        m_allocator.Attach(m_heap_handle);
    }

    ManagerImpl::~ManagerImpl() {
        lmem::DestroyExpHeap(m_heap_handle);
    }

    Result ManagerImpl::OpenSessionForDev(ams::sf::Out<ams::sf::SharedPointer<i2c::sf::ISession>> out, s32 bus_idx, u16 slave_address, i2c::AddressingMode addressing_mode, i2c::SpeedMode speed_mode) {
        /* TODO */
        AMS_UNUSED(out, bus_idx, slave_address, addressing_mode, speed_mode);
        AMS_ABORT();
    }

    Result ManagerImpl::OpenSession(ams::sf::Out<ams::sf::SharedPointer<i2c::sf::ISession>> out, i2c::I2cDevice device) {
        return this->OpenSession2(out, ConvertToDeviceCode(device));
    }

    Result ManagerImpl::HasDevice(ams::sf::Out<bool> out, i2c::I2cDevice device) {
        /* TODO */
        AMS_UNUSED(out, device);
        AMS_ABORT();
    }

    Result ManagerImpl::HasDeviceForDev(ams::sf::Out<bool> out, i2c::I2cDevice device) {
        /* TODO */
        AMS_UNUSED(out, device);
        AMS_ABORT();
    }

    Result ManagerImpl::OpenSession2(ams::sf::Out<ams::sf::SharedPointer<i2c::sf::ISession>> out, DeviceCode device_code) {
        /* Allocate a session. */
        auto session = Factory::CreateSharedEmplaced<i2c::sf::ISession, SessionImpl>(std::addressof(m_allocator), this);

        /* Open the session. */
        R_TRY(session.GetImpl().OpenSession(device_code));

        /* We succeeded. */
        *out = std::move(session);
        return ResultSuccess();
    }

}
