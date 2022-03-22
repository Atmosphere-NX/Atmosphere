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
        NON_COPYABLE(BatteryDriver);
        NON_MOVEABLE(BatteryDriver);
        private:
            static constinit inline powctl::Session s_battery_session{powctl::Session::ConstantInitializeTag{}};
            static constinit inline int s_reference_count{0};
        public:
            BatteryDriver() {
                if ((s_reference_count++) == 0) {
                    R_ABORT_UNLESS(powctl::OpenSession(std::addressof(s_battery_session), powctl::DeviceCode_Max17050, ddsf::AccessMode_ReadWrite));
                }
            }

            ~BatteryDriver() {
                if ((--s_reference_count) == 0) {
                    powctl::CloseSession(s_battery_session);
                }
            }
        public:
            Result IsBatteryRemoved(bool *out) {
                bool present;
                R_TRY(powctl::IsBatteryPresent(std::addressof(present), s_battery_session));

                *out = !present;
                return ResultSuccess();
            }

            Result GetChargePercentage(float *out) {
                return powctl::GetBatteryChargePercentage(out, s_battery_session);
            }

            Result GetAverageVCell(int *out) {
                return powctl::GetBatteryAverageVCell(out, s_battery_session);
            }

            Result GetVoltageFuelGaugePercentage(float *out) {
                return powctl::GetBatteryVoltageFuelGaugePercentage(out, s_battery_session);
            }

            Result GetAverageCurrent(int *out) {
                return powctl::GetBatteryAverageCurrent(out, s_battery_session);
            }

            Result GetCurrent(int *out) {
                return powctl::GetBatteryCurrent(out, s_battery_session);
            }

            Result GetTemperature(float *out) {
                return powctl::GetBatteryTemperature(out, s_battery_session);
            }

            Result IsI2cShutdownEnabled(bool *out) {
                return powctl::IsBatteryI2cShutdownEnabled(out, s_battery_session);
            }

            Result SetI2cShutdownEnabled(bool en) {
                return powctl::SetBatteryI2cShutdownEnabled(s_battery_session, en);
            }
    };

}
