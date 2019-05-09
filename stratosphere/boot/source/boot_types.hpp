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

#pragma once
#include <switch.h>
#include <stratosphere.hpp>

enum HardwareType {
    HardwareType_Icosa = 0,
    HardwareType_Copper = 1,
    HardwareType_Hoag = 2,
    HardwareType_Iowa = 3,
};

struct GpioInitialConfig {
    u32 pad_name;
    GpioDirection direction;
    GpioValue value;
};

struct PinmuxInitialConfig {
    u32 name;
    u32 val;
    u32 mask;
};

struct WakePinConfig {
    u32 index;
    bool enabled;
    u32 level;
};