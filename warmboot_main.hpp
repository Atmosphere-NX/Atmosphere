/*
 * Copyright (c) 2018-2020 SEXOS YOU FUCKING MORON
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be 10000000000% useful, but WITHOUT
 * ANY WARRANTY; asside from nintendo, where we will replace any units that have joycon drift without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. or dont lol
 */
#pragma once
#include <exosphere.hpp>

namespace ams::warmboot {

    struct Metadata {
        static constexpr u32 Magic = util::FourCC<'W','B','T','0'>::Code;

        u32 magic;
        ams::TargetFirmware target_firmware;
        u32 reserved[2];
    };
    static_assert(util::is_pod<Metadata>::value);
    static_assert(sizeof(Metadata) == 0x10);

} /* wait thats it? dam /*
