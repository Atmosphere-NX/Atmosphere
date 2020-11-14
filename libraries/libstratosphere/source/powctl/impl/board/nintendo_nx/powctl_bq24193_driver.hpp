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

namespace ams::powctl::impl::board::nintendo_nx {

    namespace bq24193 {

        enum ChargerConfiguration {
            ChargerConfiguration_ChargeDisable = 0,
            ChargerConfiguration_ChargeBattery = 1,
            ChargerConfiguration_Otg           = 2,
        };

        enum ChargerStatus {
            ChargerStatus_NotCharging           = 0,
            ChargerStatus_PreCharge             = 1,
            ChargerStatus_FastCharging          = 2,
            ChargerStatus_ChargeTerminationDone = 3,
        };

    }

    class Bq24193Driver {
        private:
            os::SdkMutex mutex;
            int init_count;
            i2c::I2cSession i2c_session;
        private:
            Result InitializeSession();
        public:
            Bq24193Driver() : mutex(), init_count(0), i2c_session() {
                /* ... */
            }

            void Initialize() {
                std::scoped_lock lk(this->mutex);
                if ((this->init_count++) == 0) {
                    /* Initialize i2c library. */
                    i2c::InitializeEmpty();

                    /* Open session. */
                    R_ABORT_UNLESS(i2c::OpenSession(std::addressof(this->i2c_session), i2c::DeviceCode_Bq24193));

                    /* Initialize session. */
                    R_ABORT_UNLESS(this->InitializeSession());
                }
            }

            void Finalize() {
                std::scoped_lock lk(this->mutex);
                if ((--this->init_count) == 0) {
                    /* Close session. */
                    i2c::CloseSession(this->i2c_session);

                    /* Finalize i2c library. */
                    i2c::Finalize();
                }
            }

            Result SetPreChargeCurrentLimit(int ma);
            Result SetTerminationCurrentLimit(int ma);

            Result SetMinimumSystemVoltageLimit(int mv);

            Result SetChargingSafetyTimerEnabled(bool en);

            Result GetForce20PercentChargeCurrent(bool *out);
            Result SetForce20PercentChargeCurrent(bool en);

            Result GetFastChargeCurrentLimit(int *out_ma);
            Result SetFastChargeCurrentLimit(int ma);

            Result GetChargeVoltageLimit(int *out_mv);
            Result SetChargeVoltageLimit(int mv);

            Result SetChargerConfiguration(bq24193::ChargerConfiguration cfg);

            Result IsHiZEnabled(bool *out);
            Result SetHiZEnabled(bool en);

            Result GetInputCurrentLimit(int *out_ma);
            Result SetInputCurrentLimit(int ma);

            Result SetInputVoltageLimit(int mv);

            Result SetBoostModeCurrentLimit(int ma);

            Result GetChargerStatus(bq24193::ChargerStatus *out);

            Result ResetWatchdogTimer();
            Result SetWatchdogTimerSetting(int seconds);

            Result GetBatteryCompensation(int *out_mo);
            Result SetBatteryCompensation(int mo);

            Result GetVoltageClamp(int *out_mv);
            Result SetVoltageClamp(int mv);
    };

}
