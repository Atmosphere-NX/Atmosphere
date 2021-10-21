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
#include <stratosphere.hpp>
#include "util_catch.hpp"
#include "util_check_memory.hpp"
#include "util_scoped_heap.hpp"

namespace ams::test {

    namespace {

        bool CanSetMemoryPermission(u8 state) {
            return state == svc::MemoryState_CodeData || state == svc::MemoryState_AliasCodeData || state == svc::MemoryState_Normal;
        }

    }

    alignas(os::MemoryPageSize) constinit u8 g_memory_permission_buffer[2 * os::MemoryPageSize];

    CATCH_TEST_CASE("svc::SetMemoryPermission invalid arguments") {
        const uintptr_t buffer = reinterpret_cast<uintptr_t>(g_memory_permission_buffer);

        for (size_t i = 1; i < os::MemoryPageSize; ++i) {
            CATCH_REQUIRE(svc::ResultInvalidAddress::Includes(svc::SetMemoryPermission(buffer + i, os::MemoryPageSize, svc::MemoryPermission_Read)));
            CATCH_REQUIRE(svc::ResultInvalidSize::Includes(svc::SetMemoryPermission(buffer, os::MemoryPageSize + i, svc::MemoryPermission_Read)));
        }

        CATCH_REQUIRE(svc::ResultInvalidSize::Includes(svc::SetMemoryPermission(buffer, 0, svc::MemoryPermission_Read)));

        {
            const u64 vmem_end = util::AlignDown(std::numeric_limits<u64>::max(), os::MemoryPageSize);
            CATCH_REQUIRE(svc::ResultInvalidCurrentMemory::Includes(svc::SetMemoryPermission(vmem_end, 2 * os::MemoryPageSize, svc::MemoryPermission_Read)));
        }

        CATCH_REQUIRE(svc::ResultInvalidCurrentMemory::Includes(svc::SetMemoryPermission(svc::AddressMap39End, os::MemoryPageSize, svc::MemoryPermission_Read)));

        for (size_t i = 0; i < 0x100; ++i) {
            const auto perm = static_cast<svc::MemoryPermission>(i);
            if (perm == svc::MemoryPermission_None || perm == svc::MemoryPermission_Read || perm == svc::MemoryPermission_ReadWrite) {
                continue;
            }

            CATCH_REQUIRE(svc::ResultInvalidNewMemoryPermission::Includes(svc::SetMemoryPermission(buffer, os::MemoryPageSize, perm)));
        }
        CATCH_REQUIRE(svc::ResultInvalidNewMemoryPermission::Includes(svc::SetMemoryPermission(buffer, os::MemoryPageSize, svc::MemoryPermission_ReadExecute)));
        CATCH_REQUIRE(svc::ResultInvalidNewMemoryPermission::Includes(svc::SetMemoryPermission(buffer, os::MemoryPageSize, svc::MemoryPermission_Write)));
        CATCH_REQUIRE(svc::ResultInvalidNewMemoryPermission::Includes(svc::SetMemoryPermission(buffer, os::MemoryPageSize, svc::MemoryPermission_DontCare)));
    }

    CATCH_TEST_CASE("svc::SetMemoryPermission works on specific states") {
        /* Check that we have CodeData. */
        const uintptr_t bss_buffer = reinterpret_cast<uintptr_t>(g_memory_permission_buffer);
        TestMemory(bss_buffer, sizeof(g_memory_permission_buffer), svc::MemoryState_CodeData, svc::MemoryPermission_ReadWrite, 0);

        /* Create a heap. */
        ScopedHeap scoped_heap(2 * svc::HeapSizeAlignment);
        TestMemory(scoped_heap.GetAddress(), scoped_heap.GetSize(), svc::MemoryState_Normal, svc::MemoryPermission_ReadWrite, 0);

        /* TODO: Ensure we have alias code data? */

        uintptr_t addr = 0;
        while (true) {
            /* Get current mapping. */
            svc::MemoryInfo mem_info;
            svc::PageInfo page_info;
            CATCH_REQUIRE(R_SUCCEEDED(svc::QueryMemory(std::addressof(mem_info), std::addressof(page_info), addr)));

            /* Try to set permission. */
            if (CanSetMemoryPermission(mem_info.state) && mem_info.attribute == 0) {
                CATCH_REQUIRE(R_SUCCEEDED(svc::SetMemoryPermission(mem_info.base_address, mem_info.size, svc::MemoryPermission_ReadWrite)));
                TestMemory(mem_info.base_address, mem_info.size, mem_info.state, svc::MemoryPermission_ReadWrite, mem_info.attribute);
                CATCH_REQUIRE(R_SUCCEEDED(svc::SetMemoryPermission(mem_info.base_address, mem_info.size, mem_info.permission)));
            } else {
                CATCH_REQUIRE(svc::ResultInvalidCurrentMemory::Includes(svc::SetMemoryPermission(mem_info.base_address, mem_info.size, svc::MemoryPermission_Read)));
            }

            const uintptr_t next_address = mem_info.base_address + mem_info.size;
            if (next_address <= addr) {
                break;
            }

            addr = next_address;
        }
    }

    CATCH_TEST_CASE("svc::SetMemoryPermission allows for free movement between RW-, R--, ---") {
        /* Define helper. */
        auto test_set_memory_permission = [](uintptr_t address, size_t size){
            /* Get the permission. */
            svc::MemoryInfo mem_info;
            svc::PageInfo page_info;
            CATCH_REQUIRE(R_SUCCEEDED(svc::QueryMemory(std::addressof(mem_info), std::addressof(page_info), address)));

            const svc::MemoryPermission legal_states[] = { svc::MemoryPermission_None, svc::MemoryPermission_Read, svc::MemoryPermission_ReadWrite };
            for (const auto src_state : legal_states) {
                for (const auto dst_state : legal_states) {
                    CATCH_REQUIRE(R_SUCCEEDED(svc::SetMemoryPermission(address, size, svc::MemoryPermission_None)));
                    CATCH_REQUIRE(R_SUCCEEDED(svc::SetMemoryPermission(address, size, src_state)));
                    CATCH_REQUIRE(R_SUCCEEDED(svc::SetMemoryPermission(address, size, dst_state)));
                    CATCH_REQUIRE(R_SUCCEEDED(svc::SetMemoryPermission(address, size, svc::MemoryPermission_None)));
                }
            }

            CATCH_REQUIRE(R_SUCCEEDED(svc::SetMemoryPermission(address, size, mem_info.permission)));
        };

        /* Test that we can freely move about .bss buffers. */
        test_set_memory_permission(reinterpret_cast<uintptr_t>(g_memory_permission_buffer), sizeof(g_memory_permission_buffer));

        /* Create a heap. */
        ScopedHeap scoped_heap(svc::HeapSizeAlignment);
        TestMemory(scoped_heap.GetAddress(), scoped_heap.GetSize(), svc::MemoryState_Normal, svc::MemoryPermission_ReadWrite, 0);

        /* Test that we can freely move about heap. */
        test_set_memory_permission(scoped_heap.GetAddress(), scoped_heap.GetSize());

        /* TODO: AliasCodeData */
    }

    CATCH_TEST_CASE("svc::SetMemoryPermission fails when the memory has non-zero attribute") {
        const uintptr_t bss_buffer = reinterpret_cast<uintptr_t>(g_memory_permission_buffer);
        TestMemory(bss_buffer, sizeof(g_memory_permission_buffer), svc::MemoryState_CodeData, svc::MemoryPermission_ReadWrite, 0);

        CATCH_REQUIRE(R_SUCCEEDED(svc::SetMemoryPermission(bss_buffer, sizeof(g_memory_permission_buffer), svc::MemoryPermission_None)));
        CATCH_REQUIRE(R_SUCCEEDED(svc::SetMemoryPermission(bss_buffer, sizeof(g_memory_permission_buffer), svc::MemoryPermission_Read)));
        CATCH_REQUIRE(R_SUCCEEDED(svc::SetMemoryPermission(bss_buffer, sizeof(g_memory_permission_buffer), svc::MemoryPermission_ReadWrite)));

        CATCH_REQUIRE(R_SUCCEEDED(svc::SetMemoryAttribute(bss_buffer, sizeof(g_memory_permission_buffer), svc::MemoryAttribute_Uncached, svc::MemoryAttribute_Uncached)));
        TestMemory(bss_buffer, sizeof(g_memory_permission_buffer), svc::MemoryState_CodeData, svc::MemoryPermission_ReadWrite, svc::MemoryAttribute_Uncached);

        CATCH_REQUIRE(svc::ResultInvalidCurrentMemory::Includes(svc::SetMemoryPermission(bss_buffer, sizeof(g_memory_permission_buffer), svc::MemoryPermission_None)));
        CATCH_REQUIRE(svc::ResultInvalidCurrentMemory::Includes(svc::SetMemoryPermission(bss_buffer, sizeof(g_memory_permission_buffer), svc::MemoryPermission_Read)));
        CATCH_REQUIRE(svc::ResultInvalidCurrentMemory::Includes(svc::SetMemoryPermission(bss_buffer, sizeof(g_memory_permission_buffer), svc::MemoryPermission_ReadWrite)));

        CATCH_REQUIRE(R_SUCCEEDED(svc::SetMemoryAttribute(bss_buffer, sizeof(g_memory_permission_buffer), svc::MemoryAttribute_Uncached, 0)));
        TestMemory(bss_buffer, sizeof(g_memory_permission_buffer), svc::MemoryState_CodeData, svc::MemoryPermission_ReadWrite, 0);

        CATCH_REQUIRE(R_SUCCEEDED(svc::SetMemoryPermission(bss_buffer, sizeof(g_memory_permission_buffer), svc::MemoryPermission_None)));
        CATCH_REQUIRE(R_SUCCEEDED(svc::SetMemoryPermission(bss_buffer, sizeof(g_memory_permission_buffer), svc::MemoryPermission_Read)));
        CATCH_REQUIRE(R_SUCCEEDED(svc::SetMemoryPermission(bss_buffer, sizeof(g_memory_permission_buffer), svc::MemoryPermission_ReadWrite)));
    }

}