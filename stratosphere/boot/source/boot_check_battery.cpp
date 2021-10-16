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
#include <stratosphere.hpp>
#include "boot_check_battery.hpp"
#include "boot_battery_icons.hpp"
#include "boot_boot_reason.hpp"
#include "boot_pmic_driver.hpp"
#include "boot_battery_driver.hpp"
#include "boot_charger_driver.hpp"
#include "boot_power_utils.hpp"

namespace ams::boot {

    namespace {

        /* Value definitions. */
        constexpr inline double BatteryLevelThresholdForBoot       = 3.0;
        constexpr inline double BatteryLevelThresholdForFullCharge = 99.0;

        constexpr inline int BatteryVoltageThresholdConnected    = 4000;
        constexpr inline int BatteryVoltageThresholdDisconnected = 3650;

        /* Types. */
        enum class CheckBatteryResult {
            Success,
            Shutdown,
            Reboot,
        };

        class BatteryChecker {
            private:
                boot::ChargerDriver &m_charger_driver;
                boot::BatteryDriver &m_battery_driver;
                const powctl::driver::impl::ChargeParameters &m_charge_parameters;
                powctl::driver::impl::ChargeArbiter m_charge_arbiter;
                powctl::ChargeCurrentState m_charge_current_state;
                int m_fast_charge_current_limit;
                int m_charge_voltage_limit;
                int m_battery_compensation;
                int m_voltage_clamp;
                TimeSpan m_charging_done_interval;
                bool m_has_start_time;
                TimeSpan m_start_time;
            private:
                bool IsChargeDone();
                void UpdateChargeDoneCurrent();
                void ApplyArbiterRule();
                void PrintBatteryStatus(float raw_charge, int voltage, int voltage_threshold);
                CheckBatteryResult LoopCheckBattery(bool reboot_on_power_button_press, bool return_on_enough_battery, bool shutdown_on_full_battery, bool show_display, bool show_charging_display);

                void UpdateStartTime() {
                    /* Update start time. */
                    m_start_time = os::ConvertToTimeSpan(os::GetSystemTick());
                    m_has_start_time = true;
                }
            public:
                BatteryChecker(boot::ChargerDriver &cd, boot::BatteryDriver &bd, const powctl::driver::impl::ChargeParameters &cp, int cvl) : m_charger_driver(cd), m_battery_driver(bd), m_charge_parameters(cp), m_charge_arbiter(cp.rules, cp.num_rules, cvl), m_charging_done_interval(TimeSpan::FromSeconds(2)), m_has_start_time(false) {
                    /* Get parameters from charger. */
                    if (R_FAILED(m_charger_driver.GetChargeCurrentState(std::addressof(m_charge_current_state)))) {
                        boot::ShutdownSystem();
                    }
                    if (R_FAILED(m_charger_driver.GetFastChargeCurrentLimit(std::addressof(m_fast_charge_current_limit)))) {
                        boot::ShutdownSystem();
                    }
                    if (R_FAILED(m_charger_driver.GetChargeVoltageLimit(std::addressof(m_charge_voltage_limit)))) {
                        boot::ShutdownSystem();
                    }
                    if (R_FAILED(m_charger_driver.GetBatteryCompensation(std::addressof(m_battery_compensation)))) {
                        boot::ShutdownSystem();
                    }
                    if (R_FAILED(m_charger_driver.GetVoltageClamp(std::addressof(m_voltage_clamp)))) {
                        boot::ShutdownSystem();
                    }

                    /* Update start time. */
                    this->UpdateStartTime();
                }

                CheckBatteryResult LoopCheckBattery(spl::BootReason boot_reason) {
                    if (boot_reason == spl::BootReason_RtcAlarm2) {
                        /* RTC Alarm 2 boot (QuasiOff) */
                        return this->LoopCheckBattery(true, false, true, false, false);
                    } else if (boot_reason == spl::BootReason_AcOk) {
                        /* ACOK boot */
                        return this->LoopCheckBattery(true, true, false, true, true);
                    } else {
                        /* Normal boot */
                        return this->LoopCheckBattery(false, true, false, true, false);
                    }
                }

                void UpdateCharger();
        };

        void BatteryChecker::PrintBatteryStatus(float raw_charge, int voltage, int voltage_threshold) {
            /* TODO: Print charge/voltage/threshold. */
            AMS_UNUSED(raw_charge, voltage, voltage_threshold);

            /* Get various battery metrics. */
            int avg_current, current;
            float temp, voltage_fuel_gauge_percentage;
            if (R_FAILED(m_battery_driver.GetAverageCurrent(std::addressof(avg_current)))) {
                return;
            }
            if (R_FAILED(m_battery_driver.GetCurrent(std::addressof(current)))) {
                return;
            }
            if (R_FAILED(m_battery_driver.GetTemperature(std::addressof(temp)))) {
                return;
            }
            if (R_FAILED(m_battery_driver.GetVoltageFuelGaugePercentage(std::addressof(voltage_fuel_gauge_percentage)))) {
                return;
            }

            /* TODO: Print the things we just got. */
            AMS_UNUSED(avg_current, current, temp, voltage_fuel_gauge_percentage);
        }

        bool BatteryChecker::IsChargeDone() {
            /* Get the charger status. */
            boot::ChargerStatus charger_status;
            if (R_FAILED(m_charger_driver.GetChargerStatus(std::addressof(charger_status)))) {
                boot::ShutdownSystem();
            }

            /* If charge status isn't done, we're not done. */
            if (charger_status != boot::ChargerStatus_ChargeTerminationDone) {
                return false;
            }

            /* Return whether a done current of zero is acceptable. */
            return m_charge_arbiter.IsBatteryDoneCurrentAcceptable(0);
        }

        void BatteryChecker::UpdateChargeDoneCurrent() {
            int done_current = 0;
            if (m_has_start_time && (os::ConvertToTimeSpan(os::GetSystemTick()) - m_start_time) >= m_charging_done_interval) {
                /* Get the current. */
                if (R_FAILED(m_battery_driver.GetCurrent(std::addressof(done_current)))) {
                    boot::ShutdownSystem();
                }
            } else {
                /* Get the charger status. */
                boot::ChargerStatus charger_status;
                if (R_FAILED(m_charger_driver.GetChargerStatus(std::addressof(charger_status)))) {
                    boot::ShutdownSystem();
                }

                /* If the charger status isn't done, don't update. */
                if (charger_status != boot::ChargerStatus_ChargeTerminationDone) {
                    return;
                }
            }

            /* Update done current. */
            m_charge_arbiter.SetBatteryDoneCurrent(done_current);
        }

        void BatteryChecker::UpdateCharger() {
            /* Get the battery temperature. */
            float temp;
            if (R_FAILED(m_battery_driver.GetTemperature(std::addressof(temp)))) {
                boot::ShutdownSystem();
            }

            /* Update the temperature level. */
            powctl::BatteryTemperatureLevel temp_level;
            if (temp < static_cast<float>(m_charge_parameters.temp_min)) {
                temp_level = powctl::BatteryTemperatureLevel::TooLow;
            } else if (temp < static_cast<float>(m_charge_parameters.temp_low)) {
                temp_level = powctl::BatteryTemperatureLevel::Low;
            } else if (temp < static_cast<float>(m_charge_parameters.temp_high)) {
                temp_level = powctl::BatteryTemperatureLevel::Medium;
            } else if (temp < static_cast<float>(m_charge_parameters.temp_max)) {
                temp_level = powctl::BatteryTemperatureLevel::High;
            } else {
                temp_level = powctl::BatteryTemperatureLevel::TooHigh;
            }
            m_charge_arbiter.SetBatteryTemperatureLevel(temp_level);

            /* Update average voltage. */
            int avg_v_cell;
            if (R_FAILED(m_battery_driver.GetAverageVCell(std::addressof(avg_v_cell)))) {
                boot::ShutdownSystem();
            }
            m_charge_arbiter.SetBatteryAverageVCell(avg_v_cell);

            /* Update voltage fuel gauge percentage. */
            float vfgp;
            if (R_FAILED(m_battery_driver.GetVoltageFuelGaugePercentage(std::addressof(vfgp)))) {
                boot::ShutdownSystem();
            }
            m_charge_arbiter.SetBatteryVoltageFuelGaugePercentage(vfgp);

            /* Update charge done current. */
            this->UpdateChargeDoneCurrent();

            /* Update arbiter power state. */
            m_charge_arbiter.SetPowerState(powctl::PowerState::ShutdownChargeMain);

            /* Apply the newly selected rule. */
            this->ApplyArbiterRule();
        }

        void BatteryChecker::ApplyArbiterRule() {
            /* Get the selected rule. */
            const auto *rule = m_charge_arbiter.GetSelectedRule();
            AMS_ASSERT(rule != nullptr);

            /* Check if we need to perform charger initialization. */
            const bool reinit_charger = rule->reinitialize_charger;
            const auto cur_charge_current_state = m_charge_current_state;

            /* Set the charger to not charging while we make changes. */
            if (!reinit_charger || cur_charge_current_state != powctl::ChargeCurrentState_NotCharging) {
                if (R_FAILED(m_charger_driver.SetChargeCurrentState(powctl::ChargeCurrentState_NotCharging))) {
                    boot::ShutdownSystem();
                }
                m_charge_current_state = powctl::ChargeCurrentState_NotCharging;

                /* Update start time. */
                this->UpdateStartTime();
            }

            /* Process fast charge current limit when rule is smaller. */
            const auto rule_fast_charge_current_limit = rule->fast_charge_current_limit;
            const auto cur_fast_charge_current_limit  = m_fast_charge_current_limit;
            if (rule_fast_charge_current_limit < cur_fast_charge_current_limit) {
                if (R_FAILED(m_charger_driver.SetFastChargeCurrentLimit(rule_fast_charge_current_limit))) {
                    boot::ShutdownSystem();
                }
                m_fast_charge_current_limit = rule_fast_charge_current_limit;

                /* Update start time. */
                this->UpdateStartTime();
            }

            /* Process charge voltage limit when rule is smaller. */
            const auto rule_charge_voltage_limit = std::min(rule->charge_voltage_limit, m_charge_arbiter.GetChargeVoltageLimit());
            const auto cur_charge_voltage_limit  = m_charge_voltage_limit;
            if (rule_charge_voltage_limit < cur_charge_voltage_limit) {
                if (R_FAILED(m_charger_driver.SetChargeVoltageLimit(rule_charge_voltage_limit))) {
                    boot::ShutdownSystem();
                }
                m_charge_voltage_limit = rule_charge_voltage_limit;

                /* Update start time. */
                this->UpdateStartTime();
            }

            /* Process battery compensation when rule is smaller. */
            const auto rule_battery_compensation = rule->battery_compensation;
            const auto cur_battery_compensation  = m_battery_compensation;
            if (rule_battery_compensation < cur_battery_compensation) {
                if (R_FAILED(m_charger_driver.SetBatteryCompensation(rule_battery_compensation))) {
                    boot::ShutdownSystem();
                }
                m_battery_compensation = rule_battery_compensation;

                /* Update start time. */
                this->UpdateStartTime();
            }

            /* Process voltage clamp when rule is smaller. */
            const auto rule_voltage_clamp = rule->voltage_clamp;
            const auto cur_voltage_clamp  = m_voltage_clamp;
            if (rule_voltage_clamp < cur_voltage_clamp) {
                if (R_FAILED(m_charger_driver.SetVoltageClamp(rule_voltage_clamp))) {
                    boot::ShutdownSystem();
                }
                m_voltage_clamp = rule_voltage_clamp;

                /* Update start time. */
                this->UpdateStartTime();
            }

            /* Process voltage clamp when rule is larger. */
            if (rule_voltage_clamp > cur_voltage_clamp) {
                if (R_FAILED(m_charger_driver.SetVoltageClamp(rule_voltage_clamp))) {
                    boot::ShutdownSystem();
                }
                m_voltage_clamp = rule_voltage_clamp;

                /* Update start time. */
                this->UpdateStartTime();
            }

            /* Process battery compensation when rule is larger. */
            if (rule_battery_compensation > cur_battery_compensation) {
                if (R_FAILED(m_charger_driver.SetBatteryCompensation(rule_battery_compensation))) {
                    boot::ShutdownSystem();
                }
                m_battery_compensation = rule_battery_compensation;

                /* Update start time. */
                this->UpdateStartTime();
            }

            /* Process fast charge current limit when rule is larger. */
            if (rule_fast_charge_current_limit > cur_fast_charge_current_limit) {
                if (R_FAILED(m_charger_driver.SetFastChargeCurrentLimit(rule_fast_charge_current_limit))) {
                    boot::ShutdownSystem();
                }
                m_fast_charge_current_limit = rule_fast_charge_current_limit;

                /* Update start time. */
                this->UpdateStartTime();
            }

            /* Process charge voltage limit when rule is larger. */
            if (rule_charge_voltage_limit > cur_charge_voltage_limit) {
                if (R_FAILED(m_charger_driver.SetChargeVoltageLimit(rule_charge_voltage_limit))) {
                    boot::ShutdownSystem();
                }
                m_charge_voltage_limit = rule_charge_voltage_limit;

                /* Update start time. */
                this->UpdateStartTime();
            }

            /* If we're not charging and we expect to reinitialize the charger, do so. */
            if (cur_charge_current_state != powctl::ChargeCurrentState_Charging && reinit_charger) {
                if (R_FAILED(m_charger_driver.SetChargeCurrentState(powctl::ChargeCurrentState_Charging))) {
                    boot::ShutdownSystem();
                }
                m_charge_current_state = powctl::ChargeCurrentState_Charging;

                /* Update start time. */
                this->UpdateStartTime();
            }
        }

        CheckBatteryResult BatteryChecker::LoopCheckBattery(bool reboot_on_power_button_press, bool return_on_enough_battery, bool shutdown_on_full_battery, bool show_display, bool show_charging_display) {
            /* Ensure that if we show a charging icon, we stop showing it when we're done. */
            bool is_showing_charging_icon = false;
            ON_SCOPE_EXIT {
                if (is_showing_charging_icon) {
                    boot::EndShowChargingIcon();
                    is_showing_charging_icon = false;
                }
            };

            /* Show the charging display, if we should. */
            if (show_charging_display) {
                /* Get the raw battery charge. */
                float raw_battery_charge;
                if (R_FAILED(m_battery_driver.GetChargePercentage(std::addressof(raw_battery_charge)))) {
                    return CheckBatteryResult::Shutdown;
                }

                /* Display the battery with the appropriate percentage. */
                const auto battery_charge = powctl::impl::ConvertBatteryChargePercentage(raw_battery_charge);
                boot::StartShowChargingIcon(battery_charge);
                is_showing_charging_icon = true;
            }

            /* Loop, checking the battery status. */
            TimeSpan last_progress_time = TimeSpan(0);
            while (true) {
                /* Get the raw battery charge. */
                float raw_battery_charge;
                if (R_FAILED(m_battery_driver.GetChargePercentage(std::addressof(raw_battery_charge)))) {
                    return CheckBatteryResult::Shutdown;
                }

                /* Get the average vcell. */
                int battery_voltage;
                if (R_FAILED(m_battery_driver.GetAverageVCell(std::addressof(battery_voltage)))) {
                    return CheckBatteryResult::Shutdown;
                }

                /* Get whether we're connected to charger. */
                bool ac_ok;
                if (R_FAILED((boot::PmicDriver().GetAcOk(std::addressof(ac_ok))))) {
                    return CheckBatteryResult::Shutdown;
                }

                /* Decide on a battery voltage threshold. */
                const auto battery_voltage_threshold = ac_ok ? BatteryVoltageThresholdConnected : BatteryVoltageThresholdDisconnected;

                /* Check if we should return. */
                if (return_on_enough_battery) {
                    if (raw_battery_charge >= BatteryLevelThresholdForBoot || battery_voltage >= battery_voltage_threshold) {
                        this->PrintBatteryStatus(raw_battery_charge, battery_voltage, battery_voltage_threshold);
                        return CheckBatteryResult::Success;
                    }
                }

                /* Otherwise, check if we should shut down. */
                if (shutdown_on_full_battery) {
                    if (raw_battery_charge >= BatteryLevelThresholdForFullCharge || this->IsChargeDone()) {
                        return CheckBatteryResult::Shutdown;
                    }
                }

                /* Perform periodic printing. */
                constexpr TimeSpan PrintProgressInterval = TimeSpan::FromSeconds(10);
                const auto cur_time = os::ConvertToTimeSpan(os::GetSystemTick());
                if ((cur_time - last_progress_time) >= PrintProgressInterval) {
                    last_progress_time = cur_time;
                    this->PrintBatteryStatus(raw_battery_charge, battery_voltage, battery_voltage_threshold);
                }

                /* If we've gotten to this point, we have insufficient battery to boot. If we aren't charging, show low battery and shutdown. */
                if (!ac_ok) {
                    this->PrintBatteryStatus(raw_battery_charge, battery_voltage, battery_voltage_threshold);
                    if (show_display && !is_showing_charging_icon) {
                        boot::ShowLowBatteryIcon();
                    }
                    return CheckBatteryResult::Shutdown;
                }

                /* Check if we should reboot due to a power button press. */
                if (reboot_on_power_button_press) {
                    /* Get the power button value. */
                    bool power_button_pressed;
                    if (R_FAILED((boot::PmicDriver().GetPowerButtonPressed(std::addressof(power_button_pressed))))) {
                        return CheckBatteryResult::Shutdown;
                    }

                    /* Handle the press (or not). */
                    if (power_button_pressed) {
                        return CheckBatteryResult::Reboot;
                    }
                }

                /* If we got to this point, we should show the low-battery charging screen. */
                if (show_display && !is_showing_charging_icon) {
                    boot::StartShowLowBatteryChargingIcon();
                    is_showing_charging_icon = true;
                }

                /* Wait a bit before checking again. */
                constexpr auto BatteryChargeCheckInterval = TimeSpan::FromMilliSeconds(20);
                os::SleepThread(BatteryChargeCheckInterval);

                /* Update the charger. */
                this->UpdateCharger();
            }
        }

    }

    void CheckBatteryCharge() {
        /* Open a sessions for the charger/battery. */
        boot::ChargerDriver charger_driver;
        boot::BatteryDriver battery_driver;

        /* Check if the battery is removed. */
        {
            bool removed = false;
            if (R_FAILED(battery_driver.IsBatteryRemoved(std::addressof(removed))) || removed) {
                boot::ShutdownSystem();
            }
        }

        /* Get the boot reason. */
        const auto boot_reason = boot::GetBootReason();

        /* Initialize the charger driver. */
        if (R_FAILED(charger_driver.Initialize(boot_reason != spl::BootReason_RtcAlarm2)))

        /* Check that the charger input limit is greater than 150 milli-amps. */
        {
            int input_current_limit_ma;
            if (R_FAILED(charger_driver.GetInputCurrentLimit(std::addressof(input_current_limit_ma)))) {
                boot::ShutdownSystem();
            }

            if (input_current_limit_ma <= 150) {
                charger_driver.SetChargerConfiguration(powctl::ChargerConfiguration_ChargeDisable);
                boot::ShutdownSystem();
            }
        }

        /* Get the charge parameters. */
        const auto &charge_parameters = powctl::driver::impl::GetChargeParameters();

        /* Get the charge voltage limit. */
        int charge_voltage_limit_mv;
        if (boot_reason != spl::BootReason_RtcAlarm2 || charge_parameters.unknown_x_table == nullptr || charge_parameters.x_table_size == 0) {
            charge_voltage_limit_mv = charge_parameters.default_charge_voltage_limit;
        } else {
            if (R_FAILED(charger_driver.GetChargeVoltageLimit(std::addressof(charge_voltage_limit_mv)))) {
                boot::ShutdownSystem();
            }
        }

        /* Create and update a battery checker. */
        BatteryChecker battery_checker(charger_driver, battery_driver, charge_parameters, charge_voltage_limit_mv);
        battery_checker.UpdateCharger();

        /* Set the display brightness to 25%. */
        boot::SetDisplayBrightness(25);

        /* Check the battery. */
        const CheckBatteryResult check_result = battery_checker.LoopCheckBattery(boot_reason);

        /* Set the display brightness to 100%. */
        boot::SetDisplayBrightness(100);

        /* Handle the check result. */
        switch (check_result) {
            case CheckBatteryResult::Success:
                break;
            case CheckBatteryResult::Shutdown:
                boot::ShutdownSystem();
                break;
            case CheckBatteryResult::Reboot:
                boot::RebootSystem();
                break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

}
