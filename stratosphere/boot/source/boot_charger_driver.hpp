/*
 * Copyright (c) Atmosphère-NX
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
            powctl::Session m_charger_session;
        public:
            ChargerDriver() : m_charger_session() {
                R_ABORT_UNLESS(powctl::OpenSession(std::addressof(m_charger_session), powctl::DeviceCode_Bq24193, ddsf::AccessMode_ReadWrite));
            }

            ~ChargerDriver() {
                powctl::CloseSession(m_charger_session);
            }

            Result Initialize(bool set_input_current_limit) {
                /* Configure PINMUX_AUX_CAM_FLASH_EN as tristate + passthrough. */
                {
                    const uintptr_t apb_regs = dd::QueryIoMapping(0x70000000ul, os::MemoryPageSize);
                    reg::ClearBits(apb_regs + PINMUX_AUX_CAM_FLASH_EN,  reg::EncodeMask(PINMUX_REG_BITS_MASK(AUX_TRISTATE)));
                }

                /* Set input current limit to 500 ma. */
                if (set_input_current_limit) {
                    R_TRY(powctl::SetChargerInputCurrentLimit(m_charger_session, 500));
                }

                /* Set input voltage limit to 500 mv. */
                R_TRY(powctl::SetChargerInputVoltageLimit(m_charger_session, 500));

                /* Disable hi-z mode. */
                R_TRY(powctl::SetChargerHiZEnabled(m_charger_session, false));

                /* Set configuration to charge battery. */
                R_TRY(powctl::SetChargerChargerConfiguration(m_charger_session, powctl::ChargerConfiguration_ChargeBattery));

                return ResultSuccess();
            }

            Result GetChargeCurrentState(powctl::ChargeCurrentState *out) {
                return powctl::GetChargerChargeCurrentState(out, m_charger_session);
            }

            Result SetChargeCurrentState(powctl::ChargeCurrentState state) {
                return powctl::SetChargerChargeCurrentState(m_charger_session, state);
            }

            Result GetInputCurrentLimit(int *out) {
                return powctl::GetChargerInputCurrentLimit(out, m_charger_session);
            }

            Result SetChargerConfiguration(powctl::ChargerConfiguration cfg) {
                return powctl::SetChargerChargerConfiguration(m_charger_session, cfg);
            }

            Result GetFastChargeCurrentLimit(int *out) {
                return powctl::GetChargerFastChargeCurrentLimit(out, m_charger_session);
            }

            Result SetFastChargeCurrentLimit(int limit) {
                return powctl::SetChargerFastChargeCurrentLimit(m_charger_session, limit);
            }

            Result GetChargeVoltageLimit(int *out) {
                return powctl::GetChargerChargeVoltageLimit(out, m_charger_session);
            }

            Result SetChargeVoltageLimit(int limit) {
                return powctl::SetChargerChargeVoltageLimit(m_charger_session, limit);
            }

            Result GetChargerStatus(boot::ChargerStatus *out) {
                /* Default to unknown. */
                *out = ChargerStatus_Unknown;

                /* Get the powctl status. */
                powctl::ChargerStatus powctl_status;
                R_TRY(powctl::GetChargerChargerStatus(std::addressof(powctl_status), m_charger_session));

                switch (powctl_status) {
                    case powctl::ChargerStatus_Charging:              *out = boot::ChargerStatus_Charging;              break;
                    case powctl::ChargerStatus_NotCharging:           *out = boot::ChargerStatus_NotCharging;           break;
                    case powctl::ChargerStatus_ChargeTerminationDone: *out = boot::ChargerStatus_ChargeTerminationDone; break;
                }

                return ResultSuccess();
            }

            Result GetBatteryCompensation(int *out) {
                return powctl::GetChargerBatteryCompensation(out, m_charger_session);
            }

            Result SetBatteryCompensation(int v) {
                return powctl::SetChargerBatteryCompensation(m_charger_session, v);
            }

            Result GetVoltageClamp(int *out) {
                return powctl::GetChargerVoltageClamp(out, m_charger_session);
            }

            Result SetVoltageClamp(int v) {
                return powctl::SetChargerVoltageClamp(m_charger_session, v);
            }
    };

}
