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
#include "i2c/driver/i2c_api.hpp"

namespace ams::boot {

    class BatteryDriver {
        private:
            i2c::driver::Session i2c_session;
        public:
            BatteryDriver() {
                i2c::driver::Initialize();
                i2c::driver::OpenSession(&this->i2c_session, I2cDevice_Max17050);
            }

            ~BatteryDriver() {
                i2c::driver::CloseSession(this->i2c_session);
                i2c::driver::Finalize();
            }
        private:
            Result Read(u8 addr, u16 *out_data);
            Result Write(u8 addr, u16 val);
            Result ReadWrite(u8 addr, u16 mask, u16 val);
            bool WriteValidate(u8 addr, u16 val);

            bool IsPowerOnReset();
            Result LockVfSoc();
            Result UnlockVfSoc();
            Result LockModelTable();
            Result UnlockModelTable();
            bool IsModelTableLocked();
            Result SetModelTable(const u16 *model_table);
            bool IsModelTableSet(const u16 *model_table);

        public:
            Result InitializeBatteryParameters();
            Result IsBatteryRemoved(bool *out);
            Result GetTemperature(double *out);
            Result GetAverageVCell(u32 *out);
            Result GetSocRep(double *out);
            Result GetBatteryPercentage(size_t *out);
            Result SetShutdownTimer();
            Result GetShutdownEnabled(bool *out);
            Result SetShutdownEnabled(bool enabled);
    };

}
