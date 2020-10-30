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

namespace ams::ddsf {

    enum AccessMode {
        AccessMode_None   = (0u << 0),
        AccessMode_Read   = (1u << 0),
        AccessMode_Write  = (1u << 1),
        AccessMode_Shared = (1u << 2),

        AccessMode_ReadWrite       = AccessMode_Read  | AccessMode_Write,
        AccessMode_WriteShared     = AccessMode_Write | AccessMode_Shared,
        AccessMode_ReadWriteShared = AccessMode_Read  | AccessMode_WriteShared,
    };

}
