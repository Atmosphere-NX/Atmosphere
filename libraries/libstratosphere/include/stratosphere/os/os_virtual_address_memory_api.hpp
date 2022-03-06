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
#include <stratosphere/os/os_virtual_address_memory_common.hpp>
#include <stratosphere/os/os_virtual_address_memory_types.hpp>

namespace ams::os {

    void InitializeVirtualAddressMemory();

    Result AllocateAddressRegion(uintptr_t *out, size_t size);
    Result AllocateMemory(uintptr_t *out, size_t size);
    Result AllocateMemoryPages(uintptr_t address, size_t size);

    Result FreeAddressRegion(uintptr_t address);
    Result FreeMemoryPages(uintptr_t address, size_t size);

    VirtualAddressMemoryResourceUsage GetVirtualAddressMemoryResourceUsage();

    bool IsVirtualAddressMemoryEnabled();

}
