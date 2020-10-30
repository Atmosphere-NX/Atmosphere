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
#include <stratosphere/gpio/gpio_types.hpp>

namespace ams::gpio {

    enum GpioPadName : u32 {
        GpioPadName_CodecLdoEnTemp =  1,
        GpioPadName_ButtonVolUp    = 25,
        GpioPadName_ButtonVolDn    = 26,
        GpioPadName_SdCd           = 56,
    };

    /* TODO: Better place for this? */
    constexpr inline const DeviceCode DeviceCode_CodecLdoEnTemp = 0x33000002;
    constexpr inline const DeviceCode DeviceCode_ButtonVolUp    = 0x35000002;
    constexpr inline const DeviceCode DeviceCode_ButtonVolDn    = 0x35000003;
    constexpr inline const DeviceCode DeviceCode_SdCd           = 0x3C000002;

    constexpr inline GpioPadName ConvertToGpioPadName(DeviceCode dc) {
        switch (dc.GetInternalValue()) {
            case DeviceCode_CodecLdoEnTemp.GetInternalValue(): return GpioPadName_CodecLdoEnTemp;
            case DeviceCode_ButtonVolUp   .GetInternalValue(): return GpioPadName_ButtonVolUp;
            case DeviceCode_ButtonVolDn   .GetInternalValue(): return GpioPadName_ButtonVolDn;
            case DeviceCode_SdCd          .GetInternalValue(): return GpioPadName_SdCd;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    constexpr inline DeviceCode ConvertToDeviceCode(GpioPadName gpn) {
        switch (gpn) {
            case GpioPadName_CodecLdoEnTemp: return DeviceCode_CodecLdoEnTemp;
            case GpioPadName_ButtonVolUp:    return DeviceCode_ButtonVolUp;
            case GpioPadName_ButtonVolDn:    return DeviceCode_ButtonVolDn;
            case GpioPadName_SdCd:           return DeviceCode_SdCd;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

}
