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
#include <stratosphere/gpio/gpio_types.hpp>

namespace ams::gpio {

    enum GpioPadName : u32 {
        GpioPadName_EnableSdCardPower = 2,
    };
    constexpr inline const DeviceCode DeviceCode_EnableSdCardPower = 0x3C000001;

    constexpr inline GpioPadName ConvertToGpioPadName(DeviceCode dc) {
        switch (dc.GetInternalValue()) {
            case DeviceCode_EnableSdCardPower.GetInternalValue(): return GpioPadName_EnableSdCardPower;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    constexpr inline DeviceCode ConvertToDeviceCode(GpioPadName gpn) {
        switch (gpn) {
            case GpioPadName_EnableSdCardPower: return DeviceCode_EnableSdCardPower;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

}
