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

static constexpr u8 Bq24193InputSourceControl                     = 0x00;
static constexpr u8 Bq24193PowerOnConfiguration                   = 0x01;
static constexpr u8 Bq24193ChargeCurrentControl                   = 0x02;
static constexpr u8 Bq24193PreChargeTerminationCurrentControl     = 0x03;
static constexpr u8 Bq24193ChargeVoltageControl                   = 0x04;
static constexpr u8 Bq24193ChargeTerminationTimerControl          = 0x05;
static constexpr u8 Bq24193IrCompensationThermalRegulationControl = 0x06;
static constexpr u8 Bq24193MiscOperationControl                   = 0x07;
static constexpr u8 Bq24193SystemStatus                           = 0x08;
static constexpr u8 Bq24193Fault                                  = 0x09;
static constexpr u8 Bq24193VendorPartRevisionStatus               = 0x0A;

enum ChargerConfiguration : u8 {
    ChargerConfiguration_ChargeDisable = (0 << 4),
    ChargerConfiguration_ChargeBattery = (1 << 4),
    ChargerConfiguration_Otg           = (2 << 4),
};

static constexpr u32 ChargeVoltageLimitMin = 3504;
static constexpr u32 ChargeVoltageLimitMax = 4208;

static inline u8 EncodeChargeVoltageLimit(u32 voltage) {
    if (voltage < ChargeVoltageLimitMin || voltage > ChargeVoltageLimitMax) {
        std::abort();
    }
    voltage -= ChargeVoltageLimitMin;
    voltage >>= 4;
    return static_cast<u8>(voltage << 2);
}

static inline u32 DecodeChargeVoltageLimit(u8 reg) {
    return ChargeVoltageLimitMin + (static_cast<u32>(reg & 0xFC) << 2);
}

static constexpr u32 FastChargeCurrentLimitMin = 512;
static constexpr u32 FastChargeCurrentLimitMax = 4544;

static inline u8 EncodeFastChargeCurrentLimit(u32 current) {
    if (current < FastChargeCurrentLimitMin || current > FastChargeCurrentLimitMax) {
        std::abort();
    }
    current -= FastChargeCurrentLimitMin;
    current >>= 6;
    return static_cast<u8>(current << 2);
}

static inline u32 DecodeFastChargeCurrentLimit(u8 reg) {
    return FastChargeCurrentLimitMin + (static_cast<u32>(reg & 0xFC) << 4);
}

enum InputCurrentLimit : u8 {
    InputCurrentLimit_100mA  = 0,
    InputCurrentLimit_150mA  = 1,
    InputCurrentLimit_500mA  = 2,
    InputCurrentLimit_900mA  = 3,
    InputCurrentLimit_1200mA = 4,
    InputCurrentLimit_1500mA = 5,
    InputCurrentLimit_2000mA = 6,
    InputCurrentLimit_3000mA = 7,
};

static constexpr u32 PreChargeCurrentLimitMin = 128;
static constexpr u32 PreChargeCurrentLimitMax = 2048;

static inline u8 EncodePreChargeCurrentLimit(u32 current) {
    if (current < PreChargeCurrentLimitMin || current > PreChargeCurrentLimitMax) {
        std::abort();
    }
    current -= PreChargeCurrentLimitMin;
    current >>= 7;
    return static_cast<u8>(current << 4);
}

static inline u32 DecodePreChargeCurrentLimit(u8 reg) {
    return PreChargeCurrentLimitMin + (static_cast<u32>(reg & 0xF0) << 3);
}

static constexpr u32 TerminationCurrentLimitMin = 128;
static constexpr u32 TerminationCurrentLimitMax = 2048;

static inline u8 EncodeTerminationCurrentLimit(u32 current) {
    if (current < TerminationCurrentLimitMin || current > TerminationCurrentLimitMax) {
        std::abort();
    }
    current -= TerminationCurrentLimitMin;
    current >>= 7;
    return static_cast<u8>(current);
}

static inline u32 DecodeTerminationCurrentLimit(u8 reg) {
    return TerminationCurrentLimitMin + (static_cast<u32>(reg & 0xF) << 7);
}

static constexpr u32 MinimumSystemVoltageLimitMin = 3000;
static constexpr u32 MinimumSystemVoltageLimitMax = 3700;

static inline u8 EncodeMinimumSystemVoltageLimit(u32 voltage) {
    if (voltage < MinimumSystemVoltageLimitMin || voltage > MinimumSystemVoltageLimitMax) {
        std::abort();
    }
    voltage -= MinimumSystemVoltageLimitMin;
    voltage /= 100;
    return static_cast<u8>(voltage << 1);
}

static inline u32 DecodeMinimumSystemVoltageLimit(u8 reg) {
    return MinimumSystemVoltageLimitMin + (static_cast<u32>(reg & 0x0E) * 50);
}

enum WatchdogTimerSetting : u8 {
    WatchdogTimerSetting_Disabled = (0 << 4),
    WatchdogTimerSetting_40s      = (1 << 4),
    WatchdogTimerSetting_80s      = (2 << 4),
    WatchdogTimerSetting_160s     = (3 << 4),
};

enum BoostModeCurrentLimit : u8 {
    BoostModeCurrentLimit_500mA  = 0,
    BoostModeCurrentLimit_1300mA = 1,
};