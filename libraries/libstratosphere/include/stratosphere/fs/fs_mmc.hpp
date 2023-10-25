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

    constexpr inline size_t MmcCidSize = 0x10;

    constexpr inline size_t MmcExtendedCsdSize = 0x200;

    constexpr inline int MmcExtendedCsdOffsetReEolInfo             = 267;
    constexpr inline int MmcExtendedCsdOffsetDeviceLifeTimeEstTypA = 268;
    constexpr inline int MmcExtendedCsdOffsetDeviceLifeTimeEstTypB = 269;

    enum MmcSpeedMode {
        MmcSpeedMode_Identification = 0,
        MmcSpeedMode_LegacySpeed    = 1,
        MmcSpeedMode_HighSpeed      = 2,
        MmcSpeedMode_Hs200          = 3,
        MmcSpeedMode_Hs400          = 4,
        MmcSpeedMode_Unknown        = 5,
    };

    enum class MmcPartition {
        UserData       = 0,
        BootPartition1 = 1,
        BootPartition2 = 2,
    };

    Result GetMmcCid(void *dst, size_t size);

    inline void ClearMmcCidSerialNumber(u8 *cid) {
        /* Clear the serial number from the cid. */
        std::memset(cid + 1, 0, 4);
    }

    Result GetMmcSpeedMode(MmcSpeedMode *out);

    Result GetMmcPatrolCount(u32 *out);

    Result GetMmcExtendedCsd(void *dst, size_t size);

}
