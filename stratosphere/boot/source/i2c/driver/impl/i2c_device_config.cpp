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
#include "i2c_driver_types.hpp"

namespace ams::i2c::driver::impl {

    namespace {

        struct DeviceConfig {
            I2cDevice device;
            Bus bus;
            u32 slave_address;
            AddressingMode addressing_mode;
            SpeedMode speed_mode;
            u32 max_retries;
            u64 retry_wait_time;
        };

        constexpr DeviceConfig g_device_configs[I2cDevice_Count] = {
            {I2cDevice_DebugPad,        Bus::I2C1, 0x52, AddressingMode::SevenBit, SpeedMode::Normal, 0, 0},
            {I2cDevice_TouchPanel,      Bus::I2C3, 0x49, AddressingMode::SevenBit, SpeedMode::Fast,   0, 0},
            {I2cDevice_Tmp451,          Bus::I2C1, 0x4c, AddressingMode::SevenBit, SpeedMode::Normal, 0, 0},
            {I2cDevice_Nct72,           Bus::I2C1, 0x4c, AddressingMode::SevenBit, SpeedMode::Normal, 0, 0},
            {I2cDevice_Alc5639,         Bus::I2C1, 0x1c, AddressingMode::SevenBit, SpeedMode::Normal, 0, 0},
            {I2cDevice_Max77620Rtc,     Bus::I2C5, 0x68, AddressingMode::SevenBit, SpeedMode::Fast,   3, 5'000'000},
            {I2cDevice_Max77620Pmic,    Bus::I2C5, 0x3c, AddressingMode::SevenBit, SpeedMode::Fast,   3, 5'000'000},
            {I2cDevice_Max77621Cpu,     Bus::I2C5, 0x1b, AddressingMode::SevenBit, SpeedMode::Fast,   3, 5'000'000},
            {I2cDevice_Max77621Gpu,     Bus::I2C5, 0x1c, AddressingMode::SevenBit, SpeedMode::Fast,   3, 5'000'000},
            {I2cDevice_Bq24193,         Bus::I2C1, 0x6b, AddressingMode::SevenBit, SpeedMode::Normal, 3, 5'000'000},
            {I2cDevice_Max17050,        Bus::I2C1, 0x36, AddressingMode::SevenBit, SpeedMode::Normal, 3, 5'000'000},
            {I2cDevice_Bm92t30mwv,      Bus::I2C1, 0x18, AddressingMode::SevenBit, SpeedMode::Normal, 3, 5'000'000},
            {I2cDevice_Ina226Vdd15v0Hb, Bus::I2C2, 0x40, AddressingMode::SevenBit, SpeedMode::Fast,   3, 5'000'000},
            {I2cDevice_Ina226VsysCpuDs, Bus::I2C2, 0x41, AddressingMode::SevenBit, SpeedMode::Fast,   3, 5'000'000},
            {I2cDevice_Ina226VsysGpuDs, Bus::I2C2, 0x44, AddressingMode::SevenBit, SpeedMode::Fast,   3, 5'000'000},
            {I2cDevice_Ina226VsysDdrDs, Bus::I2C2, 0x45, AddressingMode::SevenBit, SpeedMode::Fast,   3, 5'000'000},
            {I2cDevice_Ina226VsysAp,    Bus::I2C2, 0x46, AddressingMode::SevenBit, SpeedMode::Fast,   3, 5'000'000},
            {I2cDevice_Ina226VsysBlDs,  Bus::I2C2, 0x47, AddressingMode::SevenBit, SpeedMode::Fast,   3, 5'000'000},
            {I2cDevice_Bh1730,          Bus::I2C2, 0x29, AddressingMode::SevenBit, SpeedMode::Fast,   3, 5'000'000},
            {I2cDevice_Ina226VsysCore,  Bus::I2C2, 0x48, AddressingMode::SevenBit, SpeedMode::Fast,   3, 5'000'000},
            {I2cDevice_Ina226Soc1V8,    Bus::I2C2, 0x49, AddressingMode::SevenBit, SpeedMode::Fast,   3, 5'000'000},
            {I2cDevice_Ina226Lpddr1V8,  Bus::I2C2, 0x4a, AddressingMode::SevenBit, SpeedMode::Fast,   3, 5'000'000},
            {I2cDevice_Ina226Reg1V32,   Bus::I2C2, 0x4b, AddressingMode::SevenBit, SpeedMode::Fast,   3, 5'000'000},
            {I2cDevice_Ina226Vdd3V3Sys, Bus::I2C2, 0x4d, AddressingMode::SevenBit, SpeedMode::Fast,   3, 5'000'000},
            {I2cDevice_HdmiDdc,         Bus::I2C4, 0x50, AddressingMode::SevenBit, SpeedMode::Normal, 0, 0},
            {I2cDevice_HdmiScdc,        Bus::I2C4, 0x54, AddressingMode::SevenBit, SpeedMode::Normal, 0, 0},
            {I2cDevice_HdmiHdcp,        Bus::I2C4, 0x3a, AddressingMode::SevenBit, SpeedMode::Normal, 0, 0},
            {I2cDevice_Fan53528,        Bus::I2C5, 0xa4, AddressingMode::SevenBit, SpeedMode::Fast,   0, 0},
            {I2cDevice_Max77812_3,      Bus::I2C5, 0x31, AddressingMode::SevenBit, SpeedMode::Fast,   0, 0},
            {I2cDevice_Max77812_2,      Bus::I2C5, 0x33, AddressingMode::SevenBit, SpeedMode::Fast,   0, 0},
            {I2cDevice_Ina226VddDdr0V6, Bus::I2C2, 0x4e, AddressingMode::SevenBit, SpeedMode::Fast,   3, 5'000'000},
        };

        constexpr size_t NumDeviceConfigs = util::size(g_device_configs);

        constexpr size_t DeviceInvalidIndex = static_cast<size_t>(-1);

        size_t GetDeviceIndex(I2cDevice dev) {
            for (size_t i = 0; i < NumDeviceConfigs; i++) {
                if (g_device_configs[i].device == dev) {
                    return i;
                }
            }
            return DeviceInvalidIndex;
        }

    }

    bool IsDeviceSupported(I2cDevice dev) {
        return GetDeviceIndex(dev) != DeviceInvalidIndex;
    }

    Bus GetDeviceBus(I2cDevice dev) {
        const size_t dev_idx = GetDeviceIndex(dev);
        AMS_ABORT_UNLESS(dev_idx != DeviceInvalidIndex);
        return g_device_configs[dev_idx].bus;
    }

    u32 GetDeviceSlaveAddress(I2cDevice dev) {
        const size_t dev_idx = GetDeviceIndex(dev);
        AMS_ABORT_UNLESS(dev_idx != DeviceInvalidIndex);
        return g_device_configs[dev_idx].slave_address;
    }

    AddressingMode GetDeviceAddressingMode(I2cDevice dev) {
        const size_t dev_idx = GetDeviceIndex(dev);
        AMS_ABORT_UNLESS(dev_idx != DeviceInvalidIndex);
        return g_device_configs[dev_idx].addressing_mode;
    }

    SpeedMode GetDeviceSpeedMode(I2cDevice dev) {
        const size_t dev_idx = GetDeviceIndex(dev);
        AMS_ABORT_UNLESS(dev_idx != DeviceInvalidIndex);
        return g_device_configs[dev_idx].speed_mode;
    }

    u32 GetDeviceMaxRetries(I2cDevice dev) {
        const size_t dev_idx = GetDeviceIndex(dev);
        AMS_ABORT_UNLESS(dev_idx != DeviceInvalidIndex);
        return g_device_configs[dev_idx].max_retries;
    }

    u64 GetDeviceRetryWaitTime(I2cDevice dev) {
        const size_t dev_idx = GetDeviceIndex(dev);
        AMS_ABORT_UNLESS(dev_idx != DeviceInvalidIndex);
        return g_device_configs[dev_idx].retry_wait_time;
    }

}
