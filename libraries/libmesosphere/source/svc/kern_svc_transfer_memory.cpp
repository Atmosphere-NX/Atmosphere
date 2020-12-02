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

        constexpr bool IsValidTransferMemoryPermission(ams::svc::MemoryPermission perm) {
            switch (perm) {
                case ams::svc::MemoryPermission_None:
                case ams::svc::MemoryPermission_Read:
                case ams::svc::MemoryPermission_ReadWrite:
                    return true;
                default:
                    return false;
            }
        }

        Result MapTransferMemory(ams::svc::Handle trmem_handle, uintptr_t address, size_t size, ams::svc::MemoryPermission map_perm) {
            /* Validate the address/size. */
            R_UNLESS(util::IsAligned(address, PageSize), svc::ResultInvalidAddress());
            R_UNLESS(util::IsAligned(size,    PageSize), svc::ResultInvalidSize());
            R_UNLESS(size > 0,                           svc::ResultInvalidSize());
            R_UNLESS((address < address + size),         svc::ResultInvalidCurrentMemory());

            /* Validate the permission. */
            R_UNLESS(IsValidTransferMemoryPermission(map_perm), svc::ResultInvalidState());

            /* Get the transfer memory. */
            KScopedAutoObject trmem = GetCurrentProcess().GetHandleTable().GetObject<KTransferMemory>(trmem_handle);
            R_UNLESS(trmem.IsNotNull(), svc::ResultInvalidHandle());

            /* Verify that the mapping is in range. */
            R_UNLESS(GetCurrentProcess().GetPageTable().CanContain(address, size, KMemoryState_Transfered), svc::ResultInvalidMemoryRegion());

            /* Map the transfer memory. */
            R_TRY(trmem->Map(address, size, map_perm));

            /* We succeeded. */
            return ResultSuccess();
        }

        Result UnmapTransferMemory(ams::svc::Handle trmem_handle, uintptr_t address, size_t size) {
            /* Validate the address/size. */
            R_UNLESS(util::IsAligned(address, PageSize), svc::ResultInvalidAddress());
            R_UNLESS(util::IsAligned(size,    PageSize), svc::ResultInvalidSize());
            R_UNLESS(size > 0,                           svc::ResultInvalidSize());
            R_UNLESS((address < address + size),         svc::ResultInvalidCurrentMemory());

            /* Get the transfer memory. */
            KScopedAutoObject trmem = GetCurrentProcess().GetHandleTable().GetObject<KTransferMemory>(trmem_handle);
            R_UNLESS(trmem.IsNotNull(), svc::ResultInvalidHandle());

            /* Verify that the mapping is in range. */
            R_UNLESS(GetCurrentProcess().GetPageTable().CanContain(address, size, KMemoryState_Transfered), svc::ResultInvalidMemoryRegion());

            /* Unmap the transfer memory. */
            R_TRY(trmem->Unmap(address, size));

            return ResultSuccess();
        }

        Result CreateTransferMemory(ams::svc::Handle *out, uintptr_t address, size_t size, ams::svc::MemoryPermission map_perm) {
            /* Validate the size. */
            R_UNLESS(util::IsAligned(address, PageSize), svc::ResultInvalidAddress());
            R_UNLESS(util::IsAligned(size,    PageSize), svc::ResultInvalidSize());
            R_UNLESS(size > 0,                           svc::ResultInvalidSize());
            R_UNLESS((address < address + size),         svc::ResultInvalidCurrentMemory());

            /* Validate the permissions. */
            R_UNLESS(IsValidTransferMemoryPermission(map_perm), svc::ResultInvalidNewMemoryPermission());

            /* Get the current process and handle table. */
            auto &process      = GetCurrentProcess();
            auto &handle_table = process.GetHandleTable();

            /* Reserve a new transfer memory from the process resource limit. */
            KScopedResourceReservation trmem_reservation(std::addressof(process), ams::svc::LimitableResource_TransferMemoryCountMax);
            R_UNLESS(trmem_reservation.Succeeded(), svc::ResultLimitReached());

            /* Create the transfer memory. */
            KTransferMemory *trmem = KTransferMemory::Create();
            R_UNLESS(trmem != nullptr, svc::ResultOutOfResource());

            /* Ensure the only reference is in the handle table when we're done. */
            ON_SCOPE_EXIT { trmem->Close(); };

            /* Ensure that the region is in range. */
            R_UNLESS(process.GetPageTable().Contains(address, size), svc::ResultInvalidCurrentMemory());

            /* Initialize the transfer memory. */
            R_TRY(trmem->Initialize(address, size, map_perm));

            /* Commit the reservation. */
            trmem_reservation.Commit();

            /* Register the transfer memory. */
            KTransferMemory::Register(trmem);

            /* Add the transfer memory to the handle table. */
            R_TRY(handle_table.Add(out, trmem));

            return ResultSuccess();
        }

    }

    /* =============================    64 ABI    ============================= */

    Result MapTransferMemory64(ams::svc::Handle trmem_handle, ams::svc::Address address, ams::svc::Size size, ams::svc::MemoryPermission owner_perm) {
        return MapTransferMemory(trmem_handle, address, size, owner_perm);
    }

    Result UnmapTransferMemory64(ams::svc::Handle trmem_handle, ams::svc::Address address, ams::svc::Size size) {
        return UnmapTransferMemory(trmem_handle, address, size);
    }

    Result CreateTransferMemory64(ams::svc::Handle *out_handle, ams::svc::Address address, ams::svc::Size size, ams::svc::MemoryPermission map_perm) {
        return CreateTransferMemory(out_handle, address, size, map_perm);
    }

    /* ============================= 64From32 ABI ============================= */

    Result MapTransferMemory64From32(ams::svc::Handle trmem_handle, ams::svc::Address address, ams::svc::Size size, ams::svc::MemoryPermission owner_perm) {
        return MapTransferMemory(trmem_handle, address, size, owner_perm);
    }

    Result UnmapTransferMemory64From32(ams::svc::Handle trmem_handle, ams::svc::Address address, ams::svc::Size size) {
        return UnmapTransferMemory(trmem_handle, address, size);
    }

    Result CreateTransferMemory64From32(ams::svc::Handle *out_handle, ams::svc::Address address, ams::svc::Size size, ams::svc::MemoryPermission map_perm) {
        return CreateTransferMemory(out_handle, address, size, map_perm);
    }

}
