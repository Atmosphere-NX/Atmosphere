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
#include <stratosphere/pwm/pwm_types.hpp>

namespace ams::pwm {

    enum ChannelName {
        ChannelName_Invalid      = 0,
    };

    constexpr inline DeviceCode ConvertToDeviceCode(ChannelName cn) {
        switch (cn) {
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
        return InvalidDeviceCode;
    }

    constexpr inline ChannelName ConvertToChannelName(DeviceCode dc) {
        switch (dc.GetInternalValue()) {
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
        return ChannelName_Invalid;
    }

}
