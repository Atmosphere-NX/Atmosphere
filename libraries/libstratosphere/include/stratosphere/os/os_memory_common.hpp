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

namespace ams::os {

    constexpr inline size_t MemoryPageSize      = 0x1000;

    constexpr inline size_t MemoryBlockUnitSize = 0x200000;

    enum MemoryPermission {
        MemoryPermission_None      = (0 << 0),
        MemoryPermission_ReadOnly  = (1 << 0),
        MemoryPermission_WriteOnly = (1 << 1),

        MemoryPermission_ReadWrite = MemoryPermission_ReadOnly | MemoryPermission_WriteOnly,
    };

    using MemoryMapping = svc::MemoryMapping;
    using enum svc::MemoryMapping;

}
