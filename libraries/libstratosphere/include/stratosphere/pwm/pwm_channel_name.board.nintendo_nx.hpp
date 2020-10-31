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
#include <stratosphere/pwm/pwm_types.hpp>

namespace ams::pwm {

    enum ChannelName {
        ChannelName_Invalid      = 0,
        ChannelName_CpuFan       = 1,
        ChannelName_LcdBacklight = 2,
        ChannelName_BlinkLed     = 3,
    };

    constexpr inline const DeviceCode DeviceCode_CpuFan       = 0x3D000001;
    constexpr inline const DeviceCode DeviceCode_LcdBacklight = 0x3400003D;
    constexpr inline const DeviceCode DeviceCode_BlinkLed     = 0x35000065;

    constexpr inline DeviceCode ConvertToDeviceCode(ChannelName cn) {
        switch (cn) {
            case ChannelName_CpuFan:       return DeviceCode_CpuFan;
            case ChannelName_LcdBacklight: return DeviceCode_LcdBacklight;
            case ChannelName_BlinkLed:     return DeviceCode_BlinkLed;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    constexpr inline ChannelName ConvertToChannelName(DeviceCode dc) {
        switch (dc.GetInternalValue()) {
            case DeviceCode_CpuFan      .GetInternalValue(): return ChannelName_CpuFan;
            case DeviceCode_LcdBacklight.GetInternalValue(): return ChannelName_LcdBacklight;
            case DeviceCode_BlinkLed    .GetInternalValue(): return ChannelName_BlinkLed;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

}
