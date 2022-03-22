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

namespace ams::mitm {

    constexpr inline size_t CalibrationBinarySize             = 0x8000;

    constexpr inline s64 SecureCalibrationInfoBackupOffset    = 3_MB;
    constexpr inline size_t SecureCalibrationBinaryBackupSize = 0xC000;

    void InitializeProdInfoManagement();

    void SaveProdInfoBackupsAndWipeMemory(char *out_name, size_t out_name_size);

    bool ShouldReadBlankCalibrationBinary();
    bool IsWriteToCalibrationBinaryAllowed();

    void ReadFromBlankCalibrationBinary(s64 offset, void *dst, size_t size);
    void WriteToBlankCalibrationBinary(s64 offset, const void *src, size_t size);

    void ReadFromFakeSecureBackupStorage(s64 offset, void *dst, size_t size);
    void WriteToFakeSecureBackupStorage(s64 offset, const void *src, size_t size);

}
