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

namespace ams::fs::impl {

    /* Delimiting of mount names. */
    constexpr inline const char ReservedMountNamePrefixCharacter                = '@';
    constexpr inline const char * const MountNameDelimiter                      = ":/";

    /* Filesystem names. */
    constexpr inline const char * const HostRootFileSystemMountName             = "@Host";
    constexpr inline const char * const SdCardFileSystemMountName               = "@Sdcard";
    constexpr inline const char * const GameCardFileSystemMountName             = "@Gc";

    constexpr inline size_t GameCardFileSystemMountNameSuffixLength             = 1;

    constexpr inline const char * const GameCardFileSystemMountNameUpdateSuffix = "U";
    constexpr inline const char * const GameCardFileSystemMountNameNormalSuffix = "N";
    constexpr inline const char * const GameCardFileSystemMountNameSecureSuffix = "S";

    /* Built-in storage names. */
    constexpr inline const char * const BisCalibrationFilePartitionMountName    = "@CalibFile";
    constexpr inline const char * const BisSafeModePartitionMountName           = "@Safe";
    constexpr inline const char * const BisUserPartitionMountName               = "@User";
    constexpr inline const char * const BisSystemPartitionMountName             = "@System";

    /* Content storage names. */
    constexpr inline const char * const ContentStorageSystemMountName           = "@SystemContent";
    constexpr inline const char * const ContentStorageUserMountName             = "@UserContent";
    constexpr inline const char * const ContentStorageSdCardMountName           = "@SdCardContent";


    /* Registered update partition. */
    constexpr inline const char * const RegisteredUpdatePartitionMountName      = "@RegUpdate";

}
