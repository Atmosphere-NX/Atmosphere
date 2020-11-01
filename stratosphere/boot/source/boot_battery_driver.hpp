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

namespace ams::boot {

    class BatteryDriver {
        private:
            i2c::driver::I2cSession i2c_session;
        public:
            BatteryDriver() {
                R_ABORT_UNLESS(i2c::driver::OpenSession(std::addressof(this->i2c_session), i2c::DeviceCode_Max17050));
            }

            ~BatteryDriver() {
                i2c::driver::CloseSession(this->i2c_session);
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
