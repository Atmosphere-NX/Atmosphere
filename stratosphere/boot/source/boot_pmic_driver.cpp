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
#include <stratosphere.hpp>
#include "boot_i2c_utils.hpp"
#include "boot_pmic_driver.hpp"

namespace ams::boot {

    void PmicDriver::ShutdownSystem() {
        this->ShutdownSystem(false);
    }

    void PmicDriver::RebootSystem() {
        this->ShutdownSystem(true);
    }

    Result PmicDriver::GetAcOk(bool *out) {
        u8 power_status;
        R_TRY(this->GetPowerStatus(std::addressof(power_status)));
        *out = (power_status & 0x02) != 0;
        return ResultSuccess();
    }

    Result PmicDriver::GetOnOffIrq(u8 *out) {
        const u8 addr = 0x0B;
        return ReadI2cRegister(this->i2c_session, out, sizeof(*out), std::addressof(addr), sizeof(addr));
    }

    Result PmicDriver::GetPowerStatus(u8 *out) {
        const u8 addr = 0x15;
        return ReadI2cRegister(this->i2c_session, out, sizeof(*out), std::addressof(addr), sizeof(addr));
    }

    Result PmicDriver::GetNvErc(u8 *out) {
        const u8 addr = 0x0C;
        return ReadI2cRegister(this->i2c_session, out, sizeof(*out), std::addressof(addr), sizeof(addr));
    }

    Result PmicDriver::GetPowerButtonPressed(bool *out) {
        u8 on_off_irq;
        R_TRY(this->GetOnOffIrq(std::addressof(on_off_irq)));
        *out = (on_off_irq & 0x08) != 0;
        return ResultSuccess();
    }

    void PmicDriver::ShutdownSystem(bool reboot) {
        const u8 on_off_1_addr = 0x41;
        const u8 on_off_2_addr = 0x42;

        /* Get value, set or clear software reset mask. */
        u8 on_off_2_val = 0;
        R_ABORT_UNLESS(ReadI2cRegister(this->i2c_session, std::addressof(on_off_2_val), sizeof(on_off_2_val), std::addressof(on_off_2_addr), sizeof(on_off_2_addr)));
        if (reboot) {
            on_off_2_val |= 0x80;
        } else {
            on_off_2_val &= ~0x80;
        }
        R_ABORT_UNLESS(WriteI2cRegister(this->i2c_session, std::addressof(on_off_2_val), sizeof(on_off_2_val), std::addressof(on_off_2_addr), sizeof(on_off_2_addr)));

        /* Get value, set software reset mask. */
        u8 on_off_1_val = 0;
        R_ABORT_UNLESS(ReadI2cRegister(this->i2c_session, std::addressof(on_off_1_val), sizeof(on_off_1_val), std::addressof(on_off_1_addr), sizeof(on_off_1_addr)));
        on_off_1_val |= 0x80;

        /* Finalize the battery on non-Calcio. */
        if (spl::GetHardwareType() != spl::HardwareType::Calcio) {
            BatteryDriver battery_driver;
            this->FinalizeBattery(battery_driver);
        }

        /* Actually write the value to trigger shutdown/reset. */
        R_ABORT_UNLESS(WriteI2cRegister(this->i2c_session, std::addressof(on_off_1_val), sizeof(on_off_1_val), std::addressof(on_off_1_addr), sizeof(on_off_1_addr)));

        /* Allow up to 5 seconds for shutdown/reboot to take place. */
        os::SleepThread(TimeSpan::FromSeconds(5));
        AMS_ABORT("Shutdown failed");
    }

    void PmicDriver::FinalizeBattery(BatteryDriver &battery_driver) {
        /* Get whether shutdown is enabled. */
        bool shutdown_enabled;
        if (R_FAILED(battery_driver.IsI2cShutdownEnabled(std::addressof(shutdown_enabled)))) {
            return;
        }

        /* On Hoag, we don't want to use the desired shutdown value when battery charged. */
        bool use_desired_shutdown = true;
        if (spl::GetHardwareType() == spl::HardwareType::Hoag) {
            float battery_charge_raw;
            if (R_FAILED(battery_driver.GetSocRep(std::addressof(battery_charge_raw))) || battery_charge_raw >= 80.0) {
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
            battery_driver.SetI2cShutdownEnabled(desired_shutdown_enabled);
        }
    }

}
