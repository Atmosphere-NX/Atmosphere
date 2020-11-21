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
#include <stratosphere.hpp>

namespace ams::os::impl {

    inline bool CheckFreeSpace(uintptr_t address, size_t size) {
        /* Query the memory. */
        svc::MemoryInfo memory_info;
        svc::PageInfo page_info;
        const auto result = svc::QueryMemory(std::addressof(memory_info), std::addressof(page_info), address);
        AMS_ASSERT(R_SUCCEEDED(result));

        return memory_info.state == svc::MemoryState_Free && address + size <= memory_info.addr + memory_info.size;
    }

}
