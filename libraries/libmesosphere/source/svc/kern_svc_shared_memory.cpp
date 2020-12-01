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

        constexpr bool IsValidSharedMemoryPermission(ams::svc::MemoryPermission perm) {
            switch (perm) {
                case ams::svc::MemoryPermission_Read:
                case ams::svc::MemoryPermission_ReadWrite:
                    return true;
                default:
                    return false;
            }
        }

        constexpr bool IsValidRemoteSharedMemoryPermission(ams::svc::MemoryPermission perm) {
            return IsValidSharedMemoryPermission(perm) || perm == ams::svc::MemoryPermission_DontCare;
        }

        Result MapSharedMemory(ams::svc::Handle shmem_handle, uintptr_t address, size_t size, ams::svc::MemoryPermission map_perm) {
            /* Validate the address/size. */
            R_UNLESS(util::IsAligned(address, PageSize), svc::ResultInvalidAddress());
            R_UNLESS(util::IsAligned(size, PageSize),    svc::ResultInvalidSize());
            R_UNLESS(size > 0,                           svc::ResultInvalidSize());
            R_UNLESS((address < address + size),         svc::ResultInvalidCurrentMemory());

            /* Validate the permission. */
            R_UNLESS(IsValidSharedMemoryPermission(map_perm), svc::ResultInvalidNewMemoryPermission());

            /* Get the current process. */
            auto &process    = GetCurrentProcess();
            auto &page_table = process.GetPageTable();

            /* Get the shared memory. */
            KScopedAutoObject shmem = process.GetHandleTable().GetObject<KSharedMemory>(shmem_handle);
            R_UNLESS(shmem.IsNotNull(), svc::ResultInvalidHandle());

            /* Verify that the mapping is in range. */
            R_UNLESS(page_table.CanContain(address, size, KMemoryState_Shared), svc::ResultInvalidMemoryRegion());

            /* Add the shared memory to the process. */
            R_TRY(process.AddSharedMemory(shmem.GetPointerUnsafe(), address, size));

            /* Ensure that we clean up the shared memory if we fail to map it. */
            auto guard = SCOPE_GUARD { process.RemoveSharedMemory(shmem.GetPointerUnsafe(), address, size); };

            /* Map the shared memory. */
            R_TRY(shmem->Map(std::addressof(page_table), address, size, std::addressof(process), map_perm));

            /* We succeeded. */
            guard.Cancel();
            return ResultSuccess();
        }

        Result UnmapSharedMemory(ams::svc::Handle shmem_handle, uintptr_t address, size_t size) {
            /* Validate the address/size. */
            R_UNLESS(util::IsAligned(address, PageSize), svc::ResultInvalidAddress());
            R_UNLESS(util::IsAligned(size, PageSize),    svc::ResultInvalidSize());
            R_UNLESS(size > 0,                           svc::ResultInvalidSize());
            R_UNLESS((address < address + size),         svc::ResultInvalidCurrentMemory());

            /* Get the current process. */
            auto &process    = GetCurrentProcess();
            auto &page_table = process.GetPageTable();

            /* Get the shared memory. */
            KScopedAutoObject shmem = process.GetHandleTable().GetObject<KSharedMemory>(shmem_handle);
            R_UNLESS(shmem.IsNotNull(), svc::ResultInvalidHandle());

            /* Verify that the mapping is in range. */
            R_UNLESS(page_table.CanContain(address, size, KMemoryState_Shared), svc::ResultInvalidMemoryRegion());

            /* Unmap the shared memory. */
            R_TRY(shmem->Unmap(std::addressof(page_table), address, size, std::addressof(process)));

            /* Remove the shared memory from the process. */
            process.RemoveSharedMemory(shmem.GetPointerUnsafe(), address, size);

            return ResultSuccess();
        }

        Result CreateSharedMemory(ams::svc::Handle *out, size_t size, ams::svc::MemoryPermission owner_perm, ams::svc::MemoryPermission remote_perm) {
            /* Validate the size. */
            R_UNLESS(0 < size && size < kern::MainMemorySize, svc::ResultInvalidSize());
            R_UNLESS(util::IsAligned(size, PageSize),         svc::ResultInvalidSize());

            /* Validate the permissions. */
            R_UNLESS(IsValidSharedMemoryPermission(owner_perm),        svc::ResultInvalidNewMemoryPermission());
            R_UNLESS(IsValidRemoteSharedMemoryPermission(remote_perm), svc::ResultInvalidNewMemoryPermission());

            /* Create the shared memory. */
            KSharedMemory *shmem = KSharedMemory::Create();
            R_UNLESS(shmem != nullptr, svc::ResultOutOfResource());

            /* Ensure the only reference is in the handle table when we're done. */
            ON_SCOPE_EXIT { shmem->Close(); };

            /* Initialize the shared memory. */
            R_TRY(shmem->Initialize(GetCurrentProcessPointer(), size, owner_perm, remote_perm));

            /* Register the shared memory. */
            KSharedMemory::Register(shmem);

            /* Add the shared memory to the handle table. */
            R_TRY(GetCurrentProcess().GetHandleTable().Add(out, shmem));

            return ResultSuccess();
        }

    }

    /* =============================    64 ABI    ============================= */

    Result MapSharedMemory64(ams::svc::Handle shmem_handle, ams::svc::Address address, ams::svc::Size size, ams::svc::MemoryPermission map_perm) {
        return MapSharedMemory(shmem_handle, address, size, map_perm);
    }

    Result UnmapSharedMemory64(ams::svc::Handle shmem_handle, ams::svc::Address address, ams::svc::Size size) {
        return UnmapSharedMemory(shmem_handle, address, size);
    }

    Result CreateSharedMemory64(ams::svc::Handle *out_handle, ams::svc::Size size, ams::svc::MemoryPermission owner_perm, ams::svc::MemoryPermission remote_perm) {
        return CreateSharedMemory(out_handle, size, owner_perm, remote_perm);
    }

    /* ============================= 64From32 ABI ============================= */

    Result MapSharedMemory64From32(ams::svc::Handle shmem_handle, ams::svc::Address address, ams::svc::Size size, ams::svc::MemoryPermission map_perm) {
        return MapSharedMemory(shmem_handle, address, size, map_perm);
    }

    Result UnmapSharedMemory64From32(ams::svc::Handle shmem_handle, ams::svc::Address address, ams::svc::Size size) {
        return UnmapSharedMemory(shmem_handle, address, size);
    }

    Result CreateSharedMemory64From32(ams::svc::Handle *out_handle, ams::svc::Size size, ams::svc::MemoryPermission owner_perm, ams::svc::MemoryPermission remote_perm) {
        return CreateSharedMemory(out_handle, size, owner_perm, remote_perm);
    }

}
