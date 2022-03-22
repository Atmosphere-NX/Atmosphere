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

namespace ams::psc {

    enum PmModuleId : u32 {
        PmModuleId_Usb           = 4,
        PmModuleId_Ethernet      = 5,
        PmModuleId_Fgm           = 6,
        PmModuleId_PcvClock      = 7,
        PmModuleId_PcvVoltage    = 8,
        PmModuleId_Gpio          = 9,
        PmModuleId_Pinmux        = 10,
        PmModuleId_Uart          = 11,
        PmModuleId_I2c           = 12,
        PmModuleId_I2cPcv        = 13,
        PmModuleId_Spi           = 14,
        PmModuleId_Pwm           = 15,
        PmModuleId_Psm           = 16,
        PmModuleId_Tc            = 17,
        PmModuleId_Omm           = 18,
        PmModuleId_Pcie          = 19,
        PmModuleId_Lbl           = 20,
        PmModuleId_Display       = 21,

        PmModuleId_Hid           = 24,
        PmModuleId_WlanSockets   = 25,

        PmModuleId_Fs            = 27,
        PmModuleId_Audio         = 28,

        PmModuleId_TmaHostIo     = 30,
        PmModuleId_Bluetooth     = 31,
        PmModuleId_Bpc           = 32,
        PmModuleId_Fan           = 33,
        PmModuleId_Pcm           = 34,
        PmModuleId_Nfc           = 35,
        PmModuleId_Apm           = 36,
        PmModuleId_Btm           = 37,
        PmModuleId_Nifm          = 38,
        PmModuleId_GpioLow       = 39,
        PmModuleId_Npns          = 40,
        PmModuleId_Lm            = 41,
        PmModuleId_Bcat          = 42,
        PmModuleId_Time          = 43,
        PmModuleId_Pctl          = 44,
        PmModuleId_Erpt          = 45,
        PmModuleId_Eupld         = 46,
        PmModuleId_Friends       = 47,
        PmModuleId_Bgtc          = 48,
        PmModuleId_Account       = 49,
        PmModuleId_Sasbus        = 50,
        PmModuleId_Ntc           = 51,
        PmModuleId_Idle          = 52,
        PmModuleId_Tcap          = 53,
        PmModuleId_PsmLow        = 54,
        PmModuleId_Ndd           = 55,
        PmModuleId_Olsc          = 56,

        PmModuleId_Ns            = 61,

        PmModuleId_Nvservices    = 101,

        PmModuleId_Spsm          = 127,
    };

}
