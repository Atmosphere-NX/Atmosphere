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

namespace ams::wec {

    enum WakeEvent {
        WakeEvent_PexWakeN           = 0x00,
        WakeEvent_GpioPortA6         = 0x01,
        WakeEvent_QspiCsN            = 0x02,
        WakeEvent_Spi2Mosi           = 0x03,
        WakeEvent_ExtconDetS         = 0x04,
        WakeEvent_McuIrq             = 0x05,
        WakeEvent_Uart2Cts           = 0x06,
        WakeEvent_Uart3Cts           = 0x07,
        WakeEvent_WifiWakeAp         = 0x08,
        WakeEvent_AoTag2Pmc          = 0x09,
        WakeEvent_ExtconDetU         = 0x0A,
        WakeEvent_NfcInt             = 0x0B,
        WakeEvent_Gen1I2cSda         = 0x0C,
        WakeEvent_Gen2I2cSda         = 0x0D,
        WakeEvent_CradleIrq          = 0x0E,
        WakeEvent_GpioPortK6         = 0x0F,
        WakeEvent_RtcIrq             = 0x10,
        WakeEvent_Sdmmc1Dat1         = 0x11,
        WakeEvent_Sdmmc2Dat1         = 0x12,
        WakeEvent_HdmiCec            = 0x13,
        WakeEvent_Gen3I2cSda         = 0x14,
        WakeEvent_GpioPortL1         = 0x15,
        WakeEvent_Clk_32kOut         = 0x16,
        WakeEvent_PwrI2cSda          = 0x17,
        WakeEvent_ButtonPowerOn      = 0x18,
        WakeEvent_ButtonVolUp        = 0x19,
        WakeEvent_ButtonVolDown      = 0x1A,
        WakeEvent_ButtonSlideSw      = 0x1B,
        WakeEvent_ButtonHome         = 0x1C,
        /* ... */
        WakeEvent_AlsProxInt         = 0x20,
        WakeEvent_TempAlert          = 0x21,
        WakeEvent_Bq24190Irq         = 0x22,
        WakeEvent_SdCd               = 0x23,
        WakeEvent_GpioPortZ2         = 0x24,
        /* ... */
        WakeEvent_Utmip0             = 0x27,
        WakeEvent_Utmip1             = 0x28,
        WakeEvent_Utmip2             = 0x29,
        WakeEvent_Utmip3             = 0x2A,
        WakeEvent_Uhsic              = 0x2B,
        WakeEvent_Wake2PmcXusbSystem = 0x2C,
        WakeEvent_Sdmmc3Dat1         = 0x2D,
        WakeEvent_Sdmmc4Dat1         = 0x2E,
        WakeEvent_CamI2cScl          = 0x2F,
        WakeEvent_CamI2cSda          = 0x30,
        WakeEvent_GpioPortZ5         = 0x31,
        WakeEvent_DpHpd0             = 0x32,
        WakeEvent_PwrIntN            = 0x33,
        WakeEvent_BtWakeAp           = 0x34,
        WakeEvent_HdmiIntDpHpd       = 0x35,
        WakeEvent_UsbVbusEn0         = 0x36,
        WakeEvent_UsbVbusEn1         = 0x37,
        WakeEvent_LcdRst             = 0x38,
        WakeEvent_LcdGpio1           = 0x39,
        WakeEvent_LcdGpio2           = 0x3A,
        WakeEvent_Uart4Cts           = 0x3B,
        WakeEvent_ModemWakeAp        = 0x3D,
        WakeEvent_TouchInt           = 0x3E,
        WakeEvent_MotionInt          = 0x3F,

        WakeEvent_Count              = 0x40,
    };

    constexpr inline WakeEvent WakeEvent_None = static_cast<WakeEvent>(-1);

}
