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

namespace ams::kern {

    /* Static initializer. */
    void KDeviceAddressSpace::Initialize() {
        /* This just forwards to the device page table manager. */
        KDevicePageTable::Initialize();
    }

    /* Member functions. */
    Result KDeviceAddressSpace::Initialize(u64 address, u64 size) {
        MESOSPHERE_ASSERT_THIS();

        /* Initialize the device page table. */
        R_TRY(this->table.Initialize(address, size));

        /* Set member variables. */
        this->space_address  = address;
        this->space_size     = size;
        this->is_initialized = true;

        return ResultSuccess();
    }

    void KDeviceAddressSpace::Finalize() {
        MESOSPHERE_ASSERT_THIS();

        /* Finalize the table. */
        this->table.Finalize();

        /* Finalize base. */
        KAutoObjectWithSlabHeapAndContainer<KDeviceAddressSpace, KAutoObjectWithList>::Finalize();
    }

    Result KDeviceAddressSpace::Attach(ams::svc::DeviceName device_name) {
        /* Lock the address space. */
        KScopedLightLock lk(this->lock);

        /* Attach. */
        return this->table.Attach(device_name, this->space_address, this->space_size);
    }

    Result KDeviceAddressSpace::Detach(ams::svc::DeviceName device_name) {
        /* Lock the address space. */
        KScopedLightLock lk(this->lock);

        /* Detach. */
        return this->table.Detach(device_name);
    }

    Result KDeviceAddressSpace::Map(size_t *out_mapped_size, KProcessPageTable *page_table, KProcessAddress process_address, size_t size, u64 device_address, ams::svc::MemoryPermission device_perm, bool is_aligned, bool refresh_mappings) {
        /* Check that the address falls within the space. */
        R_UNLESS((this->space_address <= device_address && device_address + size - 1 <= this->space_address + this->space_size - 1), svc::ResultInvalidCurrentMemory());

        /* Lock the address space. */
        KScopedLightLock lk(this->lock);

        /* Lock the pages. */
        KPageGroup pg(page_table->GetBlockInfoManager());
        R_TRY(page_table->LockForDeviceAddressSpace(std::addressof(pg), process_address, size, ConvertToKMemoryPermission(device_perm), is_aligned));

        /* Close the pages we opened when we're done with them. */
        ON_SCOPE_EXIT { pg.Close(); };

        /* Ensure that if we fail, we don't keep unmapped pages locked. */
        ON_SCOPE_EXIT {
            if (*out_mapped_size != size) {
                page_table->UnlockForDeviceAddressSpace(process_address + *out_mapped_size, size - *out_mapped_size);
            };
        };

        /* Map the pages. */
        {
            /* Clear the output size to zero on failure. */
            auto map_guard = SCOPE_GUARD { *out_mapped_size = 0; };

            /* Perform the mapping. */
            R_TRY(this->table.Map(out_mapped_size, pg, device_address, device_perm, refresh_mappings));

            /* We succeeded, so cancel our guard. */
            map_guard.Cancel();
        }


        return ResultSuccess();
    }

    Result KDeviceAddressSpace::Unmap(KProcessPageTable *page_table, KProcessAddress process_address, size_t size, u64 device_address) {
        /* Check that the address falls within the space. */
        R_UNLESS((this->space_address <= device_address && device_address + size - 1 <= this->space_address + this->space_size - 1), svc::ResultInvalidCurrentMemory());

        /* Lock the address space. */
        KScopedLightLock lk(this->lock);

        /* Make and open a page group for the unmapped region. */
        KPageGroup pg(page_table->GetBlockInfoManager());
        R_TRY(page_table->MakeAndOpenPageGroupContiguous(std::addressof(pg), process_address, size / PageSize,
                                                         KMemoryState_FlagCanDeviceMap, KMemoryState_FlagCanDeviceMap,
                                                         KMemoryPermission_None, KMemoryPermission_None,
                                                         KMemoryAttribute_AnyLocked | KMemoryAttribute_DeviceShared | KMemoryAttribute_Locked, KMemoryAttribute_DeviceShared));

        /* Ensure the page group is closed on scope exit. */
        ON_SCOPE_EXIT { pg.Close(); };

        /* Unmap. */
        R_TRY(this->table.Unmap(pg, device_address));

        /* Unlock the pages. */
        R_TRY(page_table->UnlockForDeviceAddressSpace(process_address, size));

        return ResultSuccess();
    }

}
