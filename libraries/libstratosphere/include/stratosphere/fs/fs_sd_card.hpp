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
#include <stratosphere/fs/fs_common.hpp>

namespace ams::fs {

    /* ACCURATE_TO_VERSION: Unknown */
    class IEventNotifier;

    constexpr inline size_t SdCardCidSize = 0x10;

    enum SdCardSpeedMode {
        SdCardSpeedMode_Identification = 0,
        SdCardSpeedMode_DefaultSpeed   = 1,
        SdCardSpeedMode_HighSpeed      = 2,
        SdCardSpeedMode_Sdr12          = 3,
        SdCardSpeedMode_Sdr25          = 4,
        SdCardSpeedMode_Sdr50          = 5,
        SdCardSpeedMode_Sdr104         = 6,
        SdCardSpeedMode_Ddr50          = 7,
        SdCardSpeedMode_Unknown        = 8,
    };

    struct EncryptionSeed {
        char value[0x10];
    };
    static_assert(util::is_pod<EncryptionSeed>::value);
    static_assert(sizeof(EncryptionSeed) == 0x10);

    Result GetSdCardCid(void *dst, size_t size);

    inline void ClearSdCardCidSerialNumber(u8 *cid) {
        /* Clear the serial number from the cid. */
        std::memset(cid + 2, 0, 4);
    }

    Result GetSdCardUserAreaSize(s64 *out);
    Result GetSdCardProtectedAreaSize(s64 *out);

    Result GetSdCardSpeedMode(SdCardSpeedMode *out);

    Result MountSdCard(const char *name);

    Result MountSdCardErrorReportDirectoryForAtmosphere(const char *name);

    Result OpenSdCardDetectionEventNotifier(std::unique_ptr<IEventNotifier> *out);

    bool IsSdCardInserted();

}
