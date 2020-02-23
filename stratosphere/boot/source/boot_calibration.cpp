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
#include "boot_calibration.hpp"

namespace ams::boot {

    namespace {

        /* Convenience definitions. */
        constexpr size_t BatteryLotOffset = 0x2CE0;
        constexpr size_t BatteryLotSize = 0x20;
        constexpr size_t BatteryVersionOffset = 0x4310;
        constexpr size_t BatteryVersionSize = 0x10;

        constexpr u32 DefaultBatteryVendor = static_cast<u32>('A');
        constexpr u32 DefaultBatteryVersion = 0;

        /* Helpers. */
        constexpr u16 GetCrc16(const void *data, size_t size) {
            constexpr u16 s_crc_table[0x10] = {
                0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401,
                0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400
            };

            AMS_ABORT_UNLESS(data != nullptr);

            u16 crc16 = 0x55AA;
            const u8 *data_u8 = reinterpret_cast<const u8 *>(data);
            for (size_t i = 0; i < size; i++) {
                crc16 = (crc16 >> 4) ^ (s_crc_table[crc16 & 0xF]) ^ (s_crc_table[data_u8[i] & 0xF]);
                crc16 = (crc16 >> 4) ^ (s_crc_table[crc16 & 0xF]) ^ (s_crc_table[(data_u8[i] >> 4) & 0xF]);
            }
            return crc16;
        }

        Result ValidateCalibrationCrc16(const void *data, size_t size) {
            const u8 *data_u8 = reinterpret_cast<const u8 *>(data);
            const bool crc_valid = GetCrc16(data, size - sizeof(u16)) == *(reinterpret_cast<const u16 *>(&data_u8[size - sizeof(u16)]));
            R_UNLESS(crc_valid, cal::ResultCalibrationDataCrcError());
            return ResultSuccess();
        }

        Result GetBatteryVendorImpl(u32 *vendor) {
            FsStorage s;
            R_TRY(fsOpenBisStorage(&s, FsBisPartitionId_CalibrationBinary));
            ON_SCOPE_EXIT { fsStorageClose(&s); };

            u8 battery_lot[BatteryLotSize];
            R_TRY(fsStorageRead(&s, BatteryLotOffset, battery_lot, sizeof(battery_lot)));

            R_TRY(ValidateCalibrationCrc16(battery_lot, sizeof(battery_lot)));

            *vendor = battery_lot[7];
            return ResultSuccess();
        }

        Result GetBatteryVersionImpl(u32 *version) {
            FsStorage s;
            R_TRY(fsOpenBisStorage(&s, FsBisPartitionId_CalibrationBinary));
            ON_SCOPE_EXIT { fsStorageClose(&s); };

            u8 battery_version[BatteryVersionSize];
            R_TRY(fsStorageRead(&s, BatteryVersionOffset, battery_version, sizeof(battery_version)));

            R_TRY(ValidateCalibrationCrc16(battery_version, sizeof(battery_version)));

            *version = battery_version[0];
            return ResultSuccess();
        }

    }

    u32 GetBatteryVendor() {
        u32 vendor;
        if (R_FAILED(GetBatteryVendorImpl(&vendor))) {
            return DefaultBatteryVendor;
        }
        return vendor;
    }

    u32 GetBatteryVersion() {
        u32 version;
        if (R_FAILED(GetBatteryVersionImpl(&version))) {
            return DefaultBatteryVersion;
        }
        return version;
    }

}
