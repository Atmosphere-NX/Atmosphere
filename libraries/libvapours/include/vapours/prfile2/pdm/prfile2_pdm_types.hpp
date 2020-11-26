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
#include <vapours/prfile2/prfile2_build_config.hpp>
#include <vapours/prfile2/prfile2_handle.hpp>

namespace ams::prfile2::pdm {

    enum Error {
        Error_NoError        =   0,
        Error_Ok             =   Error_NoError,

        Error_InvalidParameter            = 0x0001,
        Error_InvalidMasterBoot           = 0x0002,
        Error_InvalidBootSector           = 0x0003,
        Error_InvalidBpb                  = 0x0004,
        Error_NotExistMbr                 = 0x0005,
        Error_NotExistEpbr                = 0x0006,
        Error_NotExistPartition           = 0x0007,
        Error_NotExistFreeDiskStruct      = 0x0008,
        Error_NotExistPartitionStruct     = 0x0009,
        Error_NotExistFreePartitionStruct = 0x000A,
        Error_StateOpened                 = 0x000B,
        Error_StateClosed                 = 0x000C,
        Error_StateLocked                 = 0x000D,
        Error_StateUnlocked               = 0x000E,
        Error_AccessPermission            = 0x000F,
        Error_WriteProtected              = 0x0010,
        Error_MediaEjected                = 0x0011,
        Error_OutOfRange                  = 0x0012,
        Error_SystemCallError             = 0x0013,
        Error_LockError                   = 0x0014,
        Error_DriverError                 = 0x0015,
        Error_UnsupportDiskFormat         = 0x0016,
    };

    using NonBlockingProtocolType = int;

    using DiskCallback = void (*)();

}
