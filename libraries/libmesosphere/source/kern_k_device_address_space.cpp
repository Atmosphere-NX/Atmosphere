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
        MESOSPHERE_UNIMPLEMENTED();
    }

    Result KDeviceAddressSpace::Map(size_t *out_mapped_size, KProcessPageTable *page_table, KProcessAddress process_address, size_t size, u64 device_address, ams::svc::MemoryPermission device_perm, bool is_aligned, bool refresh_mappings) {
        MESOSPHERE_UNIMPLEMENTED();
    }

    Result KDeviceAddressSpace::Unmap(KProcessPageTable *page_table, KProcessAddress process_address, size_t size, u64 device_address) {
        MESOSPHERE_UNIMPLEMENTED();
    }

}
