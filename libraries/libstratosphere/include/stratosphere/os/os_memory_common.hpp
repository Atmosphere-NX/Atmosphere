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

    using AddressSpaceGenerateRandomFunction = u64 (*)(u64);

    enum MemoryPermission {
        MemoryPermission_None        = (0 << 0),
        MemoryPermission_ReadOnly    = (1 << 0),
        MemoryPermission_WriteOnly   = (1 << 1),
        MemoryPermission_ExecuteOnly = (1 << 2),

        MemoryPermission_ReadWrite   = MemoryPermission_ReadOnly | MemoryPermission_WriteOnly,
        MemoryPermission_ReadExecute = MemoryPermission_ReadOnly | MemoryPermission_ExecuteOnly,
    };

    #if defined(ATMOSPHERE_OS_HORIZON)
        using MemoryMapping = svc::MemoryMapping;
        using enum svc::MemoryMapping;
    #else
        enum MemoryMapping : u32 {
            MemoryMapping_IoRegister = 0,
            MemoryMapping_Uncached   = 1,
            MemoryMapping_Memory     = 2,
        };
    #endif

}
