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

    Result KCodeMemory::Initialize(KProcessAddress addr, size_t size) {
        MESOSPHERE_ASSERT_THIS();

        /* Set members. */
        m_owner = GetCurrentProcessPointer();

        /* Initialize the page group. */
        auto &page_table = m_owner->GetPageTable();
        new (GetPointer(m_page_group)) KPageGroup(page_table.GetBlockInfoManager());

        /* Ensure that our page group's state is valid on exit. */
        auto pg_guard = SCOPE_GUARD { GetReference(m_page_group).~KPageGroup(); };

        /* Lock the memory. */
        R_TRY(page_table.LockForCodeMemory(GetPointer(m_page_group), addr, size));

        /* Clear the memory. */
        for (const auto &block : GetReference(m_page_group)) {
            /* Clear and store cache. */
            std::memset(GetVoidPointer(block.GetAddress()), 0xFF, block.GetSize());
            cpu::StoreDataCache(GetVoidPointer(block.GetAddress()), block.GetSize());
        }

        /* Set remaining tracking members. */
        m_owner->Open();
        m_address         = addr;
        m_is_initialized  = true;
        m_is_owner_mapped = false;
        m_is_mapped       = false;

        /* We succeeded. */
        pg_guard.Cancel();
        return ResultSuccess();
    }

    void KCodeMemory::Finalize() {
        MESOSPHERE_ASSERT_THIS();

        /* Unlock. */
        if (!m_is_mapped && !m_is_owner_mapped) {
            const size_t size = GetReference(m_page_group).GetNumPages() * PageSize;
            MESOSPHERE_R_ABORT_UNLESS(m_owner->GetPageTable().UnlockForCodeMemory(m_address, size, GetReference(m_page_group)));
        }

        /* Close the page group. */
        GetReference(m_page_group).Close();
        GetReference(m_page_group).Finalize();

        /* Close our reference to our owner. */
        m_owner->Close();

        /* Perform inherited finalization. */
        KAutoObjectWithSlabHeapAndContainer<KCodeMemory, KAutoObjectWithList>::Finalize();
    }

    Result KCodeMemory::Map(KProcessAddress address, size_t size) {
        MESOSPHERE_ASSERT_THIS();

        /* Validate the size. */
        R_UNLESS(GetReference(m_page_group).GetNumPages() == util::DivideUp(size, PageSize), svc::ResultInvalidSize());

        /* Lock ourselves. */
        KScopedLightLock lk(m_lock);

        /* Ensure we're not already mapped. */
        R_UNLESS(!m_is_mapped, svc::ResultInvalidState());

        /* Map the memory. */
        R_TRY(GetCurrentProcess().GetPageTable().MapPageGroup(address, GetReference(m_page_group), KMemoryState_CodeOut, KMemoryPermission_UserReadWrite));

        /* Mark ourselves as mapped. */
        m_is_mapped = true;

        return ResultSuccess();
    }

    Result KCodeMemory::Unmap(KProcessAddress address, size_t size) {
        MESOSPHERE_ASSERT_THIS();

        /* Validate the size. */
        R_UNLESS(GetReference(m_page_group).GetNumPages() == util::DivideUp(size, PageSize), svc::ResultInvalidSize());

        /* Lock ourselves. */
        KScopedLightLock lk(m_lock);

        /* Unmap the memory. */
        R_TRY(GetCurrentProcess().GetPageTable().UnmapPageGroup(address, GetReference(m_page_group), KMemoryState_CodeOut));

        /* Mark ourselves as unmapped. */
        MESOSPHERE_ASSERT(m_is_mapped);
        m_is_mapped = false;

        return ResultSuccess();
    }

    Result KCodeMemory::MapToOwner(KProcessAddress address, size_t size, ams::svc::MemoryPermission perm) {
        MESOSPHERE_ASSERT_THIS();

        /* Validate the size. */
        R_UNLESS(GetReference(m_page_group).GetNumPages() == util::DivideUp(size, PageSize), svc::ResultInvalidSize());

        /* Lock ourselves. */
        KScopedLightLock lk(m_lock);

        /* Ensure we're not already mapped. */
        R_UNLESS(!m_is_owner_mapped, svc::ResultInvalidState());

        /* Convert the memory permission. */
        KMemoryPermission k_perm;
        switch (perm) {
            case ams::svc::MemoryPermission_Read:        k_perm = KMemoryPermission_UserRead;        break;
            case ams::svc::MemoryPermission_ReadExecute: k_perm = KMemoryPermission_UserReadExecute; break;
            MESOSPHERE_UNREACHABLE_DEFAULT_CASE();
        }

        /* Map the memory. */
        R_TRY(m_owner->GetPageTable().MapPageGroup(address, GetReference(m_page_group), KMemoryState_GeneratedCode, k_perm));

        /* Mark ourselves as mapped. */
        m_is_owner_mapped = true;

        return ResultSuccess();
    }

    Result KCodeMemory::UnmapFromOwner(KProcessAddress address, size_t size) {
        MESOSPHERE_ASSERT_THIS();

        /* Validate the size. */
        R_UNLESS(GetReference(m_page_group).GetNumPages() == util::DivideUp(size, PageSize), svc::ResultInvalidSize());

        /* Lock ourselves. */
        KScopedLightLock lk(m_lock);

        /* Unmap the memory. */
        R_TRY(m_owner->GetPageTable().UnmapPageGroup(address, GetReference(m_page_group), KMemoryState_GeneratedCode));

        /* Mark ourselves as unmapped. */
        MESOSPHERE_ASSERT(m_is_owner_mapped);
        m_is_owner_mapped = false;

        return ResultSuccess();
    }

}
