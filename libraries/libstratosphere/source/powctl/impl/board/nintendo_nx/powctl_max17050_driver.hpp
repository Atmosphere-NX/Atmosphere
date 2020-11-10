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

    namespace max17050 {

    }

    class Max17050Driver {
        private:
            os::SdkMutex mutex;
            int init_count;
            i2c::I2cSession i2c_session;
        private:
            Result InitializeSession(const char *battery_vendor, u8 battery_version);
            Result SetMaximumShutdownTimerThreshold();

            bool IsPowerOnReset();
            Result LockVfSoc();
            Result UnlockVfSoc();
            Result LockModelTable();
            Result UnlockModelTable();
            bool IsModelTableLocked();
            Result SetModelTable(const u16 *model_table);
            bool IsModelTableSet(const u16 *model_table);
        public:
            Max17050Driver() : mutex(), init_count(0), i2c_session() {
                /* ... */
            }

            void Initialize(const char *battery_vendor, u8 battery_version) {
                std::scoped_lock lk(this->mutex);
                if ((this->init_count++) == 0) {
                    /* Initialize i2c library. */
                    i2c::InitializeEmpty();

                    /* Open session. */
                    R_ABORT_UNLESS(i2c::OpenSession(std::addressof(this->i2c_session), i2c::DeviceCode_Max17050));

                    /* Initialize session. */
                    R_ABORT_UNLESS(this->InitializeSession(battery_vendor, battery_version));

                    /* Set shutdown timer threshold to the maximum value. */
                    R_ABORT_UNLESS(this->SetMaximumShutdownTimerThreshold());
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

            Result GetNeedToRestoreParameters(bool *out);
            Result SetNeedToRestoreParameters(bool en);
    };

}
