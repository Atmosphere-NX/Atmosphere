/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

#include "boot_functions.hpp"

static constexpr u32 GpioPadName_FanEnable = 0x4B;

void Boot::SetFanEnabled() {
    if (Boot::GetHardwareType() == HardwareType_Copper) {
        Boot::GpioConfigure(GpioPadName_FanEnable);
        Boot::GpioSetDirection(GpioPadName_FanEnable, GpioDirection_Output);
        Boot::GpioSetValue(GpioPadName_FanEnable, GpioValue_High);
    }
}
