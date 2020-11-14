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
#include "i2c_server_manager_impl.hpp"

namespace ams::i2c::server {

    ManagerImpl::ManagerImpl() : session_memory_resource(), allocator(std::addressof(session_memory_resource)) {
        this->heap_handle = lmem::CreateExpHeap(this->heap_buffer, sizeof(this->heap_buffer), lmem::CreateOption_None);
        this->session_memory_resource.Attach(this->heap_handle);
    }

    ManagerImpl::~ManagerImpl() {
        lmem::DestroyExpHeap(this->heap_handle);
    }

    Result ManagerImpl::OpenSessionForDev(ams::sf::Out<std::shared_ptr<i2c::sf::ISession>> out, s32 bus_idx, u16 slave_address, i2c::AddressingMode addressing_mode, i2c::SpeedMode speed_mode) {
        /* TODO */
        AMS_ABORT();
    }

    Result ManagerImpl::OpenSession(ams::sf::Out<std::shared_ptr<i2c::sf::ISession>> out, i2c::I2cDevice device) {
        return this->OpenSession2(out, ConvertToDeviceCode(device));
    }

    Result ManagerImpl::HasDevice(ams::sf::Out<bool> out, i2c::I2cDevice device) {
        /* TODO */
        AMS_ABORT();
    }

    Result ManagerImpl::HasDeviceForDev(ams::sf::Out<bool> out, i2c::I2cDevice device) {
        /* TODO */
        AMS_ABORT();
    }

    Result ManagerImpl::OpenSession2(ams::sf::Out<std::shared_ptr<i2c::sf::ISession>> out, DeviceCode device_code) {
        /* Allocate a session. */
        auto session = ams::sf::AllocateShared<i2c::sf::ISession, SessionImpl>(this->allocator, this);

        /* Open the session. */
        R_TRY(session->GetImpl().OpenSession(device_code));

        /* We succeeded. */
        out.SetValue(std::move(session));
        return ResultSuccess();
    }

}
