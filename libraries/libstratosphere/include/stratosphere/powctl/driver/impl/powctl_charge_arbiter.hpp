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
#include <stratosphere/powctl/driver/impl/powctl_select_charger_parameters.hpp>

namespace ams::powctl::driver::impl {

    class ChargeArbiter {
        private:
            const ChargeParametersRule *rules;
            size_t num_rules;
            int charge_voltage_limit;
            BatteryTemperatureLevel temperature_level;
            int avg_v_cell;
            int open_circuit_voltage;
            bool has_battery_done_current;
            int battery_done_current;
            PowerState power_state;
            const ChargeParametersRule *selected_rule;
            bool check_battery_done_current;
        private:
            static constexpr bool IsInRange(int value, int min, int max) {
                if (!(min <= value)) {
                    return false;
                }

                if (max == std::numeric_limits<int>::max()) {
                    return value <= max;
                } else {
                    return value < max;
                }
            }

            bool IsAcceptablePowerState(const PowerState *acceptable, size_t num_acceptable) const {
                for (size_t i = 0; i < num_acceptable; ++i) {
                    if (this->power_state == acceptable[i]) {
                        return true;
                    }
                }
                return false;
            }
        public:
            ChargeArbiter(const ChargeParametersRule *r, size_t nr, int cvl)
                : rules(r), num_rules(nr), charge_voltage_limit(cvl), temperature_level(BatteryTemperatureLevel::Medium),
                  avg_v_cell(4080), open_circuit_voltage(4001), has_battery_done_current(false), battery_done_current(0),
                  power_state(PowerState::FullAwake), selected_rule(nullptr), check_battery_done_current(false)
            {
                this->UpdateSelectedRule();
            }

            void SetBatteryTemperatureLevel(BatteryTemperatureLevel btl) {
                this->temperature_level = btl;
                this->UpdateSelectedRule();
            }

            void SetBatteryAverageVCell(int avg) {
                this->avg_v_cell = avg;
                this->UpdateSelectedRule();
            }

            void SetBatteryOpenCircuitVoltage(int ocv) {
                this->open_circuit_voltage = ocv;
                this->UpdateSelectedRule();
            }

            void SetBatteryDoneCurrent(int current) {
                this->battery_done_current     = current;
                this->has_battery_done_current = true;
                this->UpdateSelectedRule();
            }

            void SetPowerState(PowerState ps) {
                this->power_state = ps;
                this->UpdateSelectedRule();
            }

            int GetChargeVoltageLimit() const {
                return this->charge_voltage_limit;
            }

            bool IsBatteryDoneCurrentAcceptable(int current) const {
                const auto *rule = this->GetSelectedRule();
                AMS_ASSERT(rule != nullptr);

                return IsInRange(0, rule->min_battery_done_current, rule->max_battery_done_current);
            }

            const ChargeParametersRule *GetSelectedRule() const {
                return this->selected_rule;
            }

            void UpdateSelectedRule() {
                /* Try to find an entry that fits our current requirements. */
                const ChargeParametersRule *best_rule = nullptr;
                for (size_t i = 0; i < this->num_rules; ++i) {
                    /* Get the current rule. */
                    const ChargeParametersRule &cur_rule = this->rules[i];

                    /* Check the temperature level. */
                    if (this->temperature_level != cur_rule.temperature_level) {
                        continue;
                    }

                    /* Check that average voltage is in range. */
                    if (!IsInRange(this->avg_v_cell, cur_rule.min_avg_v_cell, cur_rule.max_avg_v_cell)) {
                        continue;
                    }

                    /* Check that open circuit voltage is in range. */
                    if (!IsInRange(this->open_circuit_voltage, cur_rule.min_open_circuit_voltage, cur_rule.max_open_circuit_voltage)) {
                        continue;
                    }

                    /* Check if our power state is acceptable. */
                    if (!this->IsAcceptablePowerState(cur_rule.acceptable_power_states, cur_rule.num_acceptable_power_states)) {
                        continue;
                    }

                    /* The limit is probably acceptable. */
                    if (this->selected_rule != std::addressof(cur_rule)) {
                        /* We're selecting a new rule. Check if our need to deal with battery current is acceptable. */
                        if (cur_rule.check_battery_current && this->check_battery_done_current) {
                            continue;
                        }

                        /* Set whether we need to check the battery done current. */
                        this->has_battery_done_current = false;
                        this->check_battery_done_current |= cur_rule.check_battery_current;
                    } else {
                        /* We're selecting the currently selected rule. Make sure the battery done current is acceptable if we have one. */
                        if (this->has_battery_done_current && !IsInRange(this->battery_done_current, cur_rule.min_battery_done_current, cur_rule.max_battery_done_current)) {
                            continue;
                        }
                    }

                    /* Select the current rule. */
                    best_rule = std::addressof(cur_rule);
                    break;
                }

                /* Update our selected rule. */
                this->selected_rule = best_rule;
            }
    };

}