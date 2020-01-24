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
#include "map_types.hpp"

namespace ams::map {

    /* Public API. */
    Result GetProcessAddressSpaceInfo(AddressSpaceInfo *out, Handle process_h);
    Result LocateMappableSpace(uintptr_t *out_address, size_t size);
    Result MapCodeMemoryInProcess(MappedCodeMemory &out_mcm, Handle process_handle, uintptr_t base_address, size_t size);
    bool   CanAddGuardRegionsInProcess(Handle process_handle, uintptr_t address, size_t size);

}
