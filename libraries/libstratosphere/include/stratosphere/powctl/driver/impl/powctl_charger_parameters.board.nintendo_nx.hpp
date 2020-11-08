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
#include <stratosphere/powctl/powctl_types.hpp>

namespace ams::powctl::driver::impl {

    struct ChargeParametersRule {
        BatteryTemperatureLevel temperature_level;
        int min_avg_v_cell;
        int max_avg_v_cell;
        int min_open_circuit_voltage;
        int max_open_circuit_voltage;
        int min_battery_done_current;
        int max_battery_done_current;
        const PowerState *acceptable_power_states;
        size_t num_acceptable_power_states;
        bool check_battery_current;
        bool reinitialize_charger;
        int charge_voltage_limit;
        int fast_charge_current_limit;
        int battery_compensation;
        int voltage_clamp;
    };

    struct UnknownParameterX {
        int _00;
        int _04;
        double _08;
        double _10;
    };

    struct ChargeParameters {
        int temp_min;
        int temp_low;
        int temp_high;
        int temp_max;
        int low_voltage_fast_charge_current_limit;
        int default_charge_voltage_limit;
        const UnknownParameterX *unknown_x_table;
        size_t x_table_size;
        double _28;
        double _30;
        const ChargeParametersRule *rules;
        size_t num_rules;
    };

    const ChargeParameters &GetChargeParameters();

}
