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
#include "../../i2c_types.hpp"

namespace ams::i2c::driver::impl {

    enum class Command {
        Send    = 0,
        Receive = 1,
    };

    enum class Bus {
        I2C1 = 0,
        I2C2 = 1,
        I2C3 = 2,
        I2C4 = 3,
        I2C5 = 4,
        I2C6 = 5,
        Count,
    };

    /* Bus helpers. */
    constexpr inline size_t ConvertToIndex(Bus bus) {
        return static_cast<size_t>(bus);
    }

    constexpr inline Bus ConvertFromIndex(size_t idx) {
        AMS_ABORT_UNLESS(idx < static_cast<size_t>(Bus::Count));
        return static_cast<Bus>(idx);
    }

    constexpr inline PcvModule ConvertToPcvModule(Bus bus) {
        switch (bus) {
            case Bus::I2C1:
                return PcvModule_I2C1;
            case Bus::I2C2:
                return PcvModule_I2C2;
            case Bus::I2C3:
                return PcvModule_I2C3;
            case Bus::I2C4:
                return PcvModule_I2C4;
            case Bus::I2C5:
                return PcvModule_I2C5;
            case Bus::I2C6:
                return PcvModule_I2C6;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    constexpr inline Bus ConvertFromPcvModule(PcvModule module) {
        switch (module) {
            case PcvModule_I2C1:
                return Bus::I2C1;
            case PcvModule_I2C2:
                return Bus::I2C2;
            case PcvModule_I2C3:
                return Bus::I2C3;
            case PcvModule_I2C4:
                return Bus::I2C4;
            case PcvModule_I2C5:
                return Bus::I2C5;
            case PcvModule_I2C6:
                return Bus::I2C6;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    /* Global type functions. */
    bool IsDeviceSupported(I2cDevice dev);
    Bus GetDeviceBus(I2cDevice dev);
    u32 GetDeviceSlaveAddress(I2cDevice dev);
    AddressingMode GetDeviceAddressingMode(I2cDevice dev);
    SpeedMode GetDeviceSpeedMode(I2cDevice dev);
    u32 GetDeviceMaxRetries(I2cDevice dev);
    u64 GetDeviceRetryWaitTime(I2cDevice dev);

}
