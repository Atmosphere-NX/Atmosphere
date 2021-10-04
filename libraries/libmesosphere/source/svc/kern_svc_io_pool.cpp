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

        #if defined(AMS_SVC_IO_POOL_NOT_SUPPORTED)
            constexpr bool IsIoPoolApiSupported = false;
        #else
            constexpr bool IsIoPoolApiSupported = true;
        #endif

        [[maybe_unused]] constexpr bool IsValidIoRegionMapping(ams::svc::MemoryMapping mapping) {
            switch (mapping) {
                case ams::svc::MemoryMapping_IoRegister:
                case ams::svc::MemoryMapping_Uncached:
                case ams::svc::MemoryMapping_Memory:
                    return true;
                default:
                    return false;
            }
        }

        [[maybe_unused]] constexpr bool IsValidIoRegionPermission(ams::svc::MemoryPermission perm) {
            switch (perm) {
                case ams::svc::MemoryPermission_Read:
                case ams::svc::MemoryPermission_ReadWrite:
                    return true;
                default:
                    return false;
            }
        }

        Result CreateIoPool(ams::svc::Handle *out, ams::svc::IoPoolType pool_type) {
            if constexpr (IsIoPoolApiSupported) {
                /* Validate that we're allowed to create a pool for the given type. */
                R_UNLESS(KIoPool::IsValidIoPoolType(pool_type), svc::ResultNotFound());

                /* Create the io pool. */
                KIoPool *io_pool = KIoPool::Create();
                R_UNLESS(io_pool != nullptr, svc::ResultOutOfResource());

                /* Ensure the only reference is in the handle table when we're done. */
                ON_SCOPE_EXIT { io_pool->Close(); };

                /* Initialize the io pool. */
                R_TRY(io_pool->Initialize(pool_type));

                /* Register the io pool. */
                KIoPool::Register(io_pool);

                /* Add the io pool to the handle table. */
                R_TRY(GetCurrentProcess().GetHandleTable().Add(out, io_pool));

                return ResultSuccess();
            } else {
                MESOSPHERE_UNUSED(out, pool_type);
                return svc::ResultNotImplemented();
            }
        }

        Result CreateIoRegion(ams::svc::Handle *out, ams::svc::Handle io_pool_handle, uint64_t phys_addr, size_t size, ams::svc::MemoryMapping mapping, ams::svc::MemoryPermission perm) {
            if constexpr (IsIoPoolApiSupported) {
                /* Validate the address/size. */
                R_UNLESS(size > 0,                             svc::ResultInvalidSize());
                R_UNLESS(util::IsAligned(size,      PageSize), svc::ResultInvalidSize());
                R_UNLESS(util::IsAligned(phys_addr, PageSize), svc::ResultInvalidAddress());
                R_UNLESS((phys_addr < phys_addr + size),       svc::ResultInvalidMemoryRegion());

                /* Validate the mapping/permissions. */
                R_UNLESS(IsValidIoRegionMapping(mapping), svc::ResultInvalidEnumValue());
                R_UNLESS(IsValidIoRegionPermission(perm), svc::ResultInvalidEnumValue());

                /* Get the current handle table. */
                auto &handle_table = GetCurrentProcess().GetHandleTable();

                /* Get the io pool. */
                KScopedAutoObject io_pool = handle_table.GetObject<KIoPool>(io_pool_handle);
                R_UNLESS(io_pool.IsNotNull(), svc::ResultInvalidHandle());

                /* Create the io region. */
                KIoRegion *io_region = KIoRegion::Create();
                R_UNLESS(io_region != nullptr, svc::ResultOutOfResource());

                /* Ensure the only reference is in the handle table when we're done. */
                ON_SCOPE_EXIT { io_region->Close(); };

                /* Initialize the io region. */
                R_TRY(io_region->Initialize(io_pool.GetPointerUnsafe(), phys_addr, size, mapping, perm));

                /* Register the io region. */
                KIoRegion::Register(io_region);

                /* Add the io region to the handle table. */
                R_TRY(handle_table.Add(out, io_region));

                return ResultSuccess();
            } else {
                MESOSPHERE_UNUSED(out, io_pool_handle, phys_addr, size, mapping, perm);
                return svc::ResultNotImplemented();
            }
        }

        Result MapIoRegion(ams::svc::Handle io_region_handle, uintptr_t address, size_t size, ams::svc::MemoryPermission map_perm) {
            if constexpr (IsIoPoolApiSupported) {
                /* Validate the address/size. */
                R_UNLESS(size > 0,                           svc::ResultInvalidSize());
                R_UNLESS(util::IsAligned(size,    PageSize), svc::ResultInvalidSize());
                R_UNLESS(util::IsAligned(address, PageSize), svc::ResultInvalidAddress());
                R_UNLESS((address < address + size),         svc::ResultInvalidCurrentMemory());

                /* Verify that the mapping is in range. */
                R_UNLESS(GetCurrentProcess().GetPageTable().CanContain(address, size, KMemoryState_Io), svc::ResultInvalidMemoryRegion());

                /* Validate the map permission. */
                R_UNLESS(IsValidIoRegionPermission(map_perm), svc::ResultInvalidNewMemoryPermission());

                /* Get the io region. */
                KScopedAutoObject io_region = GetCurrentProcess().GetHandleTable().GetObject<KIoRegion>(io_region_handle);
                R_UNLESS(io_region.IsNotNull(), svc::ResultInvalidHandle());

                /* Map the io region. */
                R_TRY(io_region->Map(address, size, map_perm));

                /* We succeeded. */
                return ResultSuccess();
            } else {
                MESOSPHERE_UNUSED(io_region_handle, address, size, map_perm);
                return svc::ResultNotImplemented();
            }
        }

        Result UnmapIoRegion(ams::svc::Handle io_region_handle, uintptr_t address, size_t size) {
            if constexpr (IsIoPoolApiSupported) {
                /* Validate the address/size. */
                R_UNLESS(size > 0,                           svc::ResultInvalidSize());
                R_UNLESS(util::IsAligned(size,    PageSize), svc::ResultInvalidSize());
                R_UNLESS(util::IsAligned(address, PageSize), svc::ResultInvalidAddress());
                R_UNLESS((address < address + size),         svc::ResultInvalidCurrentMemory());

                /* Verify that the mapping is in range. */
                R_UNLESS(GetCurrentProcess().GetPageTable().CanContain(address, size, KMemoryState_Io), svc::ResultInvalidMemoryRegion());

                /* Get the io region. */
                KScopedAutoObject io_region = GetCurrentProcess().GetHandleTable().GetObject<KIoRegion>(io_region_handle);
                R_UNLESS(io_region.IsNotNull(), svc::ResultInvalidHandle());

                /* Unmap the io region. */
                R_TRY(io_region->Unmap(address, size));

                /* We succeeded. */
                return ResultSuccess();
            } else {
                MESOSPHERE_UNUSED(io_region_handle, address, size);
                return svc::ResultNotImplemented();
            }
        }

    }

    /* =============================    64 ABI    ============================= */

    Result CreateIoPool64(ams::svc::Handle *out_handle, ams::svc::IoPoolType pool_type) {
        return CreateIoPool(out_handle, pool_type);
    }

    Result CreateIoRegion64(ams::svc::Handle *out_handle, ams::svc::Handle io_pool, ams::svc::PhysicalAddress physical_address, ams::svc::Size size, ams::svc::MemoryMapping mapping, ams::svc::MemoryPermission perm) {
        return CreateIoRegion(out_handle, io_pool, physical_address, size, mapping, perm);
    }

    Result MapIoRegion64(ams::svc::Handle io_region, ams::svc::Address address, ams::svc::Size size, ams::svc::MemoryPermission perm) {
        return MapIoRegion(io_region, address, size, perm);
    }

    Result UnmapIoRegion64(ams::svc::Handle io_region, ams::svc::Address address, ams::svc::Size size) {
        return UnmapIoRegion(io_region, address, size);
    }

    /* ============================= 64From32 ABI ============================= */

    Result CreateIoPool64From32(ams::svc::Handle *out_handle, ams::svc::IoPoolType pool_type) {
        return CreateIoPool(out_handle, pool_type);
    }

    Result CreateIoRegion64From32(ams::svc::Handle *out_handle, ams::svc::Handle io_pool, ams::svc::PhysicalAddress physical_address, ams::svc::Size size, ams::svc::MemoryMapping mapping, ams::svc::MemoryPermission perm) {
        return CreateIoRegion(out_handle, io_pool, physical_address, size, mapping, perm);
    }

    Result MapIoRegion64From32(ams::svc::Handle io_region, ams::svc::Address address, ams::svc::Size size, ams::svc::MemoryPermission perm) {
        return MapIoRegion(io_region, address, size, perm);
    }

    Result UnmapIoRegion64From32(ams::svc::Handle io_region, ams::svc::Address address, ams::svc::Size size) {
        return UnmapIoRegion(io_region, address, size);
    }

}
