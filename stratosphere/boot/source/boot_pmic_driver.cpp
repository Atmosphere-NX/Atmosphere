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
#include "boot_i2c_utils.hpp"
#include "boot_pmic_driver.hpp"

namespace ams::boot {

    void PmicDriver::ShutdownSystem() {
        R_ABORT_UNLESS(this->ShutdownSystem(false));
    }

    void PmicDriver::RebootSystem() {
        R_ABORT_UNLESS(this->ShutdownSystem(true));
    }

    Result PmicDriver::GetAcOk(bool *out) {
        u8 power_status;
        R_TRY(this->GetPowerStatus(&power_status));
        *out = (power_status & 0x02) != 0;
        return ResultSuccess();
    }

    Result PmicDriver::GetPowerIntr(u8 *out) {
        const u8 addr = 0x0B;
        return ReadI2cRegister(this->i2c_session, out, sizeof(*out), &addr, sizeof(addr));
    }

    Result PmicDriver::GetPowerStatus(u8 *out) {
        const u8 addr = 0x15;
        return ReadI2cRegister(this->i2c_session, out, sizeof(*out), &addr, sizeof(addr));
    }

    Result PmicDriver::GetNvErc(u8 *out) {
        const u8 addr = 0x0C;
        return ReadI2cRegister(this->i2c_session, out, sizeof(*out), &addr, sizeof(addr));
    }

    Result PmicDriver::GetPowerButtonPressed(bool *out) {
        u8 power_intr;
        R_TRY(this->GetPowerIntr(&power_intr));
        *out = (power_intr & 0x08) != 0;
        return ResultSuccess();
    }

    Result PmicDriver::ShutdownSystem(bool reboot) {
        const u8 on_off_1_addr = 0x41;
        const u8 on_off_2_addr = 0x42;

        /* Get value, set or clear software reset mask. */
        u8 on_off_2_val = 0;
        R_ABORT_UNLESS(ReadI2cRegister(this->i2c_session, &on_off_2_val, sizeof(on_off_2_val), &on_off_2_addr, sizeof(on_off_2_addr)));
        if (reboot) {
            on_off_2_val |= 0x80;
        } else {
            on_off_2_val &= ~0x80;
        }
        R_ABORT_UNLESS(WriteI2cRegister(this->i2c_session, &on_off_2_val, sizeof(on_off_2_val), &on_off_2_addr, sizeof(on_off_2_addr)));

        /* Get value, set software reset mask. */
        u8 on_off_1_val = 0;
        R_ABORT_UNLESS(ReadI2cRegister(this->i2c_session, &on_off_1_val, sizeof(on_off_1_val), &on_off_1_addr, sizeof(on_off_1_addr)));
        on_off_1_val |= 0x80;

        /* Finalize the battery on non-Calcio. */
        if (spl::GetHardwareType() != spl::HardwareType::Calcio) {
            BatteryDriver battery_driver;
            this->FinalizeBattery(&battery_driver);
        }

        /* Actually write the value to trigger shutdown/reset. */
        R_ABORT_UNLESS(WriteI2cRegister(this->i2c_session, &on_off_1_val, sizeof(on_off_1_val), &on_off_1_addr, sizeof(on_off_1_addr)));

        /* Allow up to 5 seconds for shutdown/reboot to take place. */
        svcSleepThread(5'000'000'000ul);
        AMS_ABORT_UNLESS(false);
    }

    void PmicDriver::FinalizeBattery(BatteryDriver *battery_driver) {
        /* Set shutdown timer. */
        battery_driver->SetShutdownTimer();

        /* Get whether shutdown is enabled. */
        bool shutdown_enabled;
        if (R_FAILED(battery_driver->GetShutdownEnabled(&shutdown_enabled))) {
            return;
        }

        /* On Hoag, we don't want to use the desired shutdown value when battery charged. */
        bool use_desired_shutdown = true;
        if (spl::GetHardwareType() == spl::HardwareType::Hoag) {
            double battery_charge;
            if (R_FAILED(battery_driver->GetSocRep(&battery_charge)) || battery_charge >= 80.0) {
                use_desired_shutdown = false;
            }
        }

        bool ac_ok;
        bool desired_shutdown_enabled;
        if (R_FAILED(this->GetAcOk(&ac_ok)) || ac_ok) {
            desired_shutdown_enabled = false;
        } else {
            desired_shutdown_enabled = true;
        }

        desired_shutdown_enabled &= use_desired_shutdown;

        if (shutdown_enabled != desired_shutdown_enabled) {
            battery_driver->SetShutdownEnabled(desired_shutdown_enabled);
        }
    }

}
