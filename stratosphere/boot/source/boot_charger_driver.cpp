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
#include "boot_charger_driver.hpp"

namespace ams::boot {

    Result ChargerDriver::Read(u8 addr, u8 *out) {
        return ReadI2cRegister(this->i2c_session, reinterpret_cast<u8 *>(out), sizeof(*out), &addr, sizeof(addr));
    }

    Result ChargerDriver::Write(u8 addr, u8 val) {
        return WriteI2cRegister(this->i2c_session, reinterpret_cast<u8 *>(&val), sizeof(val), &addr, sizeof(addr));
    }

    Result ChargerDriver::ReadWrite(u8 addr, u8 mask, u8 val) {
        u8 cur_val;
        R_TRY(this->Read(addr, &cur_val));

        const u8 new_val = (cur_val & ~mask) | val;
        R_TRY(this->Write(addr, new_val));
        return ResultSuccess();
    }

    Result ChargerDriver::Initialize() {
        return this->Initialize(true);
    }

    Result ChargerDriver::Initialize(bool set_input_current_limit) {
        if (set_input_current_limit) {
            R_TRY(this->SetInputCurrentLimit(bq24193::InputCurrentLimit_500mA));
        }

        R_TRY(this->SetChargeVoltageLimit(4208));
        R_TRY(this->SetFastChargeCurrentLimit(512));
        R_TRY(this->SetForce20PercentChargeCurrent(false));
        R_TRY(this->SetPreChargeCurrentLimit(128));
        R_TRY(this->SetTerminationCurrentLimit(128));
        R_TRY(this->SetMinimumSystemVoltageLimit(3000));
        R_TRY(this->SetWatchdogTimerSetting(bq24193::WatchdogTimerSetting_Disabled));
        R_TRY(this->SetChargingSafetyTimerEnabled(false));
        R_TRY(this->ResetWatchdogTimer());
        R_TRY(this->SetBoostModeCurrentLimit(bq24193::BoostModeCurrentLimit_500mA));
        R_TRY(this->SetHiZEnabled(false));

        return ResultSuccess();
    }

    Result ChargerDriver::SetChargeEnabled(bool enabled) {
        gpio::SetValue(GpioPadName_Bq24193Charger, enabled ? GpioValue_Low : GpioValue_High);
        return this->SetChargerConfiguration(bq24193::ChargerConfiguration_ChargeBattery);
    }

    Result ChargerDriver::SetChargerConfiguration(bq24193::ChargerConfiguration config) {
        return this->ReadWrite(bq24193::PowerOnConfiguration, 0x30, config);
    }

    Result ChargerDriver::SetChargeVoltageLimit(u32 voltage) {
        return this->ReadWrite(bq24193::ChargeVoltageControl, 0xFC, bq24193::EncodeChargeVoltageLimit(voltage));
    }

    Result ChargerDriver::SetFastChargeCurrentLimit(u32 current) {
        return this->ReadWrite(bq24193::ChargeCurrentControl, 0xFC, bq24193::EncodeFastChargeCurrentLimit(current));
    }

    Result ChargerDriver::SetInputCurrentLimit(bq24193::InputCurrentLimit current) {
        return this->ReadWrite(bq24193::InputSourceControl, 0x07, current);
    }

    Result ChargerDriver::SetForce20PercentChargeCurrent(bool force) {
        return this->ReadWrite(bq24193::ChargeCurrentControl, 0x01, force ? 1 : 0);
    }

    Result ChargerDriver::SetPreChargeCurrentLimit(u32 current) {
        return this->ReadWrite(bq24193::PreChargeTerminationCurrentControl, 0xF0, bq24193::EncodePreChargeCurrentLimit(current));
    }

    Result ChargerDriver::SetTerminationCurrentLimit(u32 current) {
        return this->ReadWrite(bq24193::PreChargeTerminationCurrentControl, 0x0F, bq24193::EncodeTerminationCurrentLimit(current));
    }

    Result ChargerDriver::SetMinimumSystemVoltageLimit(u32 voltage) {
        return this->ReadWrite(bq24193::PowerOnConfiguration, 0x0E, bq24193::EncodeMinimumSystemVoltageLimit(voltage));
    }

    Result ChargerDriver::SetWatchdogTimerSetting(bq24193::WatchdogTimerSetting setting) {
        return this->ReadWrite(bq24193::ChargeTerminationTimerControl, 0x30, setting);
    }

    Result ChargerDriver::SetChargingSafetyTimerEnabled(bool enabled) {
        return this->ReadWrite(bq24193::ChargeTerminationTimerControl, 0x08, enabled ? 0x08 : 0);
    }

    Result ChargerDriver::ResetWatchdogTimer() {
        return this->ReadWrite(bq24193::PowerOnConfiguration, 0x40, 0x40);
    }

    Result ChargerDriver::SetBoostModeCurrentLimit(bq24193::BoostModeCurrentLimit current) {
        return this->ReadWrite(bq24193::PowerOnConfiguration, 0x01, current);
    }

    Result ChargerDriver::SetHiZEnabled(bool enabled) {
        return this->ReadWrite(bq24193::InputSourceControl, 0x80, enabled ? 0x80 : 0);
    }

    Result ChargerDriver::GetInputCurrentLimit(bq24193::InputCurrentLimit *out) {
        u8 limit;
        R_TRY(this->Read(bq24193::InputSourceControl, &limit));
        *out = static_cast<bq24193::InputCurrentLimit>(limit);
        return ResultSuccess();
    }

    Result ChargerDriver::GetChargeVoltageLimit(u32 *out) {
        u8 reg;
        R_TRY(this->Read(bq24193::ChargeVoltageControl, &reg));
        *out = bq24193::DecodeChargeVoltageLimit(reg);
        return ResultSuccess();
    }

}