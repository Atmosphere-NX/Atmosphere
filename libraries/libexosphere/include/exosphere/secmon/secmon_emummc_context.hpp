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
#include <vapours.hpp>

namespace ams::secmon {

    enum EmummcType : u32 {
        EmummcType_None      = 0,
        EmummcType_Partition = 1,
        EmummcType_File      = 2,
    };

    enum EmummcMmc {
        EmummcMmc_Nand = 0,
        EmummcMmc_Sd   = 1,
        EmummcMmc_Gc   = 2,
    };

    constexpr inline size_t EmummcFilePathLengthMax = 0x80;

    struct EmummcFilePath {
        char str[EmummcFilePathLengthMax];
    };
    static_assert(util::is_pod<EmummcFilePath>::value);
    static_assert(sizeof(EmummcFilePath) == EmummcFilePathLengthMax);

    struct EmummcBaseConfiguration {
        static constexpr u32 Magic = util::FourCC<'E','F','S','0'>::Code;

        u32 magic;
        EmummcType type;
        u32 id;
        u32 fs_version;

        constexpr bool IsValid() const {
            return this->magic == Magic;
        }

        constexpr bool IsEmummcActive() const {
            return this->IsValid() && this->type != EmummcType_None;
        }
    };
    static_assert(util::is_pod<EmummcBaseConfiguration>::value);
    static_assert(sizeof(EmummcBaseConfiguration) == 0x10);

    struct EmummcPartitionConfiguration {
        u64 start_sector;
    };
    static_assert(util::is_pod<EmummcPartitionConfiguration>::value);

    struct EmummcFileConfiguration {
        EmummcFilePath path;
    };
    static_assert(util::is_pod<EmummcFileConfiguration>::value);

    struct EmummcConfiguration {
        EmummcBaseConfiguration base_cfg;
        union {
            EmummcPartitionConfiguration partition_cfg;
            EmummcFileConfiguration file_cfg;
        };
        EmummcFilePath emu_dir_path;

        constexpr bool IsValid() const {
            return this->base_cfg.IsValid();
        }

        constexpr bool IsEmummcActive() const {
            return this->base_cfg.IsEmummcActive();
        }
    };
    static_assert(util::is_pod<EmummcConfiguration>::value);
    static_assert(sizeof(EmummcConfiguration) <= 0x200);

}