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
#include <mesosphere.hpp>

namespace ams::kern::svc {

    /* =============================    Common    ============================= */

    namespace {

        Result SetHeapSize(uintptr_t *out_address, size_t size) {
            /* Validate size. */
            R_UNLESS(util::IsAligned(size, ams::svc::HeapSizeAlignment), svc::ResultInvalidSize());
            R_UNLESS(size < ams::kern::MainMemorySize,                   svc::ResultInvalidSize());

            /* Set the heap size. */
            KProcessAddress address;
            R_TRY(GetCurrentProcess().GetPageTable().SetHeapSize(std::addressof(address), size));

            /* Set the output. */
            *out_address = GetInteger(address);
            return ResultSuccess();
        }

        Result SetUnsafeLimit(size_t limit) {
            /* Ensure the size is aligned. */
            R_UNLESS(util::IsAligned(limit, PageSize), svc::ResultInvalidSize());

            /* Ensure that the size is not bigger than we can accommodate. */
            R_UNLESS(limit <= Kernel::GetMemoryManager().GetSize(KMemoryManager::Pool_Unsafe), svc::ResultOutOfRange());

            /* Set the size. */
            return Kernel::GetUnsafeMemory().SetLimitSize(limit);
        }

        Result MapPhysicalMemory(uintptr_t address, size_t size) {
            /* Validate address / size. */
            R_UNLESS(util::IsAligned(address, PageSize), svc::ResultInvalidAddress());
            R_UNLESS(util::IsAligned(size,    PageSize), svc::ResultInvalidSize());
            R_UNLESS(size > 0,                           svc::ResultInvalidSize());
            R_UNLESS((address < address + size),         svc::ResultInvalidMemoryRegion());

            /* Verify that the process has system resource. */
            auto &process = GetCurrentProcess();
            R_UNLESS(process.GetTotalSystemResourceSize() > 0, svc::ResultInvalidState());

            /* Verify that the region is in range. */
            auto &page_table = process.GetPageTable();
            R_UNLESS(page_table.IsInAliasRegion(address, size), svc::ResultInvalidMemoryRegion());

            /* Map the memory. */
            R_TRY(page_table.MapPhysicalMemory(address, size));

            return ResultSuccess();
        }

        Result UnmapPhysicalMemory(uintptr_t address, size_t size) {
            /* Validate address / size. */
            R_UNLESS(util::IsAligned(address, PageSize), svc::ResultInvalidAddress());
            R_UNLESS(util::IsAligned(size,    PageSize), svc::ResultInvalidSize());
            R_UNLESS(size > 0,                           svc::ResultInvalidSize());
            R_UNLESS((address < address + size),         svc::ResultInvalidMemoryRegion());

            /* Verify that the process has system resource. */
            auto &process = GetCurrentProcess();
            R_UNLESS(process.GetTotalSystemResourceSize() > 0, svc::ResultInvalidState());

            /* Verify that the region is in range. */
            auto &page_table = process.GetPageTable();
            R_UNLESS(page_table.IsInAliasRegion(address, size), svc::ResultInvalidMemoryRegion());

            /* Unmap the memory. */
            R_TRY(page_table.UnmapPhysicalMemory(address, size));

            return ResultSuccess();
        }

        Result MapPhysicalMemoryUnsafe(uintptr_t address, size_t size) {
            /* Validate address / size. */
            R_UNLESS(util::IsAligned(address, PageSize), svc::ResultInvalidAddress());
            R_UNLESS(util::IsAligned(size,    PageSize), svc::ResultInvalidSize());
            R_UNLESS(size > 0,                           svc::ResultInvalidSize());
            R_UNLESS((address < address + size),         svc::ResultInvalidCurrentMemory());

            /* Verify that the region is in range. */
            auto &process    = GetCurrentProcess();
            auto &page_table = process.GetPageTable();
            R_UNLESS(page_table.IsInUnsafeAliasRegion(address, size), svc::ResultInvalidMemoryRegion());

            /* Verify that the process isn't already using the unsafe pool. */
            R_UNLESS(process.GetMemoryPool() != KMemoryManager::Pool_Unsafe, svc::ResultInvalidMemoryPool());

            /* Map the memory. */
            R_TRY(page_table.MapPhysicalMemoryUnsafe(address, size));

            return ResultSuccess();
        }

        Result UnmapPhysicalMemoryUnsafe(uintptr_t address, size_t size) {
            /* Validate address / size. */
            R_UNLESS(util::IsAligned(address, PageSize), svc::ResultInvalidAddress());
            R_UNLESS(util::IsAligned(size,    PageSize), svc::ResultInvalidSize());
            R_UNLESS(size > 0,                           svc::ResultInvalidSize());
            R_UNLESS((address < address + size),         svc::ResultInvalidCurrentMemory());

            /* Verify that the region is in range. */
            auto &process    = GetCurrentProcess();
            auto &page_table = process.GetPageTable();
            R_UNLESS(page_table.IsInUnsafeAliasRegion(address, size), svc::ResultInvalidMemoryRegion());

            /* Unmap the memory. */
            R_TRY(page_table.UnmapPhysicalMemoryUnsafe(address, size));

            return ResultSuccess();
        }

    }

    /* =============================    64 ABI    ============================= */

    Result SetHeapSize64(ams::svc::Address *out_address, ams::svc::Size size) {
        static_assert(sizeof(*out_address) == sizeof(uintptr_t));
        return SetHeapSize(reinterpret_cast<uintptr_t *>(out_address), size);
    }

    Result MapPhysicalMemory64(ams::svc::Address address, ams::svc::Size size) {
        return MapPhysicalMemory(address, size);
    }

    Result UnmapPhysicalMemory64(ams::svc::Address address, ams::svc::Size size) {
        return UnmapPhysicalMemory(address, size);
    }

    Result MapPhysicalMemoryUnsafe64(ams::svc::Address address, ams::svc::Size size) {
        return MapPhysicalMemoryUnsafe(address, size);
    }

    Result UnmapPhysicalMemoryUnsafe64(ams::svc::Address address, ams::svc::Size size) {
        return UnmapPhysicalMemoryUnsafe(address, size);
    }

    Result SetUnsafeLimit64(ams::svc::Size limit) {
        return SetUnsafeLimit(limit);
    }

    /* ============================= 64From32 ABI ============================= */

    Result SetHeapSize64From32(ams::svc::Address *out_address, ams::svc::Size size) {
        static_assert(sizeof(*out_address) == sizeof(uintptr_t));
        return SetHeapSize(reinterpret_cast<uintptr_t *>(out_address), size);
    }

    Result MapPhysicalMemory64From32(ams::svc::Address address, ams::svc::Size size) {
        return MapPhysicalMemory(address, size);
    }

    Result UnmapPhysicalMemory64From32(ams::svc::Address address, ams::svc::Size size) {
        return UnmapPhysicalMemory(address, size);
    }

    Result MapPhysicalMemoryUnsafe64From32(ams::svc::Address address, ams::svc::Size size) {
        return MapPhysicalMemoryUnsafe(address, size);
    }

    Result UnmapPhysicalMemoryUnsafe64From32(ams::svc::Address address, ams::svc::Size size) {
        return UnmapPhysicalMemoryUnsafe(address, size);
    }

    Result SetUnsafeLimit64From32(ams::svc::Size limit) {
        return SetUnsafeLimit(limit);
    }

}
