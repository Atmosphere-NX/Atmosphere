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
#include "boot_battery_driver.hpp"
#include "boot_battery_icons.hpp"
#include "boot_boot_reason.hpp"
#include "boot_calibration.hpp"
#include "boot_charger_driver.hpp"
#include "boot_check_battery.hpp"
#include "boot_pmic_driver.hpp"
#include "boot_power_utils.hpp"

namespace ams::boot {

    namespace {

        /* Types. */
        enum class CheckBatteryResult {
            Success,
            Shutdown,
            Reboot,
        };

        struct BatteryChargeParameters {
            u32 temp_min;
            u32 temp_low;
            u32 temp_high;
            u32 temp_max;
            u32 allow_high_temp_charge_max_voltage;
            u32 charge_voltage_limit_default;
            u32 charge_voltage_limit_high_temp;
            u32 allow_fast_charge_min_temp;
            u32 allow_fast_charge_min_voltage;
            u32 fast_charge_current_limit_default;
            u32 fast_charge_current_limit_low_temp;
            u32 fast_charge_current_limit_low_voltage;
        };

        /* Battery parameters. */
        constexpr BatteryChargeParameters BatteryChargeParameters0 = {
            .temp_min  = 4,
            .temp_low  = 17,
            .temp_high = 51,
            .temp_max  = 60,
            .allow_high_temp_charge_max_voltage = 4050,
            .charge_voltage_limit_default       = 4208,
            .charge_voltage_limit_high_temp     = 3952,
            .allow_fast_charge_min_voltage          = 3320,
            .fast_charge_current_limit_default      = 0x800,
            .fast_charge_current_limit_low_temp     = 0x300,
            .fast_charge_current_limit_low_voltage  = 0x200,
        };

        constexpr BatteryChargeParameters BatteryChargeParameters1 = {
            .temp_min  = 4,
            .temp_low  = 17,
            .temp_high = 51,
            .temp_max  = 59,
            .allow_high_temp_charge_max_voltage = 3984,
            .charge_voltage_limit_default       = 4208,
            .charge_voltage_limit_high_temp     = 3984,
            .allow_fast_charge_min_voltage          = 0,
            .fast_charge_current_limit_default      = 0x600,
            .fast_charge_current_limit_low_temp     = 0x240,
            .fast_charge_current_limit_low_voltage  = 0x600,
        };

        constexpr BatteryChargeParameters BatteryChargeParameters2 = {
            .temp_min  = 4,
            .temp_low  = 17,
            .temp_high = 51,
            .temp_max  = 59,
            .allow_high_temp_charge_max_voltage = 4080,
            .charge_voltage_limit_default       = 4320,
            .charge_voltage_limit_high_temp     = 4080,
            .allow_fast_charge_min_voltage          = 0,
            .fast_charge_current_limit_default      = 0x680,
            .fast_charge_current_limit_low_temp     = 0x280,
            .fast_charge_current_limit_low_voltage  = 0x680,
        };

        constexpr const BatteryChargeParameters *GetBatteryChargeParameters(u32 battery_version) {
            switch (battery_version) {
                case 0:
                    return &BatteryChargeParameters0;
                case 1:
                    return &BatteryChargeParameters1;
                case 2:
                    return &BatteryChargeParameters2;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

        /* Helpers. */
        void UpdateCharger(PmicDriver *pmic_driver, ChargerDriver *charger_driver, BatteryDriver *battery_driver, const BatteryChargeParameters *params, u32 charge_voltage_limit) {
            double temperature;
            u32 battery_voltage;

            if (R_FAILED(battery_driver->GetTemperature(&temperature)) || R_FAILED(battery_driver->GetAverageVCell(&battery_voltage))) {
                pmic_driver->ShutdownSystem();
            }

            bool enable_charge = true;
            if (temperature < double(params->temp_min)) {
                enable_charge = false;
            } else if (double(params->temp_high) <= temperature && temperature < double(params->temp_max)) {
                if (battery_voltage < params->allow_high_temp_charge_max_voltage) {
                    charge_voltage_limit = std::min(charge_voltage_limit, params->charge_voltage_limit_high_temp);
                } else {
                    enable_charge = false;
                }
            } else if (double(params->temp_max) <= temperature) {
                enable_charge = false;
                if (battery_voltage < params->allow_high_temp_charge_max_voltage) {
                    charge_voltage_limit = std::min(charge_voltage_limit, params->charge_voltage_limit_high_temp);
                }
            }

            u32 fast_charge_current_limit = params->fast_charge_current_limit_default;
            if (temperature < double(params->temp_low)) {
                fast_charge_current_limit = std::min(fast_charge_current_limit, params->fast_charge_current_limit_low_temp);
            }
            if (battery_voltage < params->allow_fast_charge_min_voltage) {
                fast_charge_current_limit = std::min(fast_charge_current_limit, params->fast_charge_current_limit_low_voltage);
            }

            if (R_FAILED(charger_driver->SetChargeEnabled(enable_charge))) {
                pmic_driver->ShutdownSystem();
            }
            if (R_FAILED(charger_driver->SetChargeVoltageLimit(charge_voltage_limit))) {
                pmic_driver->ShutdownSystem();
            }
            if (R_FAILED(charger_driver->SetFastChargeCurrentLimit(fast_charge_current_limit))) {
                pmic_driver->ShutdownSystem();
            }
        }

        bool IsSufficientBattery(u32 battery_voltage, bool ac_ok) {
            /* Nintendo has stuff for updating a static variable every 10 seconds here, but this seems, again, to be debug leftovers. */
            const u32 required_voltage = ac_ok ? 4000 : 3650;
            return battery_voltage >= required_voltage;
        }

        CheckBatteryResult LoopCheckBattery(PmicDriver *pmic_driver, ChargerDriver *charger_driver, BatteryDriver *battery_driver, const BatteryChargeParameters *params, u32 charge_voltage_limit, bool reboot_on_power_button_pressed, bool succeed_on_sufficient_battery, bool shutdown_on_full_battery, bool can_show_battery_icon, bool can_show_charging_icon) {
            bool is_showing_charging_icon = false;
            ON_SCOPE_EXIT {
                if (is_showing_charging_icon) {
                    EndShowChargingIcon();
                }
            };

            if (can_show_charging_icon) {
                size_t battery_percentage;
                if (R_FAILED(battery_driver->GetBatteryPercentage(&battery_percentage))) {
                    return CheckBatteryResult::Shutdown;
                }
                StartShowChargingIcon(battery_percentage, true);
                is_showing_charging_icon = true;
            }

            while (true) {
                double battery_charge;
                if (R_FAILED(battery_driver->GetSocRep(&battery_charge))) {
                    return CheckBatteryResult::Shutdown;
                }
                if (succeed_on_sufficient_battery && battery_charge >= 3.0) {
                    return CheckBatteryResult::Success;
                } else if (shutdown_on_full_battery && battery_charge >= 99.0) {
                    return CheckBatteryResult::Shutdown;
                } else {
                    /* Nintendo has logic for checking a value every 10 seconds. */
                    /* They never do anything with this value though, so it's probably just leftovers from debug? */
                }

                bool ac_ok;
                if (R_FAILED(pmic_driver->GetAcOk(&ac_ok))) {
                    return CheckBatteryResult::Shutdown;
                }

                u32 battery_voltage;
                if (R_FAILED(battery_driver->GetAverageVCell(&battery_voltage))) {
                    return CheckBatteryResult::Shutdown;
                }

                if (succeed_on_sufficient_battery && IsSufficientBattery(battery_voltage, ac_ok)) {
                    return CheckBatteryResult::Success;
                }

                if (!ac_ok) {
                    if (can_show_battery_icon && !is_showing_charging_icon) {
                        ShowLowBatteryIcon();
                    }
                    return CheckBatteryResult::Shutdown;
                }

                if (reboot_on_power_button_pressed) {
                    bool power_button_pressed;
                    if (R_FAILED(pmic_driver->GetPowerButtonPressed(&power_button_pressed))) {
                        return CheckBatteryResult::Shutdown;
                    }
                    if (power_button_pressed) {
                        return CheckBatteryResult::Reboot;
                    }
                }

                if (can_show_battery_icon && !is_showing_charging_icon) {
                    StartShowChargingIcon(1, false);
                    is_showing_charging_icon = true;
                }

                svcSleepThread(20'000'000ul);
                UpdateCharger(pmic_driver, charger_driver, battery_driver, params, charge_voltage_limit);
            }
        }
    }

    void CheckBatteryCharge() {
        PmicDriver pmic_driver;
        BatteryDriver battery_driver;
        ChargerDriver charger_driver;

        if (R_FAILED(battery_driver.InitializeBatteryParameters())) {
            pmic_driver.ShutdownSystem();
        }
        {
            bool removed;
            if (R_FAILED(battery_driver.IsBatteryRemoved(&removed)) || removed) {
                pmic_driver.ShutdownSystem();
            }
        }

        const u32 boot_reason = GetBootReason();
        bq24193::InputCurrentLimit input_current_limit;
        if (R_FAILED(charger_driver.Initialize(boot_reason != 4)) || R_FAILED(charger_driver.GetInputCurrentLimit(&input_current_limit))) {
            pmic_driver.ShutdownSystem();
        }

        if (input_current_limit <= bq24193::InputCurrentLimit_150mA) {
            charger_driver.SetChargerConfiguration(bq24193::ChargerConfiguration_ChargeDisable);
            pmic_driver.ShutdownSystem();
        }

        const BatteryChargeParameters *params = GetBatteryChargeParameters(GetBatteryVersion());
        u32 charge_voltage_limit = params->charge_voltage_limit_default;
        CheckBatteryResult check_result;
        if (boot_reason == 4) {
            if (R_FAILED(charger_driver.GetChargeVoltageLimit(&charge_voltage_limit))) {
                pmic_driver.ShutdownSystem();
            }
            UpdateCharger(&pmic_driver, &charger_driver, &battery_driver, params, charge_voltage_limit);
            check_result = LoopCheckBattery(&pmic_driver, &charger_driver, &battery_driver, params, charge_voltage_limit, true, false, true, false, false);
        } else {
            UpdateCharger(&pmic_driver, &charger_driver, &battery_driver, params, charge_voltage_limit);
            if (boot_reason == 1) {
                check_result = LoopCheckBattery(&pmic_driver, &charger_driver, &battery_driver, params, charge_voltage_limit, true, true, false, true, true);
            } else {
                check_result = LoopCheckBattery(&pmic_driver, &charger_driver, &battery_driver, params, charge_voltage_limit, false, true, false, true, false);
            }
        }

        switch (check_result)   {
            case CheckBatteryResult::Success:
                break;
            case CheckBatteryResult::Shutdown:
                pmic_driver.ShutdownSystem();
                break;
            case CheckBatteryResult::Reboot:
                RebootSystem();
                break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

}
