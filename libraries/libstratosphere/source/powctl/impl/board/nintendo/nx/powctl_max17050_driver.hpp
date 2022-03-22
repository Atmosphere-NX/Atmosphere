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

namespace ams::powctl::impl::board::nintendo::nx {

    namespace max17050 {

        struct InternalState {
            u16 rcomp0;
            u16 tempco;
            u16 fullcap;
            u16 cycles;
            u16 fullcapnom;
            u16 iavgempty;
            u16 qresidual00;
            u16 qresidual10;
            u16 qresidual20;
            u16 qresidual30;
        };

    }

    class Max17050Driver {
        private:
            os::SdkMutex m_mutex;
            int m_init_count;
            i2c::I2cSession m_i2c_session;
            max17050::InternalState m_internal_state;
        private:
            Result InitializeSession(const char *battery_vendor, u8 battery_version);
            Result SetMaximumShutdownTimerThreshold();

            Result SetAlertByChargePercentage();
            Result SetAlertByVoltageFuelGaugePercentage();

            bool IsPowerOnReset();
            Result LockVoltageFuelGauge();
            Result UnlockVoltageFuelGauge();
            Result LockModelTable();
            Result UnlockModelTable();
            bool IsModelTableLocked();
            Result SetModelTable(const u16 *model_table);
            bool IsModelTableSet(const u16 *model_table);
        public:
            constexpr Max17050Driver() : m_mutex(), m_init_count(0), m_i2c_session(), m_internal_state() {
                /* ... */
            }

            void Initialize(const char *battery_vendor, u8 battery_version) {
                std::scoped_lock lk(m_mutex);
                if ((m_init_count++) == 0) {
                    /* Initialize i2c library. */
                    i2c::InitializeEmpty();

                    /* Open session. */
                    R_ABORT_UNLESS(i2c::OpenSession(std::addressof(m_i2c_session), i2c::DeviceCode_Max17050));

                    /* Initialize session. */
                    R_ABORT_UNLESS(this->InitializeSession(battery_vendor, battery_version));

                    /* Set shutdown timer threshold to the maximum value. */
                    R_ABORT_UNLESS(this->SetMaximumShutdownTimerThreshold());
                }
            }

            void Finalize() {
                std::scoped_lock lk(m_mutex);
                if ((--m_init_count) == 0) {
                    /* Close session. */
                    i2c::CloseSession(m_i2c_session);

                    /* Finalize i2c library. */
                    i2c::Finalize();
                }
            }

            Result ReadInternalState();
            Result WriteInternalState();

            void GetInternalState(max17050::InternalState *dst) {
                *dst = m_internal_state;
            }

            void SetInternalState(const max17050::InternalState &src) {
                m_internal_state = src;
            }

            Result GetChargePercentage(double *out);
            Result GetVoltageFuelGaugePercentage(double *out);

            Result GetFullCapacity(double *out, double sense_resistor);
            Result GetRemainingCapacity(double *out, double sense_resistor);

            Result SetChargePercentageMinimumAlertThreshold(int percentage);
            Result SetChargePercentageMaximumAlertThreshold(int percentage);

            Result SetVoltageFuelGaugePercentageMinimumAlertThreshold(int percentage);
            Result SetVoltageFuelGaugePercentageMaximumAlertThreshold(int percentage);

            Result SetFullChargeThreshold(double percentage);

            Result GetAverageCurrent(double *out, double sense_resistor);
            Result GetCurrent(double *out, double sense_resistor);

            Result GetNeedToRestoreParameters(bool *out);
            Result SetNeedToRestoreParameters(bool en);

            Result IsI2cShutdownEnabled(bool *out);
            Result SetI2cShutdownEnabled(bool en);

            Result GetStatus(u16 *out);

            Result GetCycles(u16 *out);
            Result ResetCycles();

            Result GetAge(double *out);

            Result GetTemperature(double *out);

            Result GetMaximumTemperature(u8 *out);

            Result SetTemperatureMinimumAlertThreshold(int c);
            Result SetTemperatureMaximumAlertThreshold(int c);

            Result GetVCell(int *out);
            Result GetAverageVCell(int *out);
            Result GetAverageVCellTime(double *out);

            Result GetOpenCircuitVoltage(int *out);

            Result SetVoltageMinimumAlertThreshold(int mv);
            Result SetVoltageMaximumAlertThreshold(int mv);
    };

}
