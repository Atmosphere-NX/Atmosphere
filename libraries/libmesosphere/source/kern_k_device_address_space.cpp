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
        R_TRY(m_table.Initialize(address, size));

        /* Set member variables. */
        m_space_address  = address;
        m_space_size     = size;
        m_is_initialized = true;

        R_SUCCEED();
    }

    void KDeviceAddressSpace::Finalize() {
        MESOSPHERE_ASSERT_THIS();

        /* Finalize the table. */
        m_table.Finalize();
    }

    Result KDeviceAddressSpace::Attach(ams::svc::DeviceName device_name) {
        /* Lock the address space. */
        KScopedLightLock lk(m_lock);

        /* Attach. */
        R_RETURN(m_table.Attach(device_name, m_space_address, m_space_size));
    }

    Result KDeviceAddressSpace::Detach(ams::svc::DeviceName device_name) {
        /* Lock the address space. */
        KScopedLightLock lk(m_lock);

        /* Detach. */
        R_RETURN(m_table.Detach(device_name));
    }

    Result KDeviceAddressSpace::Map(KProcessPageTable *page_table, KProcessAddress process_address, size_t size, u64 device_address, u32 option, bool is_aligned) {
        /* Check that the address falls within the space. */
        R_UNLESS((m_space_address <= device_address && device_address + size - 1 <= m_space_address + m_space_size - 1), svc::ResultInvalidCurrentMemory());

        /* Decode the option. */
        const util::BitPack32 option_pack = { option };
        const auto device_perm = option_pack.Get<ams::svc::MapDeviceAddressSpaceOption::Permission>();
        const auto flags       = option_pack.Get<ams::svc::MapDeviceAddressSpaceOption::Flags>();
        const auto reserved    = option_pack.Get<ams::svc::MapDeviceAddressSpaceOption::Reserved>();

        /* Validate the option. */
        /* TODO: It is likely that this check for flags == none is only on NX board. */
        R_UNLESS(flags == ams::svc::MapDeviceAddressSpaceFlag_None, svc::ResultInvalidEnumValue());
        R_UNLESS(reserved == 0,                                     svc::ResultInvalidEnumValue());

        /* Lock the address space. */
        KScopedLightLock lk(m_lock);

        /* Lock the page table to prevent concurrent device mapping operations. */
        KScopedLightLock pt_lk = page_table->AcquireDeviceMapLock();

        /* Lock the pages. */
        bool is_io{};
        R_TRY(page_table->LockForMapDeviceAddressSpace(std::addressof(is_io), process_address, size, ConvertToKMemoryPermission(device_perm), is_aligned, true));

        /* Ensure that if we fail, we don't keep unmapped pages locked. */
        ON_RESULT_FAILURE { MESOSPHERE_R_ABORT_UNLESS(page_table->UnlockForDeviceAddressSpace(process_address, size)); };

        /* Check that the io status is allowable. */
        if (is_io) {
            R_UNLESS((flags & ams::svc::MapDeviceAddressSpaceFlag_NotIoRegister) == 0, svc::ResultInvalidCombination());
        }

        /* Map the pages. */
        {
            /* Perform the mapping. */
            R_TRY(m_table.Map(page_table, process_address, size, device_address, device_perm, is_aligned, is_io));

            /* Ensure that we unmap the pages if we fail to update the protections. */
            /* NOTE: Nintendo does not check the result of this unmap call. */
            ON_RESULT_FAILURE { m_table.Unmap(device_address, size); };

            /* Update the protections in accordance with how much we mapped. */
            R_TRY(page_table->UnlockForDeviceAddressSpacePartialMap(process_address, size));
        }

        /* We succeeded. */
        R_SUCCEED();
    }

    Result KDeviceAddressSpace::Unmap(KProcessPageTable *page_table, KProcessAddress process_address, size_t size, u64 device_address) {
        /* Check that the address falls within the space. */
        R_UNLESS((m_space_address <= device_address && device_address + size - 1 <= m_space_address + m_space_size - 1), svc::ResultInvalidCurrentMemory());

        /* Lock the address space. */
        KScopedLightLock lk(m_lock);

        /* Lock the page table to prevent concurrent device mapping operations. */
        KScopedLightLock pt_lk = page_table->AcquireDeviceMapLock();

        /* Lock the pages. */
        R_TRY(page_table->LockForUnmapDeviceAddressSpace(process_address, size, true));

        /* Unmap the pages. */
        {
            /* If we fail to unmap, we want to do a partial unlock. */
            ON_RESULT_FAILURE { MESOSPHERE_R_ABORT_UNLESS(page_table->UnlockForDeviceAddressSpacePartialMap(process_address, size)); };

            /* Perform the unmap. */
            R_TRY(m_table.Unmap(page_table, process_address, size, device_address));
        }

        /* Unlock the pages. */
        MESOSPHERE_R_ABORT_UNLESS(page_table->UnlockForDeviceAddressSpace(process_address, size));

        R_SUCCEED();
    }

}
