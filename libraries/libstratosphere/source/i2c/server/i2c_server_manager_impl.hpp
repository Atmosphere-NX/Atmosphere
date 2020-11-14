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
#include <stratosphere.hpp>
#include "i2c_server_session_impl.hpp"

namespace ams::i2c::server {

    class ManagerImpl {
        private:
            lmem::HeapHandle heap_handle;
            ams::sf::ExpHeapMemoryResource session_memory_resource;
            typename ams::sf::ServiceObjectAllocator<i2c::sf::ISession, SessionImpl> allocator;
            u8 heap_buffer[4_KB];
        public:
            ManagerImpl();

            ~ManagerImpl();
        public:
            /* Actual commands. */
            Result OpenSessionForDev(ams::sf::Out<std::shared_ptr<i2c::sf::ISession>> out, s32 bus_idx, u16 slave_address, i2c::AddressingMode addressing_mode, i2c::SpeedMode speed_mode);
            Result OpenSession(ams::sf::Out<std::shared_ptr<i2c::sf::ISession>> out, i2c::I2cDevice device);
            Result HasDevice(ams::sf::Out<bool> out, i2c::I2cDevice device);
            Result HasDeviceForDev(ams::sf::Out<bool> out, i2c::I2cDevice device);
            Result OpenSession2(ams::sf::Out<std::shared_ptr<i2c::sf::ISession>> out, DeviceCode device_code);
    };
    static_assert(i2c::sf::IsIManager<ManagerImpl>);

}
