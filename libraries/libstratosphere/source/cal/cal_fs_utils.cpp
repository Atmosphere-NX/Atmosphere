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
#include "cal_fs_utils.hpp"

namespace ams::cal::impl {

    Result ReadCalibrationBlock(s64 offset, void *dst, size_t block_size) {
        /* Open the calibration binary partition. */
        std::unique_ptr<fs::IStorage> storage;
        R_TRY(fs::OpenBisPartition(std::addressof(storage), fs::BisPartitionId::CalibrationBinary));

        /* Read data from the partition. */
        R_TRY(storage->Read(offset, dst, block_size));

        /* Validate the crc. */
        R_TRY(ValidateCalibrationCrc(dst, block_size));

        return ResultSuccess();
    }

}
