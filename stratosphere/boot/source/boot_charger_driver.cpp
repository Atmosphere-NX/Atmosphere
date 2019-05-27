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

#include <switch.h>
#include <stratosphere.hpp>
#include "boot_charger_driver.hpp"

Result ChargerDriver::Read(u8 addr, u8 *out) {
    return Boot::ReadI2cRegister(this->i2c_session, reinterpret_cast<u8 *>(out), sizeof(*out), &addr, sizeof(addr));
}

Result ChargerDriver::Write(u8 addr, u8 val) {
    return Boot::WriteI2cRegister(this->i2c_session, reinterpret_cast<u8 *>(&val), sizeof(val), &addr, sizeof(addr));
}

Result ChargerDriver::ReadWrite(u8 addr, u8 mask, u8 val) {
    Result rc;
    u8 cur_val;
    if (R_FAILED((rc = this->Read(addr, &cur_val)))) {
        return rc;
    }

    const u8 new_val = (cur_val & ~mask) | val;
    if (R_FAILED((rc = this->Write(addr, new_val)))) {
        return rc;
    }
    return ResultSuccess;
}

Result ChargerDriver::Initialize() {
    return this->Initialize(true);
}

Result ChargerDriver::Initialize(bool set_input_current_limit) {
    Result rc;
    if (set_input_current_limit) {
        if (R_FAILED((rc = this->SetInputCurrentLimit(InputCurrentLimit_500mA)))) {
            return rc;
        }
    }

    if (R_FAILED((rc = this->SetChargeVoltageLimit(4208)))) {
        return rc;
    }

    if (R_FAILED((rc = this->SetFastChargeCurrentLimit(512)))) {
        return rc;
    }

    if (R_FAILED((rc = this->SetForce20PercentChargeCurrent(false)))) {
        return rc;
    }

    if (R_FAILED((rc = this->SetPreChargeCurrentLimit(128)))) {
        return rc;
    }

    if (R_FAILED((rc = this->SetTerminationCurrentLimit(128)))) {
        return rc;
    }

    if (R_FAILED((rc = this->SetMinimumSystemVoltageLimit(3000)))) {
        return rc;
    }

    if (R_FAILED((rc = this->SetWatchdogTimerSetting(WatchdogTimerSetting_Disabled)))) {
        return rc;
    }

    if (R_FAILED((rc = this->SetChargingSafetyTimerEnabled(false)))) {
        return rc;
    }

    if (R_FAILED((rc = this->ResetWatchdogTimer()))) {
        return rc;
    }

    if (R_FAILED((rc = this->SetBoostModeCurrentLimit(BoostModeCurrentLimit_500mA)))) {
        return rc;
    }

    if (R_FAILED((rc = this->SetHiZEnabled(false)))) {
        return rc;
    }

    return ResultSuccess;
}

Result ChargerDriver::SetChargeEnabled(bool enabled) {
    Boot::GpioSetValue(GpioPadName_Bq24193Charger, enabled ? GpioValue_Low : GpioValue_High);
    return this->SetChargerConfiguration(ChargerConfiguration_ChargeBattery);
}

Result ChargerDriver::SetChargerConfiguration(ChargerConfiguration config) {
    return this->ReadWrite(Bq24193PowerOnConfiguration, 0x30, config);
}

Result ChargerDriver::SetChargeVoltageLimit(u32 voltage) {
    return this->ReadWrite(Bq24193ChargeVoltageControl, 0xFC, EncodeChargeVoltageLimit(voltage));
}

Result ChargerDriver::SetFastChargeCurrentLimit(u32 current) {
    return this->ReadWrite(Bq24193ChargeCurrentControl, 0xFC, EncodeFastChargeCurrentLimit(current));
}

Result ChargerDriver::SetInputCurrentLimit(InputCurrentLimit current) {
    return this->ReadWrite(Bq24193InputSourceControl, 0x07, current);
}

Result ChargerDriver::SetForce20PercentChargeCurrent(bool force) {
    return this->ReadWrite(Bq24193ChargeCurrentControl, 0x01, force ? 1 : 0);
}

Result ChargerDriver::SetPreChargeCurrentLimit(u32 current) {
    return this->ReadWrite(Bq24193PreChargeTerminationCurrentControl, 0xF0, EncodePreChargeCurrentLimit(current));
}

Result ChargerDriver::SetTerminationCurrentLimit(u32 current) {
    return this->ReadWrite(Bq24193PreChargeTerminationCurrentControl, 0x0F, EncodeTerminationCurrentLimit(current));
}

Result ChargerDriver::SetMinimumSystemVoltageLimit(u32 voltage) {
    return this->ReadWrite(Bq24193PowerOnConfiguration, 0x0E, EncodeMinimumSystemVoltageLimit(voltage));
}

Result ChargerDriver::SetWatchdogTimerSetting(WatchdogTimerSetting setting) {
    return this->ReadWrite(Bq24193ChargeTerminationTimerControl, 0x30, setting);
}

Result ChargerDriver::SetChargingSafetyTimerEnabled(bool enabled) {
    return this->ReadWrite(Bq24193ChargeTerminationTimerControl, 0x08, enabled ? 0x08 : 0);
}

Result ChargerDriver::ResetWatchdogTimer() {
    return this->ReadWrite(Bq24193PowerOnConfiguration, 0x40, 0x40);
}

Result ChargerDriver::SetBoostModeCurrentLimit(BoostModeCurrentLimit current) {
    return this->ReadWrite(Bq24193PowerOnConfiguration, 0x01, current);
}

Result ChargerDriver::SetHiZEnabled(bool enabled) {
    return this->ReadWrite(Bq24193InputSourceControl, 0x80, enabled ? 0x80 : 0);
}

Result ChargerDriver::GetInputCurrentLimit(InputCurrentLimit *out) {
    u8 limit;
    Result rc = this->Read(Bq24193InputSourceControl, &limit);
    if (R_FAILED(rc)) {
        return rc;
    }
    *out = static_cast<InputCurrentLimit>(limit);
    return ResultSuccess;
}

Result ChargerDriver::GetChargeVoltageLimit(u32 *out) {
    u8 reg;
    Result rc = this->Read(Bq24193ChargeVoltageControl, &reg);
    if (R_FAILED(rc)) {
        return rc;
    }
    *out = DecodeChargeVoltageLimit(reg);
    return ResultSuccess;
}
