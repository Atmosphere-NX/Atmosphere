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
#include "boot_battery_driver.hpp"

namespace ams::boot {

    /* Driver object. */
    class PmicDriver {
        private:
            i2c::driver::I2cSession i2c_session;
        public:
            PmicDriver() {
                R_ABORT_UNLESS(i2c::driver::OpenSession(std::addressof(this->i2c_session), i2c::DeviceCode_Max77620Pmic));
            }

            ~PmicDriver() {
                i2c::driver::CloseSession(this->i2c_session);
            }
        private:
            Result GetPowerStatus(u8 *out);
            void ShutdownSystem(bool reboot);
            void FinalizeBattery(BatteryDriver &battery_driver);
        public:
            void ShutdownSystem();
            void RebootSystem();
            Result GetAcOk(bool *out);
            Result GetOnOffIrq(u8 *out);
            Result GetNvErc(u8 *out);
            Result GetPowerButtonPressed(bool *out);
    };

}

