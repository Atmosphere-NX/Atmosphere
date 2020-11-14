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
#include <stratosphere/gpio/gpio_types.hpp>

namespace ams::gpio {

    enum GpioPadName : u32 {
        GpioPadName_CodecLdoEnTemp     = 1,
        GpioPadName_PowSdEn            = 2,
        GpioPadName_BtRst              = 3,
        GpioPadName_RamCode3           = 4,
        GpioPadName_GameCardReset      = 5,
        GpioPadName_CodecAlert         = 6,
        GpioPadName_PowGc              = 7,
        GpioPadName_DebugControllerDet = 8,
        GpioPadName_BattChgStatus      = 9,
        GpioPadName_BattChgEnableN     = 10,
        GpioPadName_FanTach            = 11,
        GpioPadName_ExtconDetS         = 12,
        GpioPadName_Vdd50AEn           = 13,
        GpioPadName_SdevCoaxSel1       = 14,
        GpioPadName_GameCardCd         = 15,
        GpioPadName_ProdType0          = 16,
        GpioPadName_ProdType1          = 17,
        GpioPadName_ProdType2          = 18,
        GpioPadName_ProdType3          = 19,
        GpioPadName_TempAlert          = 20,
        GpioPadName_CodecHpDetIrq      = 21,
        GpioPadName_MotionInt          = 22,
        GpioPadName_TpIrq              = 23,
        GpioPadName_ButtonSleep2       = 24,
        GpioPadName_ButtonVolUp        = 25,
        GpioPadName_ButtonVolDn        = 26,
        GpioPadName_BattMgicIrq        = 27,
        GpioPadName_RecoveryKey        = 28,
        GpioPadName_PowLcdBlEn         = 29,
        GpioPadName_LcdReset           = 30,
        GpioPadName_PdVconnEn          = 31,
        GpioPadName_PdRstN             = 32,
        GpioPadName_Bq24190Irq         = 33,
        GpioPadName_SdevCoaxSel0       = 34,
        GpioPadName_SdWp               = 35,
        GpioPadName_TpReset            = 36,
        GpioPadName_BtGpio2            = 37,
        GpioPadName_BtGpio3            = 38,
        GpioPadName_BtGpio4            = 39,
        GpioPadName_CradleIrq          = 40,
        GpioPadName_PowVcpuInt         = 41,
        GpioPadName_Max77621GpuInt     = 42,
        GpioPadName_ExtconChgU         = 43,
        GpioPadName_ExtconChgS         = 44,
        GpioPadName_WifiRfDisable      = 45,
        GpioPadName_WifiReset          = 46,
        GpioPadName_ApWakeBt           = 47,
        GpioPadName_BtWakeAp           = 48,
        GpioPadName_BtGpio5            = 49,
        GpioPadName_PowLcdVddPEn       = 50,
        GpioPadName_PowLcdVddNEn       = 51,
        GpioPadName_ExtconDetU         = 52,
        GpioPadName_RamCode2           = 53,
        GpioPadName_Vdd50BEn           = 54,
        GpioPadName_WifiWakeHost       = 55,
        GpioPadName_SdCd               = 56,
        GpioPadName_OtgFet1ForSdev     = 57,
        GpioPadName_OtgFet2ForSdev     = 58,
        GpioPadName_ExtConWakeU        = 59,
        GpioPadName_ExtConWakeS        = 60,
        GpioPadName_PmuIrq             = 61,
        GpioPadName_ExtUart2Cts        = 62,
        GpioPadName_ExtUart3Cts        = 63,
        GpioPadName_5VStepDownEn       = 64,
        GpioPadName_UsbSwitchB2Oc      = 65,
        GpioPadName_5VStepDownPg       = 66,
        GpioPadName_UsbSwitchAEn       = 67,
        GpioPadName_UsbSwitchAFlag     = 68,
        GpioPadName_UsbSwitchB3Oc      = 69,
        GpioPadName_UsbSwitchB3En      = 70,
        GpioPadName_UsbSwitchB2En      = 71,
        GpioPadName_Hdmi5VEn           = 72,
        GpioPadName_UsbSwitchB1En      = 73,
        GpioPadName_HdmiPdTrEn         = 74,
        GpioPadName_FanEn              = 75,
        GpioPadName_UsbSwitchB1Oc      = 76,
        GpioPadName_PwmFan             = 77,
        GpioPadName_HdmiHpd            = 78,
        GpioPadName_Max77812Irq        = 79,
        GpioPadName_Debug0             = 80,
        GpioPadName_Debug1             = 81,
        GpioPadName_Debug2             = 82,
        GpioPadName_Debug3             = 83,
        GpioPadName_NfcIrq             = 84,
        GpioPadName_NfcRst             = 85,
        GpioPadName_McuIrq             = 86,
        GpioPadName_McuBoot            = 87,
        GpioPadName_McuRst             = 88,
        GpioPadName_Vdd5V3En           = 89,
        GpioPadName_McuPor             = 90,
        GpioPadName_LcdGpio1           = 91,
        GpioPadName_NfcEn              = 92,
    };

    /* TODO: Better place for this? */
    constexpr inline const DeviceCode DeviceCode_CodecLdoEnTemp     = 0x33000002;
    constexpr inline const DeviceCode DeviceCode_PowSdEn            = 0x3C000001;
    constexpr inline const DeviceCode DeviceCode_BtRst              = 0x37000002;
    constexpr inline const DeviceCode DeviceCode_RamCode3           = 0xC9000402;
    constexpr inline const DeviceCode DeviceCode_GameCardReset      = 0x3C000402;
    constexpr inline const DeviceCode DeviceCode_CodecAlert         = 0x33000003;
    constexpr inline const DeviceCode DeviceCode_PowGc              = 0x3C000401;
    constexpr inline const DeviceCode DeviceCode_DebugControllerDet = 0x350000CA;
    constexpr inline const DeviceCode DeviceCode_BattChgStatus      = 0x39000407;
    constexpr inline const DeviceCode DeviceCode_BattChgEnableN     = 0x39000003;
    constexpr inline const DeviceCode DeviceCode_FanTach            = 0x3D000002;
    constexpr inline const DeviceCode DeviceCode_ExtconDetS         = 0x3500040B;
    constexpr inline const DeviceCode DeviceCode_Vdd50AEn           = 0x39000401;
    constexpr inline const DeviceCode DeviceCode_SdevCoaxSel1       = 0xCA000402;
    constexpr inline const DeviceCode DeviceCode_GameCardCd         = 0x3C000403;
    constexpr inline const DeviceCode DeviceCode_ProdType0          = 0xC900040B;
    constexpr inline const DeviceCode DeviceCode_ProdType1          = 0xC900040C;
    constexpr inline const DeviceCode DeviceCode_ProdType2          = 0xC900040D;
    constexpr inline const DeviceCode DeviceCode_ProdType3          = 0xC900040E;
    constexpr inline const DeviceCode DeviceCode_TempAlert          = 0x3E000002;
    constexpr inline const DeviceCode DeviceCode_CodecHpDetIrq      = 0x33000004;
    constexpr inline const DeviceCode DeviceCode_MotionInt          = 0x35000041;
    constexpr inline const DeviceCode DeviceCode_TpIrq              = 0x35000036;
    constexpr inline const DeviceCode DeviceCode_ButtonSleep2       = 0x35000001;
    constexpr inline const DeviceCode DeviceCode_ButtonVolUp        = 0x35000002;
    constexpr inline const DeviceCode DeviceCode_ButtonVolDn        = 0x35000003;
    constexpr inline const DeviceCode DeviceCode_BattMgicIrq        = 0x39000034;
    constexpr inline const DeviceCode DeviceCode_RecoveryKey        = 0x35000004;
    constexpr inline const DeviceCode DeviceCode_PowLcdBlEn         = 0x3400003E;
    constexpr inline const DeviceCode DeviceCode_LcdReset           = 0x34000033;
    constexpr inline const DeviceCode DeviceCode_PdVconnEn          = 0x040000CC;
    constexpr inline const DeviceCode DeviceCode_PdRstN             = 0x040000CA;
    constexpr inline const DeviceCode DeviceCode_Bq24190Irq         = 0x39000002;
    constexpr inline const DeviceCode DeviceCode_SdevCoaxSel0       = 0xCA000401;
    constexpr inline const DeviceCode DeviceCode_SdWp               = 0x3C000003;
    constexpr inline const DeviceCode DeviceCode_TpReset            = 0x35000035;
    constexpr inline const DeviceCode DeviceCode_BtGpio2            = 0x37000401;
    constexpr inline const DeviceCode DeviceCode_BtGpio3            = 0x37000402;
    constexpr inline const DeviceCode DeviceCode_BtGpio4            = 0x37000403;
    constexpr inline const DeviceCode DeviceCode_CradleIrq          = 0x040000CB;
    constexpr inline const DeviceCode DeviceCode_PowVcpuInt         = 0x3E000003;
    constexpr inline const DeviceCode DeviceCode_Max77621GpuInt     = 0x3E000004;
    constexpr inline const DeviceCode DeviceCode_ExtconChgU         = 0x35000402;
    constexpr inline const DeviceCode DeviceCode_ExtconChgS         = 0x3500040C;
    constexpr inline const DeviceCode DeviceCode_WifiRfDisable      = 0x38000003;
    constexpr inline const DeviceCode DeviceCode_WifiReset          = 0x38000002;
    constexpr inline const DeviceCode DeviceCode_ApWakeBt           = 0x37000003;
    constexpr inline const DeviceCode DeviceCode_BtWakeAp           = 0x37000004;
    constexpr inline const DeviceCode DeviceCode_BtGpio5            = 0x37000404;
    constexpr inline const DeviceCode DeviceCode_PowLcdVddPEn       = 0x34000034;
    constexpr inline const DeviceCode DeviceCode_PowLcdVddNEn       = 0x34000035;
    constexpr inline const DeviceCode DeviceCode_ExtconDetU         = 0x35000401;
    constexpr inline const DeviceCode DeviceCode_RamCode2           = 0xC9000401;
    constexpr inline const DeviceCode DeviceCode_Vdd50BEn           = 0x39000402;
    constexpr inline const DeviceCode DeviceCode_WifiWakeHost       = 0x38000004;
    constexpr inline const DeviceCode DeviceCode_SdCd               = 0x3C000002;
    constexpr inline const DeviceCode DeviceCode_OtgFet1ForSdev     = 0x39000404;
    constexpr inline const DeviceCode DeviceCode_OtgFet2ForSdev     = 0x39000405;
    constexpr inline const DeviceCode DeviceCode_ExtConWakeU        = 0x35000403;
    constexpr inline const DeviceCode DeviceCode_ExtConWakeS        = 0x3500040D;
    constexpr inline const DeviceCode DeviceCode_PmuIrq             = 0x39000406;
    constexpr inline const DeviceCode DeviceCode_ExtUart2Cts        = 0x35000404;
    constexpr inline const DeviceCode DeviceCode_ExtUart3Cts        = 0x3500040E;
    constexpr inline const DeviceCode DeviceCode_5VStepDownEn       = 0x39000408;
    constexpr inline const DeviceCode DeviceCode_UsbSwitchB2Oc      = 0x04000401;
    constexpr inline const DeviceCode DeviceCode_5VStepDownPg       = 0x39000409;
    constexpr inline const DeviceCode DeviceCode_UsbSwitchAEn       = 0x04000402;
    constexpr inline const DeviceCode DeviceCode_UsbSwitchAFlag     = 0x04000403;
    constexpr inline const DeviceCode DeviceCode_UsbSwitchB3Oc      = 0x04000404;
    constexpr inline const DeviceCode DeviceCode_UsbSwitchB3En      = 0x04000405;
    constexpr inline const DeviceCode DeviceCode_UsbSwitchB2En      = 0x04000406;
    constexpr inline const DeviceCode DeviceCode_Hdmi5VEn           = 0x34000004;
    constexpr inline const DeviceCode DeviceCode_UsbSwitchB1En      = 0x04000407;
    constexpr inline const DeviceCode DeviceCode_HdmiPdTrEn         = 0x34000005;
    constexpr inline const DeviceCode DeviceCode_FanEn              = 0x3D000003;
    constexpr inline const DeviceCode DeviceCode_UsbSwitchB1Oc      = 0x04000408;
    constexpr inline const DeviceCode DeviceCode_PwmFan             = 0x3D000001;
    constexpr inline const DeviceCode DeviceCode_HdmiHpd            = 0x34000006;
    constexpr inline const DeviceCode DeviceCode_Max77812Irq        = 0x3E000003;
    constexpr inline const DeviceCode DeviceCode_Debug0             = 0xCA000001;
    constexpr inline const DeviceCode DeviceCode_Debug1             = 0xCA000002;
    constexpr inline const DeviceCode DeviceCode_Debug2             = 0xCA000003;
    constexpr inline const DeviceCode DeviceCode_Debug3             = 0xCA000004;
    constexpr inline const DeviceCode DeviceCode_NfcIrq             = 0x36000004;
    constexpr inline const DeviceCode DeviceCode_NfcRst             = 0x36000003;
    constexpr inline const DeviceCode DeviceCode_McuIrq             = 0x35000415;
    constexpr inline const DeviceCode DeviceCode_McuBoot            = 0x35000416;
    constexpr inline const DeviceCode DeviceCode_McuRst             = 0x35000417;
    constexpr inline const DeviceCode DeviceCode_Vdd5V3En           = 0x39000403;
    constexpr inline const DeviceCode DeviceCode_McuPor             = 0x35000418;
    constexpr inline const DeviceCode DeviceCode_LcdGpio1           = 0x35000005;
    constexpr inline const DeviceCode DeviceCode_NfcEn              = 0x36000002;
    constexpr inline const DeviceCode DeviceCode_ExtUart2Rts        = 0x35000406;
    constexpr inline const DeviceCode DeviceCode_ExtUart3Rts        = 0x35000410;
    constexpr inline const DeviceCode DeviceCode_GpioPortC7         = 0x3500041B;
    constexpr inline const DeviceCode DeviceCode_GpioPortD0         = 0x3500041C;
    constexpr inline const DeviceCode DeviceCode_GpioPortC5         = 0x3500041D;
    constexpr inline const DeviceCode DeviceCode_GpioPortC6         = 0x3500041E;
    constexpr inline const DeviceCode DeviceCode_GpioPortY7         = 0x35000065;
    constexpr inline const DeviceCode DeviceCode_GpioPortF1         = 0x04000409;
    constexpr inline const DeviceCode DeviceCode_GpioPortH0         = 0x34000401;

    constexpr inline GpioPadName ConvertToGpioPadName(DeviceCode dc) {
        switch (dc.GetInternalValue()) {
            case DeviceCode_CodecLdoEnTemp    .GetInternalValue(): return GpioPadName_CodecLdoEnTemp;
            case DeviceCode_PowSdEn           .GetInternalValue(): return GpioPadName_PowSdEn;
            case DeviceCode_BtRst             .GetInternalValue(): return GpioPadName_BtRst;
            case DeviceCode_RamCode3          .GetInternalValue(): return GpioPadName_RamCode3;
            case DeviceCode_GameCardReset     .GetInternalValue(): return GpioPadName_GameCardReset;
            case DeviceCode_CodecAlert        .GetInternalValue(): return GpioPadName_CodecAlert;
            case DeviceCode_PowGc             .GetInternalValue(): return GpioPadName_PowGc;
            case DeviceCode_DebugControllerDet.GetInternalValue(): return GpioPadName_DebugControllerDet;
            case DeviceCode_BattChgStatus     .GetInternalValue(): return GpioPadName_BattChgStatus;
            case DeviceCode_BattChgEnableN    .GetInternalValue(): return GpioPadName_BattChgEnableN;
            case DeviceCode_FanTach           .GetInternalValue(): return GpioPadName_FanTach;
            case DeviceCode_ExtconDetS        .GetInternalValue(): return GpioPadName_ExtconDetS;
            case DeviceCode_Vdd50AEn          .GetInternalValue(): return GpioPadName_Vdd50AEn;
            case DeviceCode_SdevCoaxSel1      .GetInternalValue(): return GpioPadName_SdevCoaxSel1;
            case DeviceCode_GameCardCd        .GetInternalValue(): return GpioPadName_GameCardCd;
            case DeviceCode_ProdType0         .GetInternalValue(): return GpioPadName_ProdType0;
            case DeviceCode_ProdType1         .GetInternalValue(): return GpioPadName_ProdType1;
            case DeviceCode_ProdType2         .GetInternalValue(): return GpioPadName_ProdType2;
            case DeviceCode_ProdType3         .GetInternalValue(): return GpioPadName_ProdType3;
            case DeviceCode_TempAlert         .GetInternalValue(): return GpioPadName_TempAlert;
            case DeviceCode_CodecHpDetIrq     .GetInternalValue(): return GpioPadName_CodecHpDetIrq;
            case DeviceCode_MotionInt         .GetInternalValue(): return GpioPadName_MotionInt;
            case DeviceCode_TpIrq             .GetInternalValue(): return GpioPadName_TpIrq;
            case DeviceCode_ButtonSleep2      .GetInternalValue(): return GpioPadName_ButtonSleep2;
            case DeviceCode_ButtonVolUp       .GetInternalValue(): return GpioPadName_ButtonVolUp;
            case DeviceCode_ButtonVolDn       .GetInternalValue(): return GpioPadName_ButtonVolDn;
            case DeviceCode_BattMgicIrq       .GetInternalValue(): return GpioPadName_BattMgicIrq;
            case DeviceCode_RecoveryKey       .GetInternalValue(): return GpioPadName_RecoveryKey;
            case DeviceCode_PowLcdBlEn        .GetInternalValue(): return GpioPadName_PowLcdBlEn;
            case DeviceCode_LcdReset          .GetInternalValue(): return GpioPadName_LcdReset;
            case DeviceCode_PdVconnEn         .GetInternalValue(): return GpioPadName_PdVconnEn;
            case DeviceCode_PdRstN            .GetInternalValue(): return GpioPadName_PdRstN;
            case DeviceCode_Bq24190Irq        .GetInternalValue(): return GpioPadName_Bq24190Irq;
            case DeviceCode_SdevCoaxSel0      .GetInternalValue(): return GpioPadName_SdevCoaxSel0;
            case DeviceCode_SdWp              .GetInternalValue(): return GpioPadName_SdWp;
            case DeviceCode_TpReset           .GetInternalValue(): return GpioPadName_TpReset;
            case DeviceCode_BtGpio2           .GetInternalValue(): return GpioPadName_BtGpio2;
            case DeviceCode_BtGpio3           .GetInternalValue(): return GpioPadName_BtGpio3;
            case DeviceCode_BtGpio4           .GetInternalValue(): return GpioPadName_BtGpio4;
            case DeviceCode_CradleIrq         .GetInternalValue(): return GpioPadName_CradleIrq;
         /* case DeviceCode_PowVcpuInt        .GetInternalValue(): return GpioPadName_PowVcpuInt; */
            case DeviceCode_Max77621GpuInt    .GetInternalValue(): return GpioPadName_Max77621GpuInt;
            case DeviceCode_ExtconChgU        .GetInternalValue(): return GpioPadName_ExtconChgU;
            case DeviceCode_ExtconChgS        .GetInternalValue(): return GpioPadName_ExtconChgS;
            case DeviceCode_WifiRfDisable     .GetInternalValue(): return GpioPadName_WifiRfDisable;
            case DeviceCode_WifiReset         .GetInternalValue(): return GpioPadName_WifiReset;
            case DeviceCode_ApWakeBt          .GetInternalValue(): return GpioPadName_ApWakeBt;
            case DeviceCode_BtWakeAp          .GetInternalValue(): return GpioPadName_BtWakeAp;
            case DeviceCode_BtGpio5           .GetInternalValue(): return GpioPadName_BtGpio5;
            case DeviceCode_PowLcdVddPEn      .GetInternalValue(): return GpioPadName_PowLcdVddPEn;
            case DeviceCode_PowLcdVddNEn      .GetInternalValue(): return GpioPadName_PowLcdVddNEn;
            case DeviceCode_ExtconDetU        .GetInternalValue(): return GpioPadName_ExtconDetU;
            case DeviceCode_RamCode2          .GetInternalValue(): return GpioPadName_RamCode2;
            case DeviceCode_Vdd50BEn          .GetInternalValue(): return GpioPadName_Vdd50BEn;
            case DeviceCode_WifiWakeHost      .GetInternalValue(): return GpioPadName_WifiWakeHost;
            case DeviceCode_SdCd              .GetInternalValue(): return GpioPadName_SdCd;
            case DeviceCode_OtgFet1ForSdev    .GetInternalValue(): return GpioPadName_OtgFet1ForSdev;
            case DeviceCode_OtgFet2ForSdev    .GetInternalValue(): return GpioPadName_OtgFet2ForSdev;
            case DeviceCode_ExtConWakeU       .GetInternalValue(): return GpioPadName_ExtConWakeU;
            case DeviceCode_ExtConWakeS       .GetInternalValue(): return GpioPadName_ExtConWakeS;
            case DeviceCode_PmuIrq            .GetInternalValue(): return GpioPadName_PmuIrq;
            case DeviceCode_ExtUart2Cts       .GetInternalValue(): return GpioPadName_ExtUart2Cts;
            case DeviceCode_ExtUart3Cts       .GetInternalValue(): return GpioPadName_ExtUart3Cts;
            case DeviceCode_5VStepDownEn      .GetInternalValue(): return GpioPadName_5VStepDownEn;
            case DeviceCode_UsbSwitchB2Oc     .GetInternalValue(): return GpioPadName_UsbSwitchB2Oc;
            case DeviceCode_5VStepDownPg      .GetInternalValue(): return GpioPadName_5VStepDownPg;
            case DeviceCode_UsbSwitchAEn      .GetInternalValue(): return GpioPadName_UsbSwitchAEn;
            case DeviceCode_UsbSwitchAFlag    .GetInternalValue(): return GpioPadName_UsbSwitchAFlag;
            case DeviceCode_UsbSwitchB3Oc     .GetInternalValue(): return GpioPadName_UsbSwitchB3Oc;
            case DeviceCode_UsbSwitchB3En     .GetInternalValue(): return GpioPadName_UsbSwitchB3En;
            case DeviceCode_UsbSwitchB2En     .GetInternalValue(): return GpioPadName_UsbSwitchB2En;
            case DeviceCode_Hdmi5VEn          .GetInternalValue(): return GpioPadName_Hdmi5VEn;
            case DeviceCode_UsbSwitchB1En     .GetInternalValue(): return GpioPadName_UsbSwitchB1En;
            case DeviceCode_HdmiPdTrEn        .GetInternalValue(): return GpioPadName_HdmiPdTrEn;
            case DeviceCode_FanEn             .GetInternalValue(): return GpioPadName_FanEn;
            case DeviceCode_UsbSwitchB1Oc     .GetInternalValue(): return GpioPadName_UsbSwitchB1Oc;
            case DeviceCode_PwmFan            .GetInternalValue(): return GpioPadName_PwmFan;
            case DeviceCode_HdmiHpd           .GetInternalValue(): return GpioPadName_HdmiHpd;
            case DeviceCode_Max77812Irq       .GetInternalValue(): return GpioPadName_Max77812Irq;
            case DeviceCode_Debug0            .GetInternalValue(): return GpioPadName_Debug0;
            case DeviceCode_Debug1            .GetInternalValue(): return GpioPadName_Debug1;
            case DeviceCode_Debug2            .GetInternalValue(): return GpioPadName_Debug2;
            case DeviceCode_Debug3            .GetInternalValue(): return GpioPadName_Debug3;
            case DeviceCode_NfcIrq            .GetInternalValue(): return GpioPadName_NfcIrq;
            case DeviceCode_NfcRst            .GetInternalValue(): return GpioPadName_NfcRst;
            case DeviceCode_McuIrq            .GetInternalValue(): return GpioPadName_McuIrq;
            case DeviceCode_McuBoot           .GetInternalValue(): return GpioPadName_McuBoot;
            case DeviceCode_McuRst            .GetInternalValue(): return GpioPadName_McuRst;
            case DeviceCode_Vdd5V3En          .GetInternalValue(): return GpioPadName_Vdd5V3En;
            case DeviceCode_McuPor            .GetInternalValue(): return GpioPadName_McuPor;
            case DeviceCode_LcdGpio1          .GetInternalValue(): return GpioPadName_LcdGpio1;
            case DeviceCode_NfcEn             .GetInternalValue(): return GpioPadName_NfcEn;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    constexpr inline DeviceCode ConvertToDeviceCode(GpioPadName gpn) {
        switch (gpn) {
            case GpioPadName_CodecLdoEnTemp:     return DeviceCode_CodecLdoEnTemp;
            case GpioPadName_PowSdEn:            return DeviceCode_PowSdEn;
            case GpioPadName_BtRst:              return DeviceCode_BtRst;
            case GpioPadName_RamCode3:           return DeviceCode_RamCode3;
            case GpioPadName_GameCardReset:      return DeviceCode_GameCardReset;
            case GpioPadName_CodecAlert:         return DeviceCode_CodecAlert;
            case GpioPadName_PowGc:              return DeviceCode_PowGc;
            case GpioPadName_DebugControllerDet: return DeviceCode_DebugControllerDet;
            case GpioPadName_BattChgStatus:      return DeviceCode_BattChgStatus;
            case GpioPadName_BattChgEnableN:     return DeviceCode_BattChgEnableN;
            case GpioPadName_FanTach:            return DeviceCode_FanTach;
            case GpioPadName_ExtconDetS:         return DeviceCode_ExtconDetS;
            case GpioPadName_Vdd50AEn:           return DeviceCode_Vdd50AEn;
            case GpioPadName_SdevCoaxSel1:       return DeviceCode_SdevCoaxSel1;
            case GpioPadName_GameCardCd:         return DeviceCode_GameCardCd;
            case GpioPadName_ProdType0:          return DeviceCode_ProdType0;
            case GpioPadName_ProdType1:          return DeviceCode_ProdType1;
            case GpioPadName_ProdType2:          return DeviceCode_ProdType2;
            case GpioPadName_ProdType3:          return DeviceCode_ProdType3;
            case GpioPadName_TempAlert:          return DeviceCode_TempAlert;
            case GpioPadName_CodecHpDetIrq:      return DeviceCode_CodecHpDetIrq;
            case GpioPadName_MotionInt:          return DeviceCode_MotionInt;
            case GpioPadName_TpIrq:              return DeviceCode_TpIrq;
            case GpioPadName_ButtonSleep2:       return DeviceCode_ButtonSleep2;
            case GpioPadName_ButtonVolUp:        return DeviceCode_ButtonVolUp;
            case GpioPadName_ButtonVolDn:        return DeviceCode_ButtonVolDn;
            case GpioPadName_BattMgicIrq:        return DeviceCode_BattMgicIrq;
            case GpioPadName_RecoveryKey:        return DeviceCode_RecoveryKey;
            case GpioPadName_PowLcdBlEn:         return DeviceCode_PowLcdBlEn;
            case GpioPadName_LcdReset:           return DeviceCode_LcdReset;
            case GpioPadName_PdVconnEn:          return DeviceCode_PdVconnEn;
            case GpioPadName_PdRstN:             return DeviceCode_PdRstN;
            case GpioPadName_Bq24190Irq:         return DeviceCode_Bq24190Irq;
            case GpioPadName_SdevCoaxSel0:       return DeviceCode_SdevCoaxSel0;
            case GpioPadName_SdWp:               return DeviceCode_SdWp;
            case GpioPadName_TpReset:            return DeviceCode_TpReset;
            case GpioPadName_BtGpio2:            return DeviceCode_BtGpio2;
            case GpioPadName_BtGpio3:            return DeviceCode_BtGpio3;
            case GpioPadName_BtGpio4:            return DeviceCode_BtGpio4;
            case GpioPadName_CradleIrq:          return DeviceCode_CradleIrq;
            case GpioPadName_PowVcpuInt:         return DeviceCode_PowVcpuInt;
            case GpioPadName_Max77621GpuInt:     return DeviceCode_Max77621GpuInt;
            case GpioPadName_ExtconChgU:         return DeviceCode_ExtconChgU;
            case GpioPadName_ExtconChgS:         return DeviceCode_ExtconChgS;
            case GpioPadName_WifiRfDisable:      return DeviceCode_WifiRfDisable;
            case GpioPadName_WifiReset:          return DeviceCode_WifiReset;
            case GpioPadName_ApWakeBt:           return DeviceCode_ApWakeBt;
            case GpioPadName_BtWakeAp:           return DeviceCode_BtWakeAp;
            case GpioPadName_BtGpio5:            return DeviceCode_BtGpio5;
            case GpioPadName_PowLcdVddPEn:       return DeviceCode_PowLcdVddPEn;
            case GpioPadName_PowLcdVddNEn:       return DeviceCode_PowLcdVddNEn;
            case GpioPadName_ExtconDetU:         return DeviceCode_ExtconDetU;
            case GpioPadName_RamCode2:           return DeviceCode_RamCode2;
            case GpioPadName_Vdd50BEn:           return DeviceCode_Vdd50BEn;
            case GpioPadName_WifiWakeHost:       return DeviceCode_WifiWakeHost;
            case GpioPadName_SdCd:               return DeviceCode_SdCd;
            case GpioPadName_OtgFet1ForSdev:     return DeviceCode_OtgFet1ForSdev;
            case GpioPadName_OtgFet2ForSdev:     return DeviceCode_OtgFet2ForSdev;
            case GpioPadName_ExtConWakeU:        return DeviceCode_ExtConWakeU;
            case GpioPadName_ExtConWakeS:        return DeviceCode_ExtConWakeS;
            case GpioPadName_PmuIrq:             return DeviceCode_PmuIrq;
            case GpioPadName_ExtUart2Cts:        return DeviceCode_ExtUart2Cts;
            case GpioPadName_ExtUart3Cts:        return DeviceCode_ExtUart3Cts;
            case GpioPadName_5VStepDownEn:       return DeviceCode_5VStepDownEn;
            case GpioPadName_UsbSwitchB2Oc:      return DeviceCode_UsbSwitchB2Oc;
            case GpioPadName_5VStepDownPg:       return DeviceCode_5VStepDownPg;
            case GpioPadName_UsbSwitchAEn:       return DeviceCode_UsbSwitchAEn;
            case GpioPadName_UsbSwitchAFlag:     return DeviceCode_UsbSwitchAFlag;
            case GpioPadName_UsbSwitchB3Oc:      return DeviceCode_UsbSwitchB3Oc;
            case GpioPadName_UsbSwitchB3En:      return DeviceCode_UsbSwitchB3En;
            case GpioPadName_UsbSwitchB2En:      return DeviceCode_UsbSwitchB2En;
            case GpioPadName_Hdmi5VEn:           return DeviceCode_Hdmi5VEn;
            case GpioPadName_UsbSwitchB1En:      return DeviceCode_UsbSwitchB1En;
            case GpioPadName_HdmiPdTrEn:         return DeviceCode_HdmiPdTrEn;
            case GpioPadName_FanEn:              return DeviceCode_FanEn;
            case GpioPadName_UsbSwitchB1Oc:      return DeviceCode_UsbSwitchB1Oc;
            case GpioPadName_PwmFan:             return DeviceCode_PwmFan;
            case GpioPadName_HdmiHpd:            return DeviceCode_HdmiHpd;
            case GpioPadName_Max77812Irq:        return DeviceCode_Max77812Irq;
            case GpioPadName_Debug0:             return DeviceCode_Debug0;
            case GpioPadName_Debug1:             return DeviceCode_Debug1;
            case GpioPadName_Debug2:             return DeviceCode_Debug2;
            case GpioPadName_Debug3:             return DeviceCode_Debug3;
            case GpioPadName_NfcIrq:             return DeviceCode_NfcIrq;
            case GpioPadName_NfcRst:             return DeviceCode_NfcRst;
            case GpioPadName_McuIrq:             return DeviceCode_McuIrq;
            case GpioPadName_McuBoot:            return DeviceCode_McuBoot;
            case GpioPadName_McuRst:             return DeviceCode_McuRst;
            case GpioPadName_Vdd5V3En:           return DeviceCode_Vdd5V3En;
            case GpioPadName_McuPor:             return DeviceCode_McuPor;
            case GpioPadName_LcdGpio1:           return DeviceCode_LcdGpio1;
            case GpioPadName_NfcEn:              return DeviceCode_NfcEn;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

}
