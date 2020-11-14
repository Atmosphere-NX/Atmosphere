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
#include <stratosphere.hpp>

namespace ams::boot {

    enum ChargerStatus {
        ChargerStatus_Unknown               = 0,
        ChargerStatus_Charging              = 1,
        ChargerStatus_NotCharging           = 2,
        ChargerStatus_ChargeTerminationDone = 3,
    };

    class ChargerDriver {
        private:
            powctl::Session charger_session;
        public:
            ChargerDriver() : charger_session() {
                R_ABORT_UNLESS(powctl::OpenSession(std::addressof(this->charger_session), powctl::DeviceCode_Bq24193, ddsf::AccessMode_ReadWrite));
            }

            ~ChargerDriver() {
                powctl::CloseSession(this->charger_session);
            }

            Result Initialize(bool set_input_current_limit) {
                /* Set input current limit to 500 ma. */
                if (set_input_current_limit) {
                    R_TRY(powctl::SetChargerInputCurrentLimit(this->charger_session, 500));
                }

                /* Set input voltage limit to 500 mv. */
                R_TRY(powctl::SetChargerInputVoltageLimit(this->charger_session, 500));

                /* Disable hi-z mode. */
                R_TRY(powctl::SetChargerHiZEnabled(this->charger_session, false));

                /* Set configuration to charge battery. */
                R_TRY(powctl::SetChargerChargerConfiguration(this->charger_session, powctl::ChargerConfiguration_ChargeBattery));

                return ResultSuccess();
            }

            Result GetChargeCurrentState(powctl::ChargeCurrentState *out) {
                return powctl::GetChargerChargeCurrentState(out, this->charger_session);
            }

            Result SetChargeCurrentState(powctl::ChargeCurrentState state) {
                return powctl::SetChargerChargeCurrentState(this->charger_session, state);
            }

            Result GetInputCurrentLimit(int *out) {
                return powctl::GetChargerInputCurrentLimit(out, this->charger_session);
            }

            Result SetChargerConfiguration(powctl::ChargerConfiguration cfg) {
                return powctl::SetChargerChargerConfiguration(this->charger_session, cfg);
            }

            Result GetFastChargeCurrentLimit(int *out) {
                return powctl::GetChargerFastChargeCurrentLimit(out, this->charger_session);
            }

            Result SetFastChargeCurrentLimit(int limit) {
                return powctl::SetChargerFastChargeCurrentLimit(this->charger_session, limit);
            }

            Result GetChargeVoltageLimit(int *out) {
                return powctl::GetChargerChargeVoltageLimit(out, this->charger_session);
            }

            Result SetChargeVoltageLimit(int limit) {
                return powctl::SetChargerChargeVoltageLimit(this->charger_session, limit);
            }

            Result GetChargerStatus(boot::ChargerStatus *out) {
                /* Default to unknown. */
                *out = ChargerStatus_Unknown;

                /* Get the powctl status. */
                powctl::ChargerStatus powctl_status;
                R_TRY(powctl::GetChargerChargerStatus(std::addressof(powctl_status), this->charger_session));

                switch (powctl_status) {
                    case powctl::ChargerStatus_Charging:              *out = boot::ChargerStatus_Charging;              break;
                    case powctl::ChargerStatus_NotCharging:           *out = boot::ChargerStatus_NotCharging;           break;
                    case powctl::ChargerStatus_ChargeTerminationDone: *out = boot::ChargerStatus_ChargeTerminationDone; break;
                }

                return ResultSuccess();
            }

            Result GetBatteryCompensation(int *out) {
                return powctl::GetChargerBatteryCompensation(out, this->charger_session);
            }

            Result SetBatteryCompensation(int v) {
                return powctl::SetChargerBatteryCompensation(this->charger_session, v);
            }

            Result GetVoltageClamp(int *out) {
                return powctl::GetChargerVoltageClamp(out, this->charger_session);
            }

            Result SetVoltageClamp(int v) {
                return powctl::SetChargerVoltageClamp(this->charger_session, v);
            }
    };

}
