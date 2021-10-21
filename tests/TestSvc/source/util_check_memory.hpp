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
#include "util_catch.hpp"

namespace ams::test {

    inline void TestMemory(uintptr_t address, svc::MemoryState state, svc::MemoryPermission perm, u32 attr) {
        svc::MemoryInfo mem_info;
        svc::PageInfo page_info;
        CATCH_REQUIRE(R_SUCCEEDED(svc::QueryMemory(std::addressof(mem_info), std::addressof(page_info), address)));

        CATCH_REQUIRE(mem_info.base_address <= address);
        CATCH_REQUIRE(address < (mem_info.base_address + mem_info.size));
        CATCH_REQUIRE(mem_info.state == state);
        CATCH_REQUIRE(mem_info.permission == perm);
        CATCH_REQUIRE(mem_info.attribute == attr);
    }

    inline void TestMemory(uintptr_t address, size_t size, svc::MemoryState state, svc::MemoryPermission perm, u32 attr) {
        svc::MemoryInfo mem_info;
        svc::PageInfo page_info;
        CATCH_REQUIRE(R_SUCCEEDED(svc::QueryMemory(std::addressof(mem_info), std::addressof(page_info), address)));

        CATCH_REQUIRE(mem_info.base_address <= address);
        CATCH_REQUIRE(mem_info.base_address < (address + size));
        CATCH_REQUIRE((address + size) <= (mem_info.base_address + mem_info.size));
        CATCH_REQUIRE(mem_info.state == state);
        CATCH_REQUIRE(mem_info.permission == perm);
        CATCH_REQUIRE(mem_info.attribute == attr);
    }

}