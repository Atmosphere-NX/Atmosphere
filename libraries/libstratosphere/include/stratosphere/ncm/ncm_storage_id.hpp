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
#include <vapours.hpp>

namespace ams::ncm {

    enum class StorageId : u8 {
        None            = 0,
        Host            = 1,
        GameCard        = 2,
        BuiltInSystem   = 3,
        BuiltInUser     = 4,
        SdCard          = 5,
        Any             = 6,

        /* Aliases. */
        Card            = GameCard,
        BuildInSystem   = BuiltInSystem,
        BuildInUser     = BuiltInUser,
    };

    constexpr inline bool IsUniqueStorage(StorageId id) {
        return id != StorageId::None && id != StorageId::Any;
    }

    constexpr inline bool IsInstallableStorage(StorageId id) {
        return id == StorageId::BuiltInSystem || id == StorageId::BuiltInUser || id == StorageId::SdCard || id == StorageId::Any;
    }

}
