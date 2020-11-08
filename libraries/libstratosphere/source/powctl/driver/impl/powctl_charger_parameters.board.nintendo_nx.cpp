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
#include <stratosphere.hpp>

namespace ams::powctl::driver::impl {

    namespace {

        constexpr inline const PowerState AcceptablePowerStates[] = {
            PowerState::FullAwake,
            PowerState::MinimumAwake,
            PowerState::SleepCharge,
            PowerState::SleepDischarge,
            PowerState::ShutdownChargeMain,
        };

        constexpr inline const PowerState AcceptablePowerStatesForNotAwakeCharge[] = {
            PowerState::SleepCharge,
            PowerState::ShutdownChargeMain,
        };

        constexpr inline const int Min = std::numeric_limits<int>::min();
        constexpr inline const int Max = std::numeric_limits<int>::max();

        constexpr inline const UnknownParameterX UnknownXTableForBatteryVersion2[] = {
            {  20000, 4320, 95.0, 100.4 },
            {  30000, 4304, 94.0,  99.7 },
            {  40000, 4288, 93.0,  98.4 },
            {  50000, 4272, 92.0,  97.0 },
            {  60000, 4256, 90.0,  95.7 },
            {  80000, 4240, 89.0,  94.2 },
            { 100000, 4224, 88.0,  93.0 },
            {    Max, 4192, 85.0,  90.0 },
        };

        /* Include automatically extracted charger parameters. */
        #include "powctl_charger_parameters.board.nintendo_nx.inc"

    }

    const ChargeParameters &GetChargeParameters() {
        /* Get the battery version. */
        u8 battery_version;
        if (R_FAILED(cal::GetBatteryVersion(std::addressof(battery_version)))) {
            battery_version = 0;
        }

        if (battery_version == 2) {
            return ChargeParametersForBatteryVersion2;
        } else if (battery_version == 1) {
            return ChargeParametersForBatteryVersion1;
        } else {
            if (spl::GetHardwareType() == spl::HardwareType::_Five_) {
                return ChargeParametersForBatteryVersion0ForFive;
            } else {
                return ChargeParametersForBatteryVersion0;
            }
        }
    }

}