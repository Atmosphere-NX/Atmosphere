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
#include "powctl_bq24193_driver.hpp"

namespace ams::powctl::impl::board::nintendo_nx {

    Result Bq24193Driver::InitializeSession() {
        /* Set fast charge current limit. */
        R_TRY(this->SetFastChargeCurrentLimit(512));

        /* Disable force 20 percent charge. */
        R_TRY(this->SetForce20PercentChargeCurrent(false));

        /* Set pre-charge current limit. */
        R_TRY(this->SetPreChargeCurrentLimit(128));

        /* Set termination current limit. */
        R_TRY(this->SetTerminationCurrentLimit(128));

        /* Set minimum system voltage limit. */
        R_TRY(this->SetMinimumSystemVoltageLimit(3000));

        /* Set watchdog timer setting. */
        R_TRY(this->SetWatchdogTimerSetting(0));

        /* Disable charging safety timer. */
        R_TRY(this->SetChargingSafetyTimerEnabled(false));

        /* Reset the watchdog timer. */
        R_TRY(this->ResetWatchdogTimer());

        return ResultSuccess();
    }

    Result Bq24193Driver::SetPreChargeCurrentLimit(int ma) {
        /* TODO */
        AMS_ABORT();
    }

    Result Bq24193Driver::SetTerminationCurrentLimit(int ma) {
        /* TODO */
        AMS_ABORT();
    }

    Result Bq24193Driver::SetMinimumSystemVoltageLimit(int mv) {
        /* TODO */
        AMS_ABORT();
    }

    Result Bq24193Driver::SetChargingSafetyTimerEnabled(bool en) {
        /* TODO */
        AMS_ABORT();
    }

    Result Bq24193Driver::GetForce20PercentChargeCurrent(bool *out) {
        /* TODO */
        AMS_ABORT();
    }

    Result Bq24193Driver::SetForce20PercentChargeCurrent(bool en) {
        /* TODO */
        AMS_ABORT();
    }

    Result Bq24193Driver::GetFastChargeCurrentLimit(int *out_ma) {
        /* TODO */
        AMS_ABORT();
    }

    Result Bq24193Driver::SetFastChargeCurrentLimit(int ma) {
        /* TODO */
        AMS_ABORT();
    }

    Result Bq24193Driver::GetChargeVoltageLimit(int *out_mv) {
        /* TODO */
        AMS_ABORT();
    }

    Result Bq24193Driver::SetChargeVoltageLimit(int mv) {
        /* TODO */
        AMS_ABORT();
    }

    Result Bq24193Driver::SetChargerConfiguration(bq24193::ChargerConfiguration cfg) {
        /* TODO */
        AMS_ABORT();
    }

    Result Bq24193Driver::IsHiZEnabled(bool *out) {
        /* TODO */
        AMS_ABORT();
    }

    Result Bq24193Driver::SetHiZEnabled(bool en) {
        /* TODO */
        AMS_ABORT();
    }

    Result Bq24193Driver::GetInputCurrentLimit(int *out_ma) {
        /* TODO */
        AMS_ABORT();
    }

    Result Bq24193Driver::SetInputCurrentLimit(int ma) {
        /* TODO */
        AMS_ABORT();
    }

    Result Bq24193Driver::GetInputVoltageLimit(int *out_mv) {
        /* TODO */
        AMS_ABORT();
    }

    Result Bq24193Driver::SetInputVoltageLimit(int mv) {
        /* TODO */
        AMS_ABORT();
    }

    Result Bq24193Driver::GetChargerStatus(bq24193::ChargerStatus *out) {
        /* TODO */
        AMS_ABORT();
    }

    Result Bq24193Driver::ResetWatchdogTimer() {
        /* TODO */
        AMS_ABORT();
    }

    Result Bq24193Driver::SetWatchdogTimerSetting(int seconds) {
        /* TODO */
        AMS_ABORT();
    }

    Result Bq24193Driver::GetBatteryCompensation(int *out_mo) {
        /* TODO */
        AMS_ABORT();
    }

    Result Bq24193Driver::SetBatteryCompensation(int mo) {
        /* TODO */
        AMS_ABORT();
    }

    Result Bq24193Driver::GetVoltageClamp(int *out_mv) {
        /* TODO */
        AMS_ABORT();
    }

    Result Bq24193Driver::SetVoltageClamp(int mv) {
        /* TODO */
        AMS_ABORT();
    }

}