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

    enum class ContentMetaType : u8 {
        Unknown                 = 0x0,
        SystemProgram           = 0x1,
        SystemData              = 0x2,
        SystemUpdate            = 0x3,
        BootImagePackage        = 0x4,
        BootImagePackageSafe    = 0x5,
        Application             = 0x80,
        Patch                   = 0x81,
        AddOnContent            = 0x82,
        Delta                   = 0x83,
    };

    const char *GetContentMetaTypeString(ContentMetaType type);

}
