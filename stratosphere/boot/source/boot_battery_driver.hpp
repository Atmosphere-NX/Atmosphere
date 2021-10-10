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

    class BatteryDriver {
        private:
            powctl::Session m_battery_session;
        public:
            BatteryDriver() : m_battery_session() {
                R_ABORT_UNLESS(powctl::OpenSession(std::addressof(m_battery_session), powctl::DeviceCode_Max17050, ddsf::AccessMode_ReadWrite));
            }

            ~BatteryDriver() {
                powctl::CloseSession(m_battery_session);
            }
        public:
            Result IsBatteryRemoved(bool *out) {
                bool present;
                R_TRY(powctl::IsBatteryPresent(std::addressof(present), m_battery_session));

                *out = !present;
                return ResultSuccess();
            }

            Result GetSocRep(float *out) {
                return powctl::GetBatterySocRep(out, m_battery_session);
            }

            Result GetAverageVCell(int *out) {
                return powctl::GetBatteryAverageVCell(out, m_battery_session);
            }

            Result GetOpenCircuitVoltage(int *out) {
                return powctl::GetBatteryOpenCircuitVoltage(out, m_battery_session);
            }

            Result GetAverageCurrent(int *out) {
                return powctl::GetBatteryAverageCurrent(out, m_battery_session);
            }

            Result GetCurrent(int *out) {
                return powctl::GetBatteryCurrent(out, m_battery_session);
            }

            Result GetTemperature(float *out) {
                return powctl::GetBatteryTemperature(out, m_battery_session);
            }

            Result IsI2cShutdownEnabled(bool *out) {
                return powctl::IsBatteryI2cShutdownEnabled(out, m_battery_session);
            }

            Result SetI2cShutdownEnabled(bool en) {
                return powctl::SetBatteryI2cShutdownEnabled(m_battery_session, en);
            }
    };

}
