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

    Result KSharedMemory::Initialize(KProcess *owner, size_t size, ams::svc::MemoryPermission own_perm, ams::svc::MemoryPermission rem_perm) {
        MESOSPHERE_ASSERT_THIS();

        /* Set members. */
        m_owner_process_id = owner->GetId();
        m_owner_perm       = own_perm;
        m_remote_perm      = rem_perm;

        /* Get the number of pages. */
        const size_t num_pages = util::DivideUp(size, PageSize);
        MESOSPHERE_ASSERT(num_pages > 0);

        /* Get the resource limit. */
        KResourceLimit *reslimit = owner->GetResourceLimit();

        /* Reserve memory for ourselves. */
        KScopedResourceReservation memory_reservation(reslimit, ams::svc::LimitableResource_PhysicalMemoryMax, size);
        R_UNLESS(memory_reservation.Succeeded(), svc::ResultLimitReached());

        /* Allocate the memory. */
        R_TRY(Kernel::GetMemoryManager().AllocateAndOpen(std::addressof(m_page_group), num_pages, owner->GetAllocateOption()));

        /* Commit our reservation. */
        memory_reservation.Commit();

        /* Set our resource limit. */
        m_resource_limit = reslimit;
        m_resource_limit->Open();

        /* Mark initialized. */
        m_is_initialized = true;

        /* Clear all pages in the memory. */
        for (const auto &block : m_page_group) {
            std::memset(GetVoidPointer(block.GetAddress()), 0, block.GetSize());
        }

        return ResultSuccess();
    }

    void KSharedMemory::Finalize() {
        MESOSPHERE_ASSERT_THIS();

        /* Get the number of pages. */
        const size_t num_pages = m_page_group.GetNumPages();
        const size_t size      = num_pages * PageSize;

        /* Close and finalize the page group. */
        m_page_group.Close();
        m_page_group.Finalize();

        /* Release the memory reservation. */
        m_resource_limit->Release(ams::svc::LimitableResource_PhysicalMemoryMax, size);
        m_resource_limit->Close();

        /* Perform inherited finalization. */
        KAutoObjectWithSlabHeapAndContainer<KSharedMemory, KAutoObjectWithList>::Finalize();
    }

    Result KSharedMemory::Map(KProcessPageTable *table, KProcessAddress address, size_t size, KProcess *process, ams::svc::MemoryPermission map_perm) {
        MESOSPHERE_ASSERT_THIS();

        /* Validate the size. */
        R_UNLESS(m_page_group.GetNumPages() == util::DivideUp(size, PageSize), svc::ResultInvalidSize());

        /* Validate the permission. */
        const ams::svc::MemoryPermission test_perm = (process->GetId() == m_owner_process_id) ? m_owner_perm : m_remote_perm;
        if (test_perm == ams::svc::MemoryPermission_DontCare) {
            MESOSPHERE_ASSERT(map_perm == ams::svc::MemoryPermission_Read || map_perm == ams::svc::MemoryPermission_ReadWrite);
        } else {
            R_UNLESS(map_perm == test_perm, svc::ResultInvalidNewMemoryPermission());
        }

        /* Map the memory. */
        return table->MapPageGroup(address, m_page_group, KMemoryState_Shared, ConvertToKMemoryPermission(map_perm));
    }

    Result KSharedMemory::Unmap(KProcessPageTable *table, KProcessAddress address, size_t size, KProcess *process) {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_UNUSED(process);

        /* Validate the size. */
        R_UNLESS(m_page_group.GetNumPages() == util::DivideUp(size, PageSize), svc::ResultInvalidSize());

        /* Unmap the memory. */
        return table->UnmapPageGroup(address, m_page_group, KMemoryState_Shared);
    }

}
