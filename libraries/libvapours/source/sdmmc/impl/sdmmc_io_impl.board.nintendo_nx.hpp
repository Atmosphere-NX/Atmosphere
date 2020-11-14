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

namespace ams::sdmmc::impl {

    Result SetSdCardVoltageEnabled(bool en);
    Result SetSdCardVoltageValue(u32 micro_volts);

    namespace pinmux_impl {

        enum PinAssignment {
            PinAssignment_Sdmmc1OutputHigh   = 2,
            PinAssignment_Sdmmc1ResetState   = 3,
            PinAssignment_Sdmmc1SchmtEnable  = 4,
            PinAssignment_Sdmmc1SchmtDisable = 5,
        };

        void SetPinAssignment(PinAssignment assignment);

    }

    namespace gpio_impl {

        enum GpioValue {
            GpioValue_Low  = 0,
            GpioValue_High = 1
        };

        enum Direction {
            Direction_Input  = 0,
            Direction_Output = 1,
        };

        enum GpioPadName {
            GpioPadName_PowSdEn = 2,
        };

        void OpenSession(GpioPadName pad);
        void CloseSession(GpioPadName pad);

        void SetDirection(GpioPadName pad, Direction direction);
        void SetValue(GpioPadName pad, GpioValue value);


    }

}
