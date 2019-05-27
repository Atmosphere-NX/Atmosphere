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
#include "boot_battery_driver.hpp"

class PmicDriver {
    private:
        I2cSessionImpl i2c_session;
    public:
        PmicDriver() {
            I2cDriver::Initialize();
            I2cDriver::OpenSession(&this->i2c_session, I2cDevice_Max77620Pmic);
        }

        ~PmicDriver() {
            I2cDriver::CloseSession(this->i2c_session);
            I2cDriver::Finalize();
        }
    private:
        Result GetPowerStatus(u8 *out);
        Result ShutdownSystem(bool reboot);
        void FinalizeBattery(BatteryDriver *battery_driver);
    public:
        void ShutdownSystem();
        void RebootSystem();
        Result GetAcOk(bool *out);
        Result GetPowerIntr(u8 *out);
        Result GetNvErc(u8 *out);
        Result GetPowerButtonPressed(bool *out);
};