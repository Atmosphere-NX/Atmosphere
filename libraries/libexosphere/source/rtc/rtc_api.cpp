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
#include <exosphere.hpp>
#include "max77620-rtc.h"

namespace ams::rtc {

    namespace {

        constexpr inline int I2cAddressMax77620Rtc   = 0x68;

        /* TODO: Find datasheet, link to it instead. */
        /* NOTE: Tentatively, Max77620 "mostly" matches https://datasheets.maximintegrated.com/en/ds/MAX77863.pdf. */
        constexpr inline int Max77620RtcRegisterUpdate0 = 0x04;

        constexpr inline int Max77620RtcRegisterAlarmStart = 0x0E;

        constexpr inline int Max77620RtcRegisterAlarm1Sec     = 0x0E;
        constexpr inline int Max77620RtcRegisterAlarm1Min     = 0x0F;
        constexpr inline int Max77620RtcRegisterAlarm1Hour    = 0x10;
        constexpr inline int Max77620RtcRegisterAlarm1Weekday = 0x11;
        constexpr inline int Max77620RtcRegisterAlarm1Month   = 0x12;
        constexpr inline int Max77620RtcRegisterAlarm1Year    = 0x13;
        constexpr inline int Max77620RtcRegisterAlarm1Date    = 0x14;
        constexpr inline int Max77620RtcRegisterAlarm2Sec     = 0x15;
        constexpr inline int Max77620RtcRegisterAlarm2Min     = 0x16;
        constexpr inline int Max77620RtcRegisterAlarm2Hour    = 0x17;
        constexpr inline int Max77620RtcRegisterAlarm2Weekday = 0x18;
        constexpr inline int Max77620RtcRegisterAlarm2Month   = 0x19;
        constexpr inline int Max77620RtcRegisterAlarm2Year    = 0x1A;
        constexpr inline int Max77620RtcRegisterAlarm2Date    = 0x1B;

        constexpr inline int Max77620RtcRegisterAlarmLast  = 0x1B;

    }

    void StopAlarm() {
        /* Begin update. */
        i2c::SendByte(i2c::Port_5, I2cAddressMax77620Rtc, Max77620RtcRegisterUpdate0, MAX77620_RTC_READ_UPDATE);

        /* Clear ALARM_EN for all alarm registers. */
        for (auto reg = Max77620RtcRegisterAlarmStart; reg <= Max77620RtcRegisterAlarmLast; ++reg) {
            u8 val = i2c::QueryByte(i2c::Port_5, I2cAddressMax77620Rtc, reg);
            val &= ~MAX77620_RTC_ALARM_EN_MASK;
            i2c::SendByte(i2c::Port_5, I2cAddressMax77620Rtc, reg, val);
        }

        /* End update. */
        i2c::SendByte(i2c::Port_5, I2cAddressMax77620Rtc, Max77620RtcRegisterUpdate0, MAX77620_RTC_WRITE_UPDATE);
    }

}
