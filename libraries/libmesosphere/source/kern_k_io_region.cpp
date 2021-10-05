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

    Result KIoRegion::Initialize(KIoPool *pool, KPhysicalAddress phys_addr, size_t size, ams::svc::MemoryMapping mapping, ams::svc::MemoryPermission perm) {
        MESOSPHERE_ASSERT_THIS();

        /* Set fields. */
        m_physical_address = phys_addr;
        m_size             = size;
        m_mapping          = mapping;
        m_permission       = perm;
        m_pool             = pool;
        m_is_mapped        = false;

        /* Add ourselves to our pool. */
        R_TRY(m_pool->AddIoRegion(this));

        /* Open a reference to our pool. */
        m_pool->Open();

        /* Mark ourselves as initialized. */
        m_is_initialized = true;

        return ResultSuccess();
    }

    void KIoRegion::Finalize() {
        /* Remove ourselves from our pool. */
        m_pool->RemoveIoRegion(this);

        /* Close our reference to our pool. */
        m_pool->Close();
    }

    Result KIoRegion::Map(KProcessAddress address, size_t size, ams::svc::MemoryPermission map_perm) {
        MESOSPHERE_ASSERT_THIS();

        /* Check that the desired perm is allowable. */
        R_UNLESS((m_permission | map_perm) == m_permission, svc::ResultInvalidNewMemoryPermission());

        /* Check that the size is correct. */
        R_UNLESS(size == m_size, svc::ResultInvalidSize());

        /* Lock ourselves. */
        KScopedLightLock lk(m_lock);

        /* Check that we're not already mapped. */
        R_UNLESS(!m_is_mapped, svc::ResultInvalidState());

        /* Map ourselves. */
        R_TRY(GetCurrentProcess().GetPageTable().MapIoRegion(address, m_physical_address, size, m_mapping, map_perm));

        /* Add ourselves to the current process. */
        GetCurrentProcess().AddIoRegion(this);

        /* Note that we're mapped. */
        m_is_mapped = true;

        return ResultSuccess();
    }

    Result KIoRegion::Unmap(KProcessAddress address, size_t size) {
        MESOSPHERE_ASSERT_THIS();

        /* Check that the size is correct. */
        R_UNLESS(size == m_size, svc::ResultInvalidSize());

        /* Lock ourselves. */
        KScopedLightLock lk(m_lock);

        /* Unmap ourselves. */
        R_TRY(GetCurrentProcess().GetPageTable().UnmapIoRegion(address, m_physical_address, size));

        /* Remove ourselves from the current process. */
        GetCurrentProcess().RemoveIoRegion(this);

        /* Note that we're unmapped. */
        m_is_mapped = false;

        return ResultSuccess();
    }

}
