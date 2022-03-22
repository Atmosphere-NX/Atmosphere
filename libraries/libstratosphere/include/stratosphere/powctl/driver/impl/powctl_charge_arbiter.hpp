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
#include <stratosphere/powctl/powctl_types.hpp>
#include <stratosphere/powctl/driver/impl/powctl_select_charger_parameters.hpp>

namespace ams::powctl::driver::impl {

    class ChargeArbiter {
        private:
            const ChargeParametersRule *m_rules;
            size_t m_num_rules;
            int m_charge_voltage_limit;
            BatteryTemperatureLevel m_temperature_level;
            int m_avg_v_cell;
            float m_voltage_fuel_gauge_percentage;
            bool m_has_battery_done_current;
            int m_battery_done_current;
            PowerState m_power_state;
            const ChargeParametersRule *m_selected_rule;
            bool m_check_battery_done_current;
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
            static constexpr bool IsInRangeFloat(float value, float min, float max) {
                if (!(min <= value)) {
                    return false;
                }

                if (max == std::numeric_limits<float>::max()) {
                    return value <= max;
                } else {
                    return value < max;
                }
            }

            bool IsAcceptablePowerState(const PowerState *acceptable, size_t num_acceptable) const {
                for (size_t i = 0; i < num_acceptable; ++i) {
                    if (m_power_state == acceptable[i]) {
                        return true;
                    }
                }
                return false;
            }
        public:
            ChargeArbiter(const ChargeParametersRule *r, size_t nr, int cvl)
                : m_rules(r), m_num_rules(nr), m_charge_voltage_limit(cvl), m_temperature_level(BatteryTemperatureLevel::Medium),
                  m_avg_v_cell(4080), m_voltage_fuel_gauge_percentage(25.0), m_has_battery_done_current(false), m_battery_done_current(0),
                  m_power_state(PowerState::FullAwake), m_selected_rule(nullptr), m_check_battery_done_current(false)
            {
                this->UpdateSelectedRule();
            }

            void SetBatteryTemperatureLevel(BatteryTemperatureLevel btl) {
                m_temperature_level = btl;
                this->UpdateSelectedRule();
            }

            void SetBatteryAverageVCell(int avg) {
                m_avg_v_cell = avg;
                this->UpdateSelectedRule();
            }

            void SetBatteryVoltageFuelGaugePercentage(float pct) {
                m_voltage_fuel_gauge_percentage = pct;
                this->UpdateSelectedRule();
            }

            void SetBatteryDoneCurrent(int current) {
                m_battery_done_current     = current;
                m_has_battery_done_current = true;
                this->UpdateSelectedRule();
            }

            void SetPowerState(PowerState ps) {
                m_power_state = ps;
                this->UpdateSelectedRule();
            }

            int GetChargeVoltageLimit() const {
                return m_charge_voltage_limit;
            }

            bool IsBatteryDoneCurrentAcceptable(int current) const {
                const auto *rule = this->GetSelectedRule();
                AMS_ASSERT(rule != nullptr);

                return IsInRange(current, rule->min_battery_done_current, rule->max_battery_done_current);
            }

            const ChargeParametersRule *GetSelectedRule() const {
                return m_selected_rule;
            }

            void UpdateSelectedRule() {
                /* Try to find an entry that fits our current requirements. */
                const ChargeParametersRule *best_rule = nullptr;
                for (size_t i = 0; i < m_num_rules; ++i) {
                    /* Get the current rule. */
                    const ChargeParametersRule &cur_rule = m_rules[i];

                    /* Check the temperature level. */
                    if (m_temperature_level != cur_rule.temperature_level) {
                        continue;
                    }

                    /* Check that average voltage is in range. */
                    if (!IsInRange(m_avg_v_cell, cur_rule.min_avg_v_cell, cur_rule.max_avg_v_cell)) {
                        continue;
                    }

                    /* Check that voltage fuel gauge percentage is in range. */
                    if (!IsInRangeFloat(m_voltage_fuel_gauge_percentage, cur_rule.min_voltage_fuel_gauge_percentage, cur_rule.max_voltage_fuel_gauge_percentage)) {
                        continue;
                    }

                    /* Check if our power state is acceptable. */
                    if (!this->IsAcceptablePowerState(cur_rule.acceptable_power_states, cur_rule.num_acceptable_power_states)) {
                        continue;
                    }

                    /* The limit is probably acceptable. */
                    if (m_selected_rule != std::addressof(cur_rule)) {
                        /* We're selecting a new rule. Check if our need to deal with battery current is acceptable. */
                        if (cur_rule.check_battery_current && m_check_battery_done_current) {
                            continue;
                        }

                        /* Determine whether we should check battery done current. */
                        bool check_battery_done_current;

                        if (m_selected_rule != nullptr && m_selected_rule->check_battery_current) {
                            if (m_selected_rule->temperature_level == m_temperature_level &&
                                IsInRange(m_avg_v_cell, m_selected_rule->min_avg_v_cell, m_selected_rule->max_avg_v_cell) &&
                                IsInRangeFloat(m_voltage_fuel_gauge_percentage, m_selected_rule->min_voltage_fuel_gauge_percentage, m_selected_rule->max_voltage_fuel_gauge_percentage))
                            {
                                check_battery_done_current = m_has_battery_done_current && !IsInRange(m_battery_done_current, m_selected_rule->min_battery_done_current, m_selected_rule->max_battery_done_current);
                            } else {
                                check_battery_done_current = true;
                            }
                        } else {
                            check_battery_done_current = false;
                        }

                        /* Set whether we need to check the battery done current. */
                        m_has_battery_done_current = false;
                        m_check_battery_done_current |= check_battery_done_current;
                    } else {
                        /* We're selecting the currently selected rule. Make sure the battery done current is acceptable if we have one. */
                        if (m_has_battery_done_current && !IsInRange(m_battery_done_current, cur_rule.min_battery_done_current, cur_rule.max_battery_done_current)) {
                            continue;
                        }
                    }

                    /* Select the current rule. */
                    best_rule = std::addressof(cur_rule);
                    break;
                }

                /* Update our selected rule. */
                m_selected_rule = best_rule;
            }
    };

}