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
#include <vapours.hpp>
#include <stratosphere/i2c/i2c_types.hpp>

namespace ams::i2c {

    enum I2cBus {
        I2cBus_I2c1 = 0,
    };

    constexpr inline const DeviceCode DeviceCode_I2c1 = 0x02000001;

    constexpr inline DeviceCode ConvertToDeviceCode(I2cBus bus) {
        switch (bus) {
            case I2cBus_I2c1: return DeviceCode_I2c1;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    constexpr inline DeviceCode ConvertToI2cBus(DeviceCode dc) {
        switch (dc.GetInternalValue()) {
            case DeviceCode_I2c1.GetInternalValue(): return I2cBus_I2c1;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    enum I2cDevice : u32 {
        I2cDevice_ClassicController =  0,
    };

    /* TODO: Better place for this? */
    constexpr inline const DeviceCode DeviceCode_ClassicController = 0x350000C9;

    constexpr inline DeviceCode ConvertToDeviceCode(I2cDevice dv) {
        switch (dv) {
            case I2cDevice_ClassicController: return DeviceCode_ClassicController;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    constexpr inline I2cDevice ConvertToI2cDevice(DeviceCode dc) {
        switch (dc.GetInternalValue()) {
            case DeviceCode_ClassicController.GetInternalValue(): return I2cDevice_ClassicController;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    constexpr bool IsPowerBusDeviceCode(DeviceCode device_code) {
        switch (device_code.GetInternalValue()) {
            default:
                return false;
        }
    }

}
