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
#include <stratosphere/i2c/i2c_types.hpp>

namespace ams::i2c {

    enum I2cBus {
        I2cBus_I2c1 = 0,
        I2cBus_I2c2 = 1,
        I2cBus_I2c3 = 2,
        I2cBus_I2c4 = 3,
        I2cBus_I2c5 = 4,
        I2cBus_I2c6 = 5,
    };

    constexpr inline const DeviceCode DeviceCode_I2c1 = 0x02000001;
    constexpr inline const DeviceCode DeviceCode_I2c2 = 0x02000002;
    constexpr inline const DeviceCode DeviceCode_I2c3 = 0x02000003;
    constexpr inline const DeviceCode DeviceCode_I2c4 = 0x02000004;
    constexpr inline const DeviceCode DeviceCode_I2c5 = 0x02000005;
    constexpr inline const DeviceCode DeviceCode_I2c6 = 0x02000006;

    constexpr inline DeviceCode ConvertToDeviceCode(I2cBus bus) {
        switch (bus) {
            case I2cBus_I2c1: return DeviceCode_I2c1;
            case I2cBus_I2c2: return DeviceCode_I2c2;
            case I2cBus_I2c3: return DeviceCode_I2c3;
            case I2cBus_I2c4: return DeviceCode_I2c4;
            case I2cBus_I2c5: return DeviceCode_I2c5;
            case I2cBus_I2c6: return DeviceCode_I2c6;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    constexpr inline DeviceCode ConvertToI2cBus(DeviceCode dc) {
        switch (dc.GetInternalValue()) {
            case DeviceCode_I2c1.GetInternalValue(): return I2cBus_I2c1;
            case DeviceCode_I2c2.GetInternalValue(): return I2cBus_I2c2;
            case DeviceCode_I2c3.GetInternalValue(): return I2cBus_I2c3;
            case DeviceCode_I2c4.GetInternalValue(): return I2cBus_I2c4;
            case DeviceCode_I2c5.GetInternalValue(): return I2cBus_I2c5;
            case DeviceCode_I2c6.GetInternalValue(): return I2cBus_I2c6;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    enum I2cDevice : u32 {
        I2cDevice_ClassicController   =  0,
        I2cDevice_Ftm3bd56            =  1,
        I2cDevice_Tmp451              =  2,
        I2cDevice_Nct72               =  3,
        I2cDevice_Alc5639             =  4,
        I2cDevice_Max77620Rtc         =  5,
        I2cDevice_Max77620Pmic        =  6,
        I2cDevice_Max77621Cpu         =  7,
        I2cDevice_Max77621Gpu         =  8,
        I2cDevice_Bq24193             =  9,
        I2cDevice_Max17050            = 10,
        I2cDevice_Bm92t30mwv          = 11,
        I2cDevice_Ina226Vdd15v0Hb     = 12,

        I2cDevice_Ina226VsysCpuDs     = 13,
        I2cDevice_Ina226VddCpuAp      = 13,

        I2cDevice_Ina226VsysGpuDs     = 14,
        I2cDevice_Ina226VddGpuAp      = 14,

        I2cDevice_Ina226VsysDdrDs     = 15,
        I2cDevice_Ina226VddDdr1V1Pmic = 15,

        I2cDevice_Ina226VsysAp        = 16,
        I2cDevice_Ina226VsysBlDs      = 17,
        I2cDevice_Bh1730              = 18,

        I2cDevice_Ina226VsysCore      = 19,
        I2cDevice_Ina226VddCoreAp     = 19,

        I2cDevice_Ina226Soc1V8        = 20,
        I2cDevice_Ina226VddSoc1V8     = 20,

        I2cDevice_Ina226Lpddr1V8      = 21,
        I2cDevice_Ina226Vdd1V8        = 21,

        I2cDevice_Ina226Reg1V32       = 22,
        I2cDevice_Ina226Vdd3V3Sys     = 23,
        I2cDevice_HdmiDdc             = 24,
        I2cDevice_HdmiScdc            = 25,
        I2cDevice_HdmiHdcp            = 26,
        I2cDevice_Fan53528            = 27,
        I2cDevice_Max77812_3          = 28,
        I2cDevice_Max77812_2          = 29,
        I2cDevice_Ina226VddDdr0V6     = 30,
        I2cDevice_HoagNfcIc           = 31, /* TODO */
    };

    /* TODO: Better place for this? */
    constexpr inline const DeviceCode DeviceCode_ClassicController  = 0x350000C9;
    constexpr inline const DeviceCode DeviceCode_Ftm3bd56           = 0x35000033;
    constexpr inline const DeviceCode DeviceCode_Tmp451             = 0x3E000001;
    constexpr inline const DeviceCode DeviceCode_Nct72              = 0x3E000001;
    constexpr inline const DeviceCode DeviceCode_Alc5639            = 0x33000001;
    constexpr inline const DeviceCode DeviceCode_Max77620Rtc        = 0x3B000001;
    constexpr inline const DeviceCode DeviceCode_Max77620Pmic       = 0x3A000001;
    constexpr inline const DeviceCode DeviceCode_Max77621Cpu        = 0x3A000003;
    constexpr inline const DeviceCode DeviceCode_Max77621Gpu        = 0x3A000004;
    constexpr inline const DeviceCode DeviceCode_Bq24193            = 0x39000001;
    constexpr inline const DeviceCode DeviceCode_Max17050           = 0x39000033;
    constexpr inline const DeviceCode DeviceCode_Bm92t30mwv         = 0x040000C9;
    constexpr inline const DeviceCode DeviceCode_Ina226Vdd15v0Hb    = 0x3F000401;

    constexpr inline const DeviceCode DeviceCode_Ina226VsysCpuDs    = 0x3F000001;
    constexpr inline const DeviceCode DeviceCode_Ina226VddCpuAp     = 0x3F000001;

    constexpr inline const DeviceCode DeviceCode_Ina226VsysGpuDs    = 0x3F000002;
    constexpr inline const DeviceCode DeviceCode_Ina226VddGpuAp     = 0x3F000002;

    constexpr inline const DeviceCode DeviceCode_Ina226VsysDdrDs    = 0x3F000003;
    constexpr inline const DeviceCode DeviceCode_Ina226VddDdr1V1Pmi = 0x3F000003;

    constexpr inline const DeviceCode DeviceCode_Ina226VsysAp       = 0x3F000402;
    constexpr inline const DeviceCode DeviceCode_Ina226VsysBlDs     = 0x3F000403;
    constexpr inline const DeviceCode DeviceCode_Bh1730             = 0x35000047;

    constexpr inline const DeviceCode DeviceCode_Ina226VsysCore     = 0x3F000404;
    constexpr inline const DeviceCode DeviceCode_Ina226VddCoreAp    = 0x3F000404;

    constexpr inline const DeviceCode DeviceCode_Ina226Soc1V8       = 0x3F000405;
    constexpr inline const DeviceCode DeviceCode_Ina226VddSoc1V8    = 0x3F000405;

    constexpr inline const DeviceCode DeviceCode_Ina226Lpddr1V8     = 0x3F000406;
    constexpr inline const DeviceCode DeviceCode_Ina226Vdd1V8       = 0x3F000406;

    constexpr inline const DeviceCode DeviceCode_Ina226Reg1V32      = 0x3F000407;
    constexpr inline const DeviceCode DeviceCode_Ina226Vdd3V3Sys    = 0x3F000408;
    constexpr inline const DeviceCode DeviceCode_HdmiDdc            = 0x34000001;
    constexpr inline const DeviceCode DeviceCode_HdmiScdc           = 0x34000002;
    constexpr inline const DeviceCode DeviceCode_HdmiHdcp           = 0x34000003;
    constexpr inline const DeviceCode DeviceCode_Fan53528           = 0x3A000005;
    constexpr inline const DeviceCode DeviceCode_Max77812_3         = 0x3A000002;
    constexpr inline const DeviceCode DeviceCode_Max77812_2         = 0x3A000006;
    constexpr inline const DeviceCode DeviceCode_Ina226VddDdr0V6    = 0x3F000409;
    constexpr inline const DeviceCode DeviceCode_HoagNfcIc          = 0x36000001;

    constexpr inline DeviceCode ConvertToDeviceCode(I2cDevice dv) {
        switch (dv) {
            case I2cDevice_ClassicController: return DeviceCode_ClassicController;
            case I2cDevice_Ftm3bd56:          return DeviceCode_Ftm3bd56;
            case I2cDevice_Tmp451:            return DeviceCode_Tmp451;
            case I2cDevice_Nct72:             return DeviceCode_Nct72;
            case I2cDevice_Alc5639:           return DeviceCode_Alc5639;
            case I2cDevice_Max77620Rtc:       return DeviceCode_Max77620Rtc;
            case I2cDevice_Max77620Pmic:      return DeviceCode_Max77620Pmic;
            case I2cDevice_Max77621Cpu:       return DeviceCode_Max77621Cpu;
            case I2cDevice_Max77621Gpu:       return DeviceCode_Max77621Gpu;
            case I2cDevice_Bq24193:           return DeviceCode_Bq24193;
            case I2cDevice_Max17050:          return DeviceCode_Max17050;
            case I2cDevice_Bm92t30mwv:        return DeviceCode_Bm92t30mwv;
            case I2cDevice_Ina226Vdd15v0Hb:   return DeviceCode_Ina226Vdd15v0Hb;
            case I2cDevice_Ina226VsysCpuDs:   return DeviceCode_Ina226VsysCpuDs;
            case I2cDevice_Ina226VsysGpuDs:   return DeviceCode_Ina226VsysGpuDs;
            case I2cDevice_Ina226VsysDdrDs:   return DeviceCode_Ina226VsysDdrDs;
            case I2cDevice_Ina226VsysAp:      return DeviceCode_Ina226VsysAp;
            case I2cDevice_Ina226VsysBlDs:    return DeviceCode_Ina226VsysBlDs;
            case I2cDevice_Bh1730:            return DeviceCode_Bh1730;
            case I2cDevice_Ina226VsysCore:    return DeviceCode_Ina226VsysCore;
            case I2cDevice_Ina226Soc1V8:      return DeviceCode_Ina226Soc1V8;
            case I2cDevice_Ina226Lpddr1V8:    return DeviceCode_Ina226Lpddr1V8;
            case I2cDevice_Ina226Reg1V32:     return DeviceCode_Ina226Reg1V32;
            case I2cDevice_Ina226Vdd3V3Sys:   return DeviceCode_Ina226Vdd3V3Sys;
            case I2cDevice_HdmiDdc:           return DeviceCode_HdmiDdc;
            case I2cDevice_HdmiScdc:          return DeviceCode_HdmiScdc;
            case I2cDevice_HdmiHdcp:          return DeviceCode_HdmiHdcp;
            case I2cDevice_Fan53528:          return DeviceCode_Fan53528;
            case I2cDevice_Max77812_3:        return DeviceCode_Max77812_3;
            case I2cDevice_Max77812_2:        return DeviceCode_Max77812_2;
            case I2cDevice_Ina226VddDdr0V6:   return DeviceCode_Ina226VddDdr0V6;
            case I2cDevice_HoagNfcIc:         return DeviceCode_HoagNfcIc;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    constexpr inline I2cDevice ConvertToI2cDevice(DeviceCode dc) {
        switch (dc.GetInternalValue()) {
            case DeviceCode_ClassicController.GetInternalValue(): return I2cDevice_ClassicController;
            case DeviceCode_Ftm3bd56         .GetInternalValue(): return I2cDevice_Ftm3bd56;
            case DeviceCode_Tmp451           .GetInternalValue(): return I2cDevice_Tmp451;
         /* case DeviceCode_Nct72            .GetInternalValue(): return I2cDevice_Nct72; */
            case DeviceCode_Alc5639          .GetInternalValue(): return I2cDevice_Alc5639;
            case DeviceCode_Max77620Rtc      .GetInternalValue(): return I2cDevice_Max77620Rtc;
            case DeviceCode_Max77620Pmic     .GetInternalValue(): return I2cDevice_Max77620Pmic;
            case DeviceCode_Max77621Cpu      .GetInternalValue(): return I2cDevice_Max77621Cpu;
            case DeviceCode_Max77621Gpu      .GetInternalValue(): return I2cDevice_Max77621Gpu;
            case DeviceCode_Bq24193          .GetInternalValue(): return I2cDevice_Bq24193;
            case DeviceCode_Max17050         .GetInternalValue(): return I2cDevice_Max17050;
            case DeviceCode_Bm92t30mwv       .GetInternalValue(): return I2cDevice_Bm92t30mwv;
            case DeviceCode_Ina226Vdd15v0Hb  .GetInternalValue(): return I2cDevice_Ina226Vdd15v0Hb;
            case DeviceCode_Ina226VsysCpuDs  .GetInternalValue(): return I2cDevice_Ina226VsysCpuDs;
            case DeviceCode_Ina226VsysGpuDs  .GetInternalValue(): return I2cDevice_Ina226VsysGpuDs;
            case DeviceCode_Ina226VsysDdrDs  .GetInternalValue(): return I2cDevice_Ina226VsysDdrDs;
            case DeviceCode_Ina226VsysAp     .GetInternalValue(): return I2cDevice_Ina226VsysAp;
            case DeviceCode_Ina226VsysBlDs   .GetInternalValue(): return I2cDevice_Ina226VsysBlDs;
            case DeviceCode_Bh1730           .GetInternalValue(): return I2cDevice_Bh1730;
            case DeviceCode_Ina226VsysCore   .GetInternalValue(): return I2cDevice_Ina226VsysCore;
            case DeviceCode_Ina226Soc1V8     .GetInternalValue(): return I2cDevice_Ina226Soc1V8;
            case DeviceCode_Ina226Lpddr1V8   .GetInternalValue(): return I2cDevice_Ina226Lpddr1V8;
            case DeviceCode_Ina226Reg1V32    .GetInternalValue(): return I2cDevice_Ina226Reg1V32;
            case DeviceCode_Ina226Vdd3V3Sys  .GetInternalValue(): return I2cDevice_Ina226Vdd3V3Sys;
            case DeviceCode_HdmiDdc          .GetInternalValue(): return I2cDevice_HdmiDdc;
            case DeviceCode_HdmiScdc         .GetInternalValue(): return I2cDevice_HdmiScdc;
            case DeviceCode_HdmiHdcp         .GetInternalValue(): return I2cDevice_HdmiHdcp;
            case DeviceCode_Fan53528         .GetInternalValue(): return I2cDevice_Fan53528;
            case DeviceCode_Max77812_3       .GetInternalValue(): return I2cDevice_Max77812_3;
            case DeviceCode_Max77812_2       .GetInternalValue(): return I2cDevice_Max77812_2;
            case DeviceCode_Ina226VddDdr0V6  .GetInternalValue(): return I2cDevice_Ina226VddDdr0V6;
            case DeviceCode_HoagNfcIc        .GetInternalValue(): return I2cDevice_HoagNfcIc;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    constexpr bool IsPowerBusDeviceCode(DeviceCode device_code) {
        switch (device_code.GetInternalValue()) {
            case DeviceCode_Max77620Pmic.GetInternalValue():
            case DeviceCode_Max77812_3  .GetInternalValue():
            case DeviceCode_Max77621Cpu .GetInternalValue():
            case DeviceCode_Max77621Gpu .GetInternalValue():
            case DeviceCode_Fan53528    .GetInternalValue():
            case DeviceCode_Max77812_2  .GetInternalValue():
            case DeviceCode_Max77620Rtc .GetInternalValue():
                return true;
            default:
                return false;
        }
    }

}
