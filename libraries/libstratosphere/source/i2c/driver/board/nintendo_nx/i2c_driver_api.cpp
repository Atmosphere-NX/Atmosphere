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

namespace ams::i2c::driver::board::nintendo_nx {

    namespace {

        struct I2cDeviceDefinition {
            DeviceCode device_code;
            u8 slave_address;
        };

        struct I2cBusDefinition {
            DeviceCode device_code;
            dd::PhysicalAddress registers_phys_addr;
            size_t registers_size;
            SpeedMode speed_mode;
            os::InterruptName interrupt_name;
            const I2cDeviceDefinition *devices;
            size_t num_devices;
        };

        #include "i2c_bus_device_map.inc"

    }

    void Initialize() {
        /* TODO */
    }

}
