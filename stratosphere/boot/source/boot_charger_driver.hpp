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
#include <stratosphere.hpp>

#include "i2c_driver/i2c_api.hpp"
#include "boot_functions.hpp"
#include "boot_bq24193_charger.hpp"

class ChargerDriver {
    private:
        static constexpr u32 GpioPadName_Bq24193Charger = 0xA;
    private:
        I2cSessionImpl i2c_session;
    public:
        ChargerDriver() {
            I2cDriver::Initialize();
            I2cDriver::OpenSession(&this->i2c_session, I2cDevice_Bq24193);

            Boot::GpioConfigure(GpioPadName_Bq24193Charger);
            Boot::GpioSetDirection(GpioPadName_Bq24193Charger, GpioDirection_Output);
        }

        ~ChargerDriver() {
            I2cDriver::CloseSession(this->i2c_session);
            I2cDriver::Finalize();
        }
    private:
        Result Read(u8 addr, u8 *out_data);
        Result Write(u8 addr, u8 val);
        Result ReadWrite(u8 addr, u8 mask, u8 val);

        Result SetInputCurrentLimit(InputCurrentLimit current);
        Result SetForce20PercentChargeCurrent(bool force);
        Result SetPreChargeCurrentLimit(u32 current);
        Result SetTerminationCurrentLimit(u32 current);
        Result SetMinimumSystemVoltageLimit(u32 voltage);
        Result SetWatchdogTimerSetting(WatchdogTimerSetting setting);
        Result SetChargingSafetyTimerEnabled(bool enabled);
        Result ResetWatchdogTimer();
        Result SetBoostModeCurrentLimit(BoostModeCurrentLimit current);
        Result SetHiZEnabled(bool enabled);

    public:
        Result Initialize();
        Result Initialize(bool set_input_current_limit);
        Result SetChargeVoltageLimit(u32 voltage);
        Result SetFastChargeCurrentLimit(u32 current);
        Result SetChargeEnabled(bool enabled);
        Result SetChargerConfiguration(ChargerConfiguration config);
        Result GetInputCurrentLimit(InputCurrentLimit *out);
        Result GetChargeVoltageLimit(u32 *out);
};
