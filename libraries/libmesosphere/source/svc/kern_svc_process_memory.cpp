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
#include <mesosphere.hpp>

namespace ams::kern::svc {

    /* =============================    Common    ============================= */

    namespace {

        constexpr bool IsValidProcessMemoryPermission(ams::svc::MemoryPermission perm) {
            switch (perm) {
                case ams::svc::MemoryPermission_None:
                case ams::svc::MemoryPermission_Read:
                case ams::svc::MemoryPermission_ReadWrite:
                case ams::svc::MemoryPermission_ReadExecute:
                    return true;
                default:
                    return false;
            }
        }

        Result SetProcessMemoryPermission(ams::svc::Handle process_handle, uint64_t address, uint64_t size, ams::svc::MemoryPermission perm) {
            /* Validate the address/size. */
            R_UNLESS(util::IsAligned(address, PageSize),         svc::ResultInvalidAddress());
            R_UNLESS(util::IsAligned(size,    PageSize),         svc::ResultInvalidSize());
            R_UNLESS(size > 0,                                   svc::ResultInvalidSize());
            R_UNLESS((address < address + size),                 svc::ResultInvalidCurrentMemory());
            R_UNLESS(address == static_cast<uintptr_t>(address), svc::ResultInvalidCurrentMemory());
            R_UNLESS(size == static_cast<size_t>(size),          svc::ResultInvalidCurrentMemory());

            /* Validate the memory permission. */
            R_UNLESS(IsValidProcessMemoryPermission(perm), svc::ResultInvalidNewMemoryPermission());

            /* Get the process from its handle. */
            KScopedAutoObject process = GetCurrentProcess().GetHandleTable().GetObject<KProcess>(process_handle);
            R_UNLESS(process.IsNotNull(), svc::ResultInvalidHandle());

            /* Validate that the address is in range. */
            auto &page_table = process->GetPageTable();
            R_UNLESS(page_table.Contains(address, size), svc::ResultInvalidCurrentMemory());

            /* Set the memory permission. */
            R_RETURN(page_table.SetProcessMemoryPermission(address, size, perm));
        }

        Result MapProcessMemory(uintptr_t dst_address, ams::svc::Handle process_handle, uint64_t src_address, size_t size) {
            /* Validate the address/size. */
            R_UNLESS(util::IsAligned(dst_address, PageSize),             svc::ResultInvalidAddress());
            R_UNLESS(util::IsAligned(src_address, PageSize),             svc::ResultInvalidAddress());
            R_UNLESS(util::IsAligned(size,    PageSize),                 svc::ResultInvalidSize());
            R_UNLESS(size > 0,                                           svc::ResultInvalidSize());
            R_UNLESS((dst_address < dst_address + size),                 svc::ResultInvalidCurrentMemory());
            R_UNLESS((src_address < src_address + size),                 svc::ResultInvalidCurrentMemory());
            R_UNLESS(src_address == static_cast<uintptr_t>(src_address), svc::ResultInvalidCurrentMemory());

            /* Get the processes. */
            KProcess *dst_process = GetCurrentProcessPointer();
            KScopedAutoObject src_process = dst_process->GetHandleTable().GetObjectWithoutPseudoHandle<KProcess>(process_handle);
            R_UNLESS(src_process.IsNotNull(), svc::ResultInvalidHandle());

            /* Get the page tables. */
            auto &dst_pt = dst_process->GetPageTable();
            auto &src_pt = src_process->GetPageTable();

            /* Validate that the mapping is in range. */
            R_UNLESS(src_pt.Contains(src_address, size),                            svc::ResultInvalidCurrentMemory());
            R_UNLESS(dst_pt.CanContain(dst_address, size, KMemoryState_SharedCode), svc::ResultInvalidMemoryRegion());

            /* Create a new page group. */
            KPageGroup pg(dst_pt.GetBlockInfoManager());

            /* Make the page group. */
            R_TRY(src_pt.MakeAndOpenPageGroup(std::addressof(pg),
                                              src_address, size / PageSize,
                                              KMemoryState_FlagCanMapProcess, KMemoryState_FlagCanMapProcess,
                                              KMemoryPermission_None, KMemoryPermission_None,
                                              KMemoryAttribute_All, KMemoryAttribute_None));

            /* Close the page group when we're done. */
            ON_SCOPE_EXIT { pg.Close(); };

            /* Map the group. */
            R_TRY(dst_pt.MapPageGroup(dst_address, pg, KMemoryState_SharedCode, KMemoryPermission_UserReadWrite));

            R_SUCCEED();
        }

        Result UnmapProcessMemory(uintptr_t dst_address, ams::svc::Handle process_handle, uint64_t src_address, size_t size) {
            /* Validate the address/size. */
            R_UNLESS(util::IsAligned(dst_address, PageSize),             svc::ResultInvalidAddress());
            R_UNLESS(util::IsAligned(src_address, PageSize),             svc::ResultInvalidAddress());
            R_UNLESS(util::IsAligned(size,    PageSize),                 svc::ResultInvalidSize());
            R_UNLESS(size > 0,                                           svc::ResultInvalidSize());
            R_UNLESS((dst_address < dst_address + size),                 svc::ResultInvalidCurrentMemory());
            R_UNLESS((src_address < src_address + size),                 svc::ResultInvalidCurrentMemory());
            R_UNLESS(src_address == static_cast<uintptr_t>(src_address), svc::ResultInvalidCurrentMemory());

            /* Get the processes. */
            KProcess *dst_process = GetCurrentProcessPointer();
            KScopedAutoObject src_process = dst_process->GetHandleTable().GetObjectWithoutPseudoHandle<KProcess>(process_handle);
            R_UNLESS(src_process.IsNotNull(), svc::ResultInvalidHandle());

            /* Get the page tables. */
            auto &dst_pt = dst_process->GetPageTable();
            auto &src_pt = src_process->GetPageTable();

            /* Validate that the mapping is in range. */
            R_UNLESS(src_pt.Contains(src_address, size),                            svc::ResultInvalidCurrentMemory());
            R_UNLESS(dst_pt.CanContain(dst_address, size, KMemoryState_SharedCode), svc::ResultInvalidMemoryRegion());

            /* Unmap the memory. */
            R_TRY(dst_pt.UnmapProcessMemory(dst_address, size, src_pt, src_address));

            R_SUCCEED();
        }

        Result MapProcessCodeMemory(ams::svc::Handle process_handle, uint64_t dst_address, uint64_t src_address, uint64_t size) {
            /* Validate the address/size. */
            R_UNLESS(util::IsAligned(dst_address, PageSize),             svc::ResultInvalidAddress());
            R_UNLESS(util::IsAligned(src_address, PageSize),             svc::ResultInvalidAddress());
            R_UNLESS(util::IsAligned(size,    PageSize),                 svc::ResultInvalidSize());
            R_UNLESS(size > 0,                                           svc::ResultInvalidSize());
            R_UNLESS((dst_address < dst_address + size),                 svc::ResultInvalidCurrentMemory());
            R_UNLESS((src_address < src_address + size),                 svc::ResultInvalidCurrentMemory());
            R_UNLESS(src_address == static_cast<uintptr_t>(src_address), svc::ResultInvalidCurrentMemory());
            R_UNLESS(dst_address == static_cast<uintptr_t>(dst_address), svc::ResultInvalidCurrentMemory());
            R_UNLESS(size == static_cast<size_t>(size),                  svc::ResultInvalidCurrentMemory());

            /* Get the process from its handle. */
            KScopedAutoObject process = GetCurrentProcess().GetHandleTable().GetObjectWithoutPseudoHandle<KProcess>(process_handle);
            R_UNLESS(process.IsNotNull(), svc::ResultInvalidHandle());

            /* Validate that the mapping is in range. */
            auto &page_table = process->GetPageTable();
            R_UNLESS(page_table.Contains(src_address, size),                           svc::ResultInvalidCurrentMemory());
            R_UNLESS(page_table.CanContain(dst_address, size, KMemoryState_AliasCode), svc::ResultInvalidCurrentMemory());

            /* Map the memory. */
            R_TRY(page_table.MapCodeMemory(dst_address, src_address, size));

            R_SUCCEED();
        }

        Result UnmapProcessCodeMemory(ams::svc::Handle process_handle, uint64_t dst_address, uint64_t src_address, uint64_t size) {
            /* Validate the address/size. */
            R_UNLESS(util::IsAligned(dst_address, PageSize),             svc::ResultInvalidAddress());
            R_UNLESS(util::IsAligned(src_address, PageSize),             svc::ResultInvalidAddress());
            R_UNLESS(util::IsAligned(size,    PageSize),                 svc::ResultInvalidSize());
            R_UNLESS(size > 0,                                           svc::ResultInvalidSize());
            R_UNLESS((dst_address < dst_address + size),                 svc::ResultInvalidCurrentMemory());
            R_UNLESS((src_address < src_address + size),                 svc::ResultInvalidCurrentMemory());
            R_UNLESS(src_address == static_cast<uintptr_t>(src_address), svc::ResultInvalidCurrentMemory());
            R_UNLESS(dst_address == static_cast<uintptr_t>(dst_address), svc::ResultInvalidCurrentMemory());
            R_UNLESS(size == static_cast<size_t>(size),                  svc::ResultInvalidCurrentMemory());

            /* Get the process from its handle. */
            KScopedAutoObject process = GetCurrentProcess().GetHandleTable().GetObjectWithoutPseudoHandle<KProcess>(process_handle);
            R_UNLESS(process.IsNotNull(), svc::ResultInvalidHandle());

            /* Validate that the mapping is in range. */
            auto &page_table = process->GetPageTable();
            R_UNLESS(page_table.Contains(src_address, size),                           svc::ResultInvalidCurrentMemory());
            R_UNLESS(page_table.CanContain(dst_address, size, KMemoryState_AliasCode), svc::ResultInvalidCurrentMemory());

            /* Unmap the memory. */
            R_TRY(page_table.UnmapCodeMemory(dst_address, src_address, size));

            R_SUCCEED();
        }

    }

    /* =============================    64 ABI    ============================= */

    Result SetProcessMemoryPermission64(ams::svc::Handle process_handle, uint64_t address, uint64_t size, ams::svc::MemoryPermission perm) {
        R_RETURN(SetProcessMemoryPermission(process_handle, address, size, perm));
    }

    Result MapProcessMemory64(ams::svc::Address dst_address, ams::svc::Handle process_handle, uint64_t src_address, ams::svc::Size size) {
        R_RETURN(MapProcessMemory(dst_address, process_handle, src_address, size));
    }

    Result UnmapProcessMemory64(ams::svc::Address dst_address, ams::svc::Handle process_handle, uint64_t src_address, ams::svc::Size size) {
        R_RETURN(UnmapProcessMemory(dst_address, process_handle, src_address, size));
    }

    Result MapProcessCodeMemory64(ams::svc::Handle process_handle, uint64_t dst_address, uint64_t src_address, uint64_t size) {
        R_RETURN(MapProcessCodeMemory(process_handle, dst_address, src_address, size));
    }

    Result UnmapProcessCodeMemory64(ams::svc::Handle process_handle, uint64_t dst_address, uint64_t src_address, uint64_t size) {
        R_RETURN(UnmapProcessCodeMemory(process_handle, dst_address, src_address, size));
    }

    /* ============================= 64From32 ABI ============================= */

    Result SetProcessMemoryPermission64From32(ams::svc::Handle process_handle, uint64_t address, uint64_t size, ams::svc::MemoryPermission perm) {
        R_RETURN(SetProcessMemoryPermission(process_handle, address, size, perm));
    }

    Result MapProcessMemory64From32(ams::svc::Address dst_address, ams::svc::Handle process_handle, uint64_t src_address, ams::svc::Size size) {
        R_RETURN(MapProcessMemory(dst_address, process_handle, src_address, size));
    }

    Result UnmapProcessMemory64From32(ams::svc::Address dst_address, ams::svc::Handle process_handle, uint64_t src_address, ams::svc::Size size) {
        R_RETURN(UnmapProcessMemory(dst_address, process_handle, src_address, size));
    }

    Result MapProcessCodeMemory64From32(ams::svc::Handle process_handle, uint64_t dst_address, uint64_t src_address, uint64_t size) {
        R_RETURN(MapProcessCodeMemory(process_handle, dst_address, src_address, size));
    }

    Result UnmapProcessCodeMemory64From32(ams::svc::Handle process_handle, uint64_t dst_address, uint64_t src_address, uint64_t size) {
        R_RETURN(UnmapProcessCodeMemory(process_handle, dst_address, src_address, size));
    }

}
