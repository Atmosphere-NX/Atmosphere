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

enum AddressingMode {
    AddressingMode_7Bit = 0,
};

enum SpeedMode {
    SpeedMode_Normal    = 100000,
    SpeedMode_Fast      = 400000,
    SpeedMode_FastPlus  = 1000000,
    SpeedMode_HighSpeed = 3400000,
};

enum I2cBus {
    I2cBus_I2c1 = 0,
    I2cBus_I2c2 = 1,
    I2cBus_I2c3 = 2,
    I2cBus_I2c4 = 3,
    I2cBus_I2c5 = 4,
    I2cBus_I2c6 = 5,
};

enum DriverCommand {
    DriverCommand_Send = 0,
    DriverCommand_Receive = 1,
};

struct I2cSessionImpl {
    I2cBus bus;
    size_t session_id;
};

enum I2cCommand {
    I2cCommand_Send = 0,
    I2cCommand_Receive = 1,
    I2cCommand_SubCommand = 2,
    I2cCommand_Count,
};

enum I2cSubCommand {
    I2cSubCommand_Sleep = 0,
    I2cSubCommand_Count,
};

bool IsI2cDeviceSupported(I2cDevice dev);
I2cBus GetI2cDeviceBus(I2cDevice dev);
u32 GetI2cDeviceSlaveAddress(I2cDevice dev);
AddressingMode GetI2cDeviceAddressingMode(I2cDevice dev);
SpeedMode GetI2cDeviceSpeedMode(I2cDevice dev);
u32 GetI2cDeviceMaxRetries(I2cDevice dev);
u64 GetI2cDeviceRetryWaitTime(I2cDevice dev);
