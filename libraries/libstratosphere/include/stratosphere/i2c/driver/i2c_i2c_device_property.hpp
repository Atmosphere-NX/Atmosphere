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
#include <vapours.hpp>
#include <stratosphere/i2c/i2c_types.hpp>
#include <stratosphere/ddsf.hpp>

namespace ams::i2c::driver {

    class I2cDeviceProperty : public ::ams::ddsf::IDevice {
        NON_COPYABLE(I2cDeviceProperty);
        NON_MOVEABLE(I2cDeviceProperty);
        AMS_DDSF_CASTABLE_TRAITS(ams::i2c::driver::I2cDeviceProperty, ::ams::ddsf::IDevice);
        private:
            u16 address;
            AddressingMode addressing_mode;
            util::IntrusiveListNode device_property_list_node;
        public:
            using DevicePropertyListTraits = util::IntrusiveListMemberTraitsDeferredAssert<&I2cDeviceProperty::device_property_list_node>;
            using DevicePropertyList       = typename DevicePropertyListTraits::ListType;
            friend class util::IntrusiveList<I2cDeviceProperty, util::IntrusiveListMemberTraitsDeferredAssert<&I2cDeviceProperty::device_property_list_node>>;
        public:
            I2cDeviceProperty() : IDevice(false), address(0), addressing_mode(AddressingMode_SevenBit), device_property_list_node() { /* ... */ }
            I2cDeviceProperty(u16 addr, AddressingMode m) : IDevice(false), address(addr), addressing_mode(m), device_property_list_node() { /* ... */ }

            virtual ~I2cDeviceProperty() { /* ... */ }

            u16 GetAddress() const {
                return this->address;
            }

            AddressingMode GetAddressingMode() const {
                return this->addressing_mode;
            }
    };

}
