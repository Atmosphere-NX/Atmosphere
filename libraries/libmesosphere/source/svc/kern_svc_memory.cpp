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

        constexpr bool IsValidSetMemoryPermission(ams::svc::MemoryPermission perm) {
            switch (perm) {
                case ams::svc::MemoryPermission_None:
                case ams::svc::MemoryPermission_Read:
                case ams::svc::MemoryPermission_ReadWrite:
                    return true;
                default:
                    return false;
            }
        }

        Result SetMemoryPermission(uintptr_t address, size_t size, ams::svc::MemoryPermission perm) {
            /* Validate address / size. */
            R_UNLESS(util::IsAligned(address, PageSize), svc::ResultInvalidAddress());
            R_UNLESS(util::IsAligned(size,    PageSize), svc::ResultInvalidSize());
            R_UNLESS(size > 0,                           svc::ResultInvalidSize());
            R_UNLESS((address < address + size),         svc::ResultInvalidCurrentMemory());

            /* Validate the permission. */
            R_UNLESS(IsValidSetMemoryPermission(perm), svc::ResultInvalidNewMemoryPermission());

            /* Validate that the region is in range for the current process. */
            auto &page_table = GetCurrentProcess().GetPageTable();
            R_UNLESS(page_table.Contains(address, size), svc::ResultInvalidCurrentMemory());

            /* Set the memory attribute. */
            return page_table.SetMemoryPermission(address, size, perm);
        }

        Result SetMemoryAttribute(uintptr_t address, size_t size, uint32_t mask, uint32_t attr) {
            /* Validate address / size. */
            R_UNLESS(util::IsAligned(address, PageSize), svc::ResultInvalidAddress());
            R_UNLESS(util::IsAligned(size,    PageSize), svc::ResultInvalidSize());
            R_UNLESS(size > 0,                           svc::ResultInvalidSize());
            R_UNLESS((address < address + size),         svc::ResultInvalidCurrentMemory());

            /* Validate the attribute and mask. */
            constexpr u32 SupportedMask = ams::svc::MemoryAttribute_Uncached;
            R_UNLESS((mask | attr) == mask,                          svc::ResultInvalidCombination());
            R_UNLESS((mask | attr | SupportedMask) == SupportedMask, svc::ResultInvalidCombination());

            /* Validate that the region is in range for the current process. */
            auto &page_table = GetCurrentProcess().GetPageTable();
            R_UNLESS(page_table.Contains(address, size), svc::ResultInvalidCurrentMemory());

            /* Set the memory attribute. */
            return page_table.SetMemoryAttribute(address, size, mask, attr);
        }

        Result MapMemory(uintptr_t dst_address, uintptr_t src_address, size_t size) {
            /* Validate that addresses are page aligned. */
            R_UNLESS(util::IsAligned(dst_address, PageSize), svc::ResultInvalidAddress());
            R_UNLESS(util::IsAligned(src_address, PageSize), svc::ResultInvalidAddress());

            /* Validate that size is positive and page aligned. */
            R_UNLESS(size > 0,                        svc::ResultInvalidSize());
            R_UNLESS(util::IsAligned(size, PageSize), svc::ResultInvalidSize());

            /* Ensure that neither mapping overflows. */
            R_UNLESS(src_address < src_address + size, svc::ResultInvalidCurrentMemory());
            R_UNLESS(dst_address < dst_address + size, svc::ResultInvalidCurrentMemory());

            /* Get the page table we're operating on. */
            auto &page_table = GetCurrentProcess().GetPageTable();

            /* Ensure that the memory we're mapping is in range. */
            R_UNLESS(page_table.Contains(src_address, size),                       svc::ResultInvalidCurrentMemory());
            R_UNLESS(page_table.CanContain(dst_address, size, KMemoryState_Stack), svc::ResultInvalidMemoryRegion());

            /* Map the memory. */
            return page_table.MapMemory(dst_address, src_address, size);
        }

        Result UnmapMemory(uintptr_t dst_address, uintptr_t src_address, size_t size) {
            /* Validate that addresses are page aligned. */
            R_UNLESS(util::IsAligned(dst_address, PageSize), svc::ResultInvalidAddress());
            R_UNLESS(util::IsAligned(src_address, PageSize), svc::ResultInvalidAddress());

            /* Validate that size is positive and page aligned. */
            R_UNLESS(size > 0,                        svc::ResultInvalidSize());
            R_UNLESS(util::IsAligned(size, PageSize), svc::ResultInvalidSize());

            /* Ensure that neither mapping overflows. */
            R_UNLESS(src_address < src_address + size, svc::ResultInvalidCurrentMemory());
            R_UNLESS(dst_address < dst_address + size, svc::ResultInvalidCurrentMemory());

            /* Get the page table we're operating on. */
            auto &page_table = GetCurrentProcess().GetPageTable();

            /* Ensure that the memory we're unmapping is in range. */
            R_UNLESS(page_table.Contains(src_address, size),                       svc::ResultInvalidCurrentMemory());
            R_UNLESS(page_table.CanContain(dst_address, size, KMemoryState_Stack), svc::ResultInvalidMemoryRegion());

            /* Unmap the memory. */
            return page_table.UnmapMemory(dst_address, src_address, size);
        }

    }

    /* =============================    64 ABI    ============================= */

    Result SetMemoryPermission64(ams::svc::Address address, ams::svc::Size size, ams::svc::MemoryPermission perm) {
        return SetMemoryPermission(address, size, perm);
    }

    Result SetMemoryAttribute64(ams::svc::Address address, ams::svc::Size size, uint32_t mask, uint32_t attr) {
        return SetMemoryAttribute(address, size, mask, attr);
    }

    Result MapMemory64(ams::svc::Address dst_address, ams::svc::Address src_address, ams::svc::Size size) {
        return MapMemory(dst_address, src_address, size);
    }

    Result UnmapMemory64(ams::svc::Address dst_address, ams::svc::Address src_address, ams::svc::Size size) {
        return UnmapMemory(dst_address, src_address, size);
    }

    /* ============================= 64From32 ABI ============================= */

    Result SetMemoryPermission64From32(ams::svc::Address address, ams::svc::Size size, ams::svc::MemoryPermission perm) {
        return SetMemoryPermission(address, size, perm);
    }

    Result SetMemoryAttribute64From32(ams::svc::Address address, ams::svc::Size size, uint32_t mask, uint32_t attr) {
        return SetMemoryAttribute(address, size, mask, attr);
    }

    Result MapMemory64From32(ams::svc::Address dst_address, ams::svc::Address src_address, ams::svc::Size size) {
        return MapMemory(dst_address, src_address, size);
    }

    Result UnmapMemory64From32(ams::svc::Address dst_address, ams::svc::Address src_address, ams::svc::Size size) {
        return UnmapMemory(dst_address, src_address, size);
    }

}
