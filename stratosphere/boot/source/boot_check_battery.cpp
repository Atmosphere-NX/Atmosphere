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

#include "boot_functions.hpp"
#include "boot_pmic_driver.hpp"
#include "boot_battery_driver.hpp"
#include "boot_charger_driver.hpp"

enum CheckBatteryResult {
    CheckBatteryResult_Success  = 0,
    CheckBatteryResult_Shutdown = 1,
    CheckBatteryResult_Reboot   = 2,
};

void Boot::CheckBatteryCharge() {
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

    const u32 boot_reason = Boot::GetBootReason();
    InputCurrentLimit input_current_limit;
    if (R_FAILED(charger_driver.Initialize(boot_reason != 4)) || R_FAILED(charger_driver.GetInputCurrentLimit(&input_current_limit))) {
        pmic_driver.ShutdownSystem();
    }

    if (input_current_limit <= InputCurrentLimit_150mA) {
        charger_driver.SetChargerConfiguration(ChargerConfiguration_ChargeDisable);
        pmic_driver.ShutdownSystem();
    }

    const u32 battery_version = Boot::GetBatteryVersion();

    /* TODO: UpdateCharger(); */
    /* TODO: LoopCheckBattery(); */
    CheckBatteryResult check_result = CheckBatteryResult_Success;

    switch (check_result)   {
        case CheckBatteryResult_Success:
            break;
        case CheckBatteryResult_Shutdown:
            pmic_driver.ShutdownSystem();
            break;
        case CheckBatteryResult_Reboot:
            pmic_driver.RebootSystem();
            break;
        default:
            std::abort();
    }
}
