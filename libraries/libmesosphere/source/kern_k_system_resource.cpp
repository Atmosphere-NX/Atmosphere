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

    Result KSecureSystemResource::Initialize(size_t size, KResourceLimit *resource_limit, KMemoryManager::Pool pool) {
        /* Set members. */
        m_resource_limit = resource_limit;
        m_resource_size  = size;
        m_resource_pool  = pool;

        /* Determine required size for our secure resource. */
        const size_t secure_size = this->CalculateRequiredSecureMemorySize();

        /* Reserve memory for our secure resource. */
        KScopedResourceReservation memory_reservation(m_resource_limit, ams::svc::LimitableResource_PhysicalMemoryMax, secure_size);
        R_UNLESS(memory_reservation.Succeeded(), svc::ResultLimitReached());

        /* Allocate secure memory. */
        R_TRY(KSystemControl::AllocateSecureMemory(std::addressof(m_resource_address), m_resource_size, m_resource_pool));
        MESOSPHERE_ASSERT(m_resource_address != Null<KVirtualAddress>);

        /* Ensure we clean up the secure memory, if we fail past this point. */
        ON_RESULT_FAILURE { KSystemControl::FreeSecureMemory(m_resource_address, m_resource_size, m_resource_pool); };

        /* Check that our allocation is bigger than the reference counts needed for it. */
        const size_t rc_size = util::AlignUp(KPageTableSlabHeap::CalculateReferenceCountSize(m_resource_size), PageSize);
        R_UNLESS(m_resource_size > rc_size, svc::ResultOutOfMemory());

        /* Initialize slab heaps. */
        m_dynamic_page_manager.Initialize(m_resource_address + rc_size, m_resource_size - rc_size, PageSize);
        m_page_table_heap.Initialize(std::addressof(m_dynamic_page_manager), 0, GetPointer<KPageTableManager::RefCount>(m_resource_address));
        m_memory_block_heap.Initialize(std::addressof(m_dynamic_page_manager), 0);
        m_block_info_heap.Initialize(std::addressof(m_dynamic_page_manager), 0);

        /* Initialize managers. */
        m_page_table_manager.Initialize(std::addressof(m_dynamic_page_manager), std::addressof(m_page_table_heap));
        m_memory_block_slab_manager.Initialize(std::addressof(m_dynamic_page_manager), std::addressof(m_memory_block_heap));
        m_block_info_manager.Initialize(std::addressof(m_dynamic_page_manager), std::addressof(m_block_info_heap));

        /* Set our managers. */
        this->SetManagers(m_memory_block_slab_manager, m_block_info_manager, m_page_table_manager);

        /* Commit the memory reservation. */
        memory_reservation.Commit();

        /* Open reference to our resource limit. */
        if (m_resource_limit != nullptr) {
            m_resource_limit->Open();
        }

        /* Set ourselves as initialized. */
        m_is_initialized = true;

        R_SUCCEED();
    }

    void KSecureSystemResource::Finalize() {
        /* Check that we have no outstanding allocations. */
        MESOSPHERE_ABORT_UNLESS(m_memory_block_slab_manager.GetUsed() == 0);
        MESOSPHERE_ABORT_UNLESS(m_block_info_manager.GetUsed()        == 0);
        MESOSPHERE_ABORT_UNLESS(m_page_table_manager.GetUsed()        == 0);

        /* Free our secure memory. */
        KSystemControl::FreeSecureMemory(m_resource_address, m_resource_size, m_resource_pool);

        /* Clean up our resource usage. */
        if (m_resource_limit != nullptr) {
            /* Release the memory reservation. */
            m_resource_limit->Release(ams::svc::LimitableResource_PhysicalMemoryMax, this->CalculateRequiredSecureMemorySize());

            /* Close reference to our resource limit. */
            m_resource_limit->Close();
        }
    }

    size_t KSecureSystemResource::CalculateRequiredSecureMemorySize(size_t size, KMemoryManager::Pool pool) {
        return KSystemControl::CalculateRequiredSecureMemorySize(size, pool);
    }

}
