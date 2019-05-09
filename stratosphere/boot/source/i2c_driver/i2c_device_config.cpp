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

#include <switch.h>
#include <stratosphere.hpp>

#include "i2c_types.hpp"

struct DeviceConfig {
    I2cDevice device;
    I2cBus bus;
    u32 slave_address;
    AddressingMode addressing_mode;
    SpeedMode speed_mode;
    u32 max_retries;
    u64 retry_wait_time;
};

static constexpr DeviceConfig g_device_configs[I2cDevice_Count] = {
    {I2cDevice_DebugPad,        I2cBus_I2c1, 0x52, AddressingMode_7Bit, SpeedMode_Normal, 0, 0},
    {I2cDevice_TouchPanel,      I2cBus_I2c3, 0x49, AddressingMode_7Bit, SpeedMode_Fast,   0, 0},
    {I2cDevice_Tmp451,          I2cBus_I2c1, 0x4c, AddressingMode_7Bit, SpeedMode_Normal, 0, 0},
    {I2cDevice_Nct72,           I2cBus_I2c1, 0x4c, AddressingMode_7Bit, SpeedMode_Normal, 0, 0},
    {I2cDevice_Alc5639,         I2cBus_I2c1, 0x1c, AddressingMode_7Bit, SpeedMode_Normal, 0, 0},
    {I2cDevice_Max77620Rtc,     I2cBus_I2c5, 0x68, AddressingMode_7Bit, SpeedMode_Fast,   3, 5'000'000},
    {I2cDevice_Max77620Pmic,    I2cBus_I2c5, 0x3c, AddressingMode_7Bit, SpeedMode_Fast,   3, 5'000'000},
    {I2cDevice_Max77621Cpu,     I2cBus_I2c5, 0x1b, AddressingMode_7Bit, SpeedMode_Fast,   3, 5'000'000},
    {I2cDevice_Max77621Gpu,     I2cBus_I2c5, 0x1c, AddressingMode_7Bit, SpeedMode_Fast,   3, 5'000'000},
    {I2cDevice_Bq24193,         I2cBus_I2c1, 0x6b, AddressingMode_7Bit, SpeedMode_Normal, 3, 5'000'000},
    {I2cDevice_Max17050,        I2cBus_I2c1, 0x36, AddressingMode_7Bit, SpeedMode_Normal, 3, 5'000'000},
    {I2cDevice_Bm92t30mwv,      I2cBus_I2c1, 0x18, AddressingMode_7Bit, SpeedMode_Normal, 3, 5'000'000},
    {I2cDevice_Ina226Vdd15v0Hb, I2cBus_I2c2, 0x40, AddressingMode_7Bit, SpeedMode_Fast,   3, 5'000'000},
    {I2cDevice_Ina226VsysCpuDs, I2cBus_I2c2, 0x41, AddressingMode_7Bit, SpeedMode_Fast,   3, 5'000'000},
    {I2cDevice_Ina226VsysGpuDs, I2cBus_I2c2, 0x44, AddressingMode_7Bit, SpeedMode_Fast,   3, 5'000'000},
    {I2cDevice_Ina226VsysDdrDs, I2cBus_I2c2, 0x45, AddressingMode_7Bit, SpeedMode_Fast,   3, 5'000'000},
    {I2cDevice_Ina226VsysAp,    I2cBus_I2c2, 0x46, AddressingMode_7Bit, SpeedMode_Fast,   3, 5'000'000},
    {I2cDevice_Ina226VsysBlDs,  I2cBus_I2c2, 0x47, AddressingMode_7Bit, SpeedMode_Fast,   3, 5'000'000},
    {I2cDevice_Bh1730,          I2cBus_I2c2, 0x29, AddressingMode_7Bit, SpeedMode_Fast,   3, 5'000'000},
    {I2cDevice_Ina226VsysCore,  I2cBus_I2c2, 0x48, AddressingMode_7Bit, SpeedMode_Fast,   3, 5'000'000},
    {I2cDevice_Ina226Soc1V8,    I2cBus_I2c2, 0x49, AddressingMode_7Bit, SpeedMode_Fast,   3, 5'000'000},
    {I2cDevice_Ina226Lpddr1V8,  I2cBus_I2c2, 0x4a, AddressingMode_7Bit, SpeedMode_Fast,   3, 5'000'000},
    {I2cDevice_Ina226Reg1V32,   I2cBus_I2c2, 0x4b, AddressingMode_7Bit, SpeedMode_Fast,   3, 5'000'000},
    {I2cDevice_Ina226Vdd3V3Sys, I2cBus_I2c2, 0x4d, AddressingMode_7Bit, SpeedMode_Fast,   3, 5'000'000},
    {I2cDevice_HdmiDdc,         I2cBus_I2c4, 0x50, AddressingMode_7Bit, SpeedMode_Normal, 0, 0},
    {I2cDevice_HdmiScdc,        I2cBus_I2c4, 0x54, AddressingMode_7Bit, SpeedMode_Normal, 0, 0},
    {I2cDevice_HdmiHdcp,        I2cBus_I2c4, 0x3a, AddressingMode_7Bit, SpeedMode_Normal, 0, 0},
    {I2cDevice_Fan53528,        I2cBus_I2c5, 0xa4, AddressingMode_7Bit, SpeedMode_Fast,   0, 0},
    {I2cDevice_Max77812_3,      I2cBus_I2c5, 0x31, AddressingMode_7Bit, SpeedMode_Fast,   0, 0},
    {I2cDevice_Max77812_2,      I2cBus_I2c5, 0x33, AddressingMode_7Bit, SpeedMode_Fast,   0, 0},
    {I2cDevice_Ina226VddDdr0V6, I2cBus_I2c2, 0x4e, AddressingMode_7Bit, SpeedMode_Fast,   3, 5'000'000},
};

static constexpr size_t NumDeviceConfigs = sizeof(g_device_configs) / sizeof(g_device_configs[0]);

static constexpr size_t I2cDeviceInvalidIndex = static_cast<size_t>(-1);

static size_t GetI2cDeviceIndex(I2cDevice dev) {
    for (size_t i = 0; i < NumDeviceConfigs; i++) {
        if (g_device_configs[i].device == dev) {
            return i;
        }
    }
    return I2cDeviceInvalidIndex;
}

bool IsI2cDeviceSupported(I2cDevice dev) {
    return GetI2cDeviceIndex(dev) != I2cDeviceInvalidIndex;
}

I2cBus GetI2cDeviceBus(I2cDevice dev) {
    const size_t dev_idx = GetI2cDeviceIndex(dev);
    if (dev_idx == I2cDeviceInvalidIndex) { std::abort(); }
    return g_device_configs[dev_idx].bus;
}

u32 GetI2cDeviceSlaveAddress(I2cDevice dev) {
    const size_t dev_idx = GetI2cDeviceIndex(dev);
    if (dev_idx == I2cDeviceInvalidIndex) { std::abort(); }
    return g_device_configs[dev_idx].slave_address;
}

AddressingMode GetI2cDeviceAddressingMode(I2cDevice dev) {
    const size_t dev_idx = GetI2cDeviceIndex(dev);
    if (dev_idx == I2cDeviceInvalidIndex) { std::abort(); }
    return g_device_configs[dev_idx].addressing_mode;
}

SpeedMode GetI2cDeviceSpeedMode(I2cDevice dev) {
    const size_t dev_idx = GetI2cDeviceIndex(dev);
    if (dev_idx == I2cDeviceInvalidIndex) { std::abort(); }
    return g_device_configs[dev_idx].speed_mode;
}

u32 GetI2cDeviceMaxRetries(I2cDevice dev) {
    const size_t dev_idx = GetI2cDeviceIndex(dev);
    if (dev_idx == I2cDeviceInvalidIndex) { std::abort(); }
    return g_device_configs[dev_idx].max_retries;
}

u64 GetI2cDeviceRetryWaitTime(I2cDevice dev) {
    const size_t dev_idx = GetI2cDeviceIndex(dev);
    if (dev_idx == I2cDeviceInvalidIndex) { std::abort(); }
    return g_device_configs[dev_idx].retry_wait_time;
}
