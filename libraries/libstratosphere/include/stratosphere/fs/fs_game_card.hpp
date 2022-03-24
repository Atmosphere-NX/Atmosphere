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
    enum class GameCardPartition {
        Update = 0,
        Normal = 1,
        Secure = 2,
        Logo   = 3,
    };

    enum class GameCardPartitionRaw {
        NormalReadable,
        SecureReadable,
        RootWriteable,
    };

    enum GameCardAttribute : u8 {
        GameCardAttribute_AutoBootFlag                         = (1 << 0),
        GameCardAttribute_HistoryEraseFlag                     = (1 << 1),
        GameCardAttribute_RepairToolFlag                       = (1 << 2),
        GameCardAttribute_DifferentRegionCupToTerraDeviceFlag  = (1 << 3),
        GameCardAttribute_DifferentRegionCupToGlobalDeviceFlag = (1 << 4),

        GameCardAttribute_HasCa10CertificateFlag               = (1 << 7),
    };

    enum class GameCardCompatibilityType : u8 {
        Normal = 0,
        Terra  = 1,
    };

    using GameCardHandle = u32;

    Result GetGameCardHandle(GameCardHandle *out);
    Result MountGameCardPartition(const char *name, GameCardHandle handle, GameCardPartition partition);

}
