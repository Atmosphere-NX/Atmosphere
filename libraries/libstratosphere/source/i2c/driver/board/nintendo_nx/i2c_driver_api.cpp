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
#include "impl/i2c_bus_manager.hpp"
#include "impl/i2c_device_property_manager.hpp"

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

            constexpr bool IsPowerBus() const {
                return this->device_code == DeviceCode_I2c5;
            }
        };

        #include "i2c_bus_device_map.inc"

        void CheckSpeedMode(SpeedMode speed_mode) {
            switch (speed_mode) {
                case SpeedMode_Standard:  break;
                case SpeedMode_Fast:      break;
                case SpeedMode_FastPlus:  break;
                case SpeedMode_HighSpeed: break;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

        void Initialize(impl::I2cBusAccessorManager &bus_manager, impl::I2cDevicePropertyManager &device_manager) {
            /* Create an accessor for each bus. */
            for (const auto &bus_def : I2cBusList) {
                /* Check that the speed mode is valid. */
                CheckSpeedMode(bus_def.speed_mode);

                /* Find the bus. */
                auto *bus = bus_manager.Find([&bus_def](const auto &it) {
                    return it.GetRegistersPhysicalAddress() == bus_def.registers_phys_addr;
                });

                /* If the bus doesn't exist, create it. */
                if (bus == nullptr) {
                    /* Allocate the bus. */
                    bus = bus_manager.Allocate();

                    /* Initialize the bus. */
                    bus->Initialize(bus_def.registers_phys_addr, bus_def.registers_size, bus_def.interrupt_name, bus_def.IsPowerBus(), bus_def.speed_mode);

                    /* Register the bus. */
                    i2c::driver::RegisterDriver(bus);
                }

                /* Set the bus's device code. */
                bus->RegisterDeviceCode(bus_def.device_code);

                /* Allocate and register the devices for the bus. */
                for (size_t i = 0; i < bus_def.num_devices; ++i) {
                    /* Get the device definition. */
                    const auto &entry = bus_def.devices[i];

                    /* Allocate the device. */
                    I2cDeviceProperty *device = device_manager.Allocate(entry.slave_address, AddressingMode_SevenBit);

                    /* Register the device with our bus. */
                    bus->RegisterDevice(device);

                    /* Register the device code with our driver. */
                    R_ABORT_UNLESS(i2c::driver::RegisterDeviceCode(entry.device_code, device));
                }
            }
        }

    }

    void Initialize() {
        /* TODO: Should these be moved into getters? They're only used here, and they never destruct. */
        static impl::I2cBusAccessorManager s_bus_accessor_manager(ddsf::GetMemoryResource());
        static impl::I2cDevicePropertyManager s_device_manager(ddsf::GetMemoryResource());

        return Initialize(s_bus_accessor_manager, s_device_manager);
    }

}
