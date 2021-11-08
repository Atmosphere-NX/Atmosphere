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
#include "util_common.hpp"
#include "util_check_memory.hpp"

namespace ams::test {

    namespace {

        size_t GetPhysicalMemorySizeMax() {
            u64 v;
            R_ABORT_UNLESS(svc::GetInfo(std::addressof(v), svc::InfoType_ResourceLimit, svc::InvalidHandle, 0));

            const svc::Handle resource_limit = v;
            ON_SCOPE_EXIT { svc::CloseHandle(resource_limit); };

            s64 size;
            R_ABORT_UNLESS(svc::GetResourceLimitLimitValue(std::addressof(size), resource_limit, svc::LimitableResource_PhysicalMemoryMax));

            return static_cast<size_t>(size);
        }

        size_t GetPhysicalMemorySizeAvailable() {
            u64 v;
            R_ABORT_UNLESS(svc::GetInfo(std::addressof(v), svc::InfoType_ResourceLimit, svc::InvalidHandle, 0));

            const svc::Handle resource_limit = v;
            ON_SCOPE_EXIT { svc::CloseHandle(resource_limit); };

            s64 total;
            R_ABORT_UNLESS(svc::GetResourceLimitLimitValue(std::addressof(total), resource_limit, svc::LimitableResource_PhysicalMemoryMax));

            s64 current;
            R_ABORT_UNLESS(svc::GetResourceLimitCurrentValue(std::addressof(current), resource_limit, svc::LimitableResource_PhysicalMemoryMax));

            return static_cast<size_t>(total - current);
        }

    }

    DOCTEST_TEST_CASE("svc::SetHeapSize") {
        svc::MemoryInfo mem_info;
        svc::PageInfo page_info;
        uintptr_t dummy;

        /* Reset the heap. */
        uintptr_t addr;
        DOCTEST_CHECK(R_SUCCEEDED(svc::SetHeapSize(std::addressof(addr), 0)));

        /* Ensure that we don't leak memory. */
        const size_t initial_memory = GetPhysicalMemorySizeAvailable();
        ON_SCOPE_EXIT { DOCTEST_CHECK(initial_memory == GetPhysicalMemorySizeAvailable()); };

        DOCTEST_SUBCASE("Unaligned and too big sizes fail") {
            for (size_t i = 1; i < svc::HeapSizeAlignment; i = util::AlignUp(i + 1, os::MemoryPageSize)){
                DOCTEST_CHECK(svc::ResultInvalidSize::Includes(svc::SetHeapSize(std::addressof(dummy), i)));
            }
            DOCTEST_CHECK(svc::ResultInvalidSize::Includes(svc::SetHeapSize(std::addressof(dummy), 64_GB)));
        }

        DOCTEST_SUBCASE("Larger size than address space fails") {
            DOCTEST_CHECK(svc::ResultOutOfMemory::Includes(svc::SetHeapSize(std::addressof(dummy), util::AlignUp(svc::AddressMemoryRegionHeap39Size + 1, svc::HeapSizeAlignment))));
        }

        DOCTEST_SUBCASE("Bounded by resource limit") {
            DOCTEST_CHECK(svc::ResultLimitReached::Includes(svc::SetHeapSize(std::addressof(dummy), util::AlignUp(GetPhysicalMemorySizeMax() + 1, svc::HeapSizeAlignment))));
            DOCTEST_CHECK(svc::ResultLimitReached::Includes(svc::SetHeapSize(std::addressof(dummy), util::AlignUp(GetPhysicalMemorySizeAvailable() + 1, svc::HeapSizeAlignment))));
        }

        DOCTEST_SUBCASE("SetHeapSize gives heap memory") {
            DOCTEST_CHECK(R_SUCCEEDED(svc::SetHeapSize(std::addressof(addr), svc::HeapSizeAlignment)));
            TestMemory(addr, svc::HeapSizeAlignment, svc::MemoryState_Normal, svc::MemoryPermission_ReadWrite, 0);

            DOCTEST_CHECK(R_SUCCEEDED(svc::SetHeapSize(std::addressof(addr), 0)));
        }

        DOCTEST_SUBCASE("SetHeapSize cannot remove read-only heap") {
            DOCTEST_CHECK(R_SUCCEEDED(svc::SetHeapSize(std::addressof(addr), svc::HeapSizeAlignment)));

            DOCTEST_CHECK(R_SUCCEEDED(svc::QueryMemory(std::addressof(mem_info), std::addressof(page_info), addr)));
            TestMemory(addr, svc::HeapSizeAlignment, svc::MemoryState_Normal, svc::MemoryPermission_ReadWrite, 0);

            DOCTEST_CHECK(R_SUCCEEDED(svc::SetMemoryPermission(addr, svc::HeapSizeAlignment, svc::MemoryPermission_Read)));
            TestMemory(addr, svc::HeapSizeAlignment, svc::MemoryState_Normal, svc::MemoryPermission_Read, 0);

            DOCTEST_CHECK(svc::ResultInvalidCurrentMemory::Includes(svc::SetHeapSize(std::addressof(dummy), 0)));

            DOCTEST_CHECK(R_SUCCEEDED(svc::SetMemoryPermission(addr, svc::HeapSizeAlignment, svc::MemoryPermission_ReadWrite)));
            TestMemory(addr, svc::HeapSizeAlignment, svc::MemoryState_Normal, svc::MemoryPermission_ReadWrite, 0);

            DOCTEST_CHECK(R_SUCCEEDED(svc::SetHeapSize(std::addressof(addr), 0)));
        }

        DOCTEST_SUBCASE("Heap memory does not survive unmap/re-map") {
            DOCTEST_CHECK(R_SUCCEEDED(svc::SetHeapSize(std::addressof(addr), 2 * svc::HeapSizeAlignment)));

            u8 * const heap = reinterpret_cast<u8 *>(addr);

            std::memset(heap, 0xAA, svc::HeapSizeAlignment);
            std::memset(heap + svc::HeapSizeAlignment, 0xBB, svc::HeapSizeAlignment);

            DOCTEST_CHECK(heap[svc::HeapSizeAlignment] == 0xBB);
            DOCTEST_CHECK(std::memcmp(heap + svc::HeapSizeAlignment, heap + svc::HeapSizeAlignment + 1, svc::HeapSizeAlignment - 1) == 0);

            DOCTEST_CHECK(R_SUCCEEDED(svc::SetHeapSize(std::addressof(addr), svc::HeapSizeAlignment)));

            DOCTEST_CHECK(heap[0] == 0xAA);
            DOCTEST_CHECK(std::memcmp(heap, heap + 1, svc::HeapSizeAlignment - 1) == 0);

            DOCTEST_CHECK(R_SUCCEEDED(svc::SetHeapSize(std::addressof(addr), 2 * svc::HeapSizeAlignment)));

            DOCTEST_CHECK(heap[svc::HeapSizeAlignment] == 0x00);
            DOCTEST_CHECK(std::memcmp(heap + svc::HeapSizeAlignment, heap + svc::HeapSizeAlignment + 1, svc::HeapSizeAlignment - 1) == 0);

            DOCTEST_CHECK(R_SUCCEEDED(svc::SetHeapSize(std::addressof(addr), 0)));
        }
    }

}