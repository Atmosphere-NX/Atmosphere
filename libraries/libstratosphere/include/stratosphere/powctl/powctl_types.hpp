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

namespace ams::powctl {

    /* Charger types. */
    enum ChargerStatus {
        ChargerStatus_Charging              = 1,

        ChargerStatus_NotCharging           = 3,
        ChargerStatus_ChargeTerminationDone = 4,
    };

    enum ChargerConfiguration {
        ChargerConfiguration_ChargeDisable = 1,
        ChargerConfiguration_ChargeBattery = 2,
        ChargerConfiguration_Otg           = 3,
    };

    enum ChargeCurrentState {
        ChargeCurrentState_Unknown                = 0x0,
        ChargeCurrentState_NotCharging            = 0x1,
        ChargeCurrentState_ChargingForce20Percent = 0x2,
        ChargeCurrentState_Charging               = 0x3,
    };

    enum class BatteryTemperatureLevel {
        TooLow  = 0,
        Low     = 1,
        Medium  = 2,
        High    = 3,
        TooHigh = 4,
    };

    enum class PowerState {
        FullAwake          = 0,
        MinimumAwake       = 1,
        SleepCharge        = 2,
        SleepDischarge     = 3,
        ShutdownChargeMain = 4,
    };

}