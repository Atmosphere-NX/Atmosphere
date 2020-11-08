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
#include "cal_fs_utils.hpp"

namespace ams::cal {

    namespace {

        constexpr inline s64 BatteryLotOffset  = 0x2CE0;
        constexpr inline size_t BatteryLotSize = 0x20;

        constexpr inline s64 BatteryVersionOffset  = 0x4310;
        constexpr inline size_t BatteryVersionSize = 0x10;

        constexpr inline size_t BatteryVendorSizeMax = 0x18;

    }

    Result GetBatteryVersion(u8 *out) {
        /* Read the battery version. */
        u8 battery_version[BatteryVersionSize];
        R_TRY(cal::impl::ReadCalibrationBlock(BatteryVersionOffset, battery_version, sizeof(battery_version)));

        /* Write the output. */
        *out = battery_version[0];
        return ResultSuccess();
    }

    Result GetBatteryVendor(size_t *out_vendor_size, void *dst, size_t dst_size) {
        /* Read the battery lot. */
        char battery_lot[BatteryLotSize];
        R_TRY(cal::impl::ReadCalibrationBlock(BatteryLotOffset, battery_lot, sizeof(battery_lot)));

        /* Copy output. */
        *out_vendor_size = static_cast<size_t>(util::Strlcpy(static_cast<char *>(dst), battery_lot, std::min(dst_size, BatteryVendorSizeMax)));
        return ResultSuccess();
    }

}
