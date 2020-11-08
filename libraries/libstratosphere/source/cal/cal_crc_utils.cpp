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
#include <stratosphere.hpp>
#include "cal_crc_utils.hpp"

namespace ams::cal::impl {

    namespace {

        constexpr inline const u16 CrcTable[0x10] = {
            0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401,
            0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400
        };

    }

    u16 CalculateCrc16(const void *data, size_t size) {
        AMS_ASSERT(data != nullptr);

        u16 crc = 0x55AA;
        const u8 *data_u8 = static_cast<const u8 *>(data);
        for (size_t i = 0; i < size; ++i) {
            crc = (crc >> 4) ^ (CrcTable[crc & 0xF]) ^ (CrcTable[(data_u8[i] >> 0) & 0xF]);
            crc = (crc >> 4) ^ (CrcTable[crc & 0xF]) ^ (CrcTable[(data_u8[i] >> 4) & 0xF]);
        }

        return crc;
    }

    Result ValidateCalibrationCrc(const void *data, size_t size) {
        AMS_ASSERT(data != nullptr);
        AMS_ASSERT(size >= sizeof(u16));

        const u16 crc = *reinterpret_cast<const u16 *>(reinterpret_cast<uintptr_t>(data) + size - sizeof(u16));
        R_UNLESS(CalculateCrc16(data, size - sizeof(u16)) == crc, cal::ResultCalibrationDataCrcError());

        return ResultSuccess();
    }

}
