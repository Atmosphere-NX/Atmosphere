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
    constexpr inline const char ReservedMountNamePrefixCharacter         = '@';
    constexpr inline const char *MountNameDelimiter                      = ":/";

    /* Filesystem names. */
    constexpr inline const char *HostRootFileSystemMountName             = "@Host";
    constexpr inline const char *SdCardFileSystemMountName               = "@Sdcard";
    constexpr inline const char *GameCardFileSystemMountName             = "@Gc";

    constexpr inline size_t GameCardFileSystemMountNameSuffixLength      = 1;
    constexpr inline const char *GameCardFileSystemMountNameUpdateSuffix = "U";
    constexpr inline const char *GameCardFileSystemMountNameNormalSuffix = "N";
    constexpr inline const char *GameCardFileSystemMountNameSecureSuffix = "S";

    /* Built-in storage names. */
    constexpr inline const char *BisCalibrationFilePartitionMountName    = "@CalibFile";
    constexpr inline const char *BisSafeModePartitionMountName           = "@Safe";
    constexpr inline const char *BisUserPartitionMountName               = "@User";
    constexpr inline const char *BisSystemPartitionMountName             = "@System";

    /* Registered update partition. */
    constexpr inline const char *RegisteredUpdatePartitionMountName      = "@RegUpdate";

}
