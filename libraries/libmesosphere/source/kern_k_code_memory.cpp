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
        this->owner = GetCurrentProcessPointer();

        /* Initialize the page group. */
        auto &page_table = this->owner->GetPageTable();
        new (GetPointer(this->page_group)) KPageGroup(page_table.GetBlockInfoManager());

        /* Ensure that our page group's state is valid on exit. */
        auto pg_guard = SCOPE_GUARD { GetReference(this->page_group).~KPageGroup(); };

        /* Lock the memory. */
        R_TRY(page_table.LockForCodeMemory(GetPointer(this->page_group), addr, size));

        /* Clear the memory. */
        for (const auto &block : GetReference(this->page_group)) {
            /* Clear and store cache. */
            std::memset(GetVoidPointer(block.GetAddress()), 0xFF, block.GetSize());
            cpu::StoreDataCache(GetVoidPointer(block.GetAddress()), block.GetSize());
        }

        /* Set remaining tracking members. */
        this->owner->Open();
        this->address         = addr;
        this->is_initialized  = true;
        this->is_owner_mapped = false;
        this->is_mapped       = false;

        /* We succeeded. */
        pg_guard.Cancel();
        return ResultSuccess();
    }

    void KCodeMemory::Finalize() {
        MESOSPHERE_ASSERT_THIS();

        /* Unlock. */
        if (!this->is_mapped && !this->is_owner_mapped) {
            const size_t size = GetReference(this->page_group).GetNumPages() * PageSize;
            MESOSPHERE_R_ABORT_UNLESS(this->owner->GetPageTable().UnlockForCodeMemory(this->address, size, GetReference(this->page_group)));
        }

        /* Close the page group. */
        GetReference(this->page_group).Close();
        GetReference(this->page_group).Finalize();

        /* Close our reference to our owner. */
        this->owner->Close();

        /* Perform inherited finalization. */
        KAutoObjectWithSlabHeapAndContainer<KCodeMemory, KAutoObjectWithList>::Finalize();
    }

    Result KCodeMemory::Map(KProcessAddress address, size_t size) {
        MESOSPHERE_ASSERT_THIS();

        /* Validate the size. */
        R_UNLESS(GetReference(this->page_group).GetNumPages() == util::DivideUp(size, PageSize), svc::ResultInvalidSize());

        /* Lock ourselves. */
        KScopedLightLock lk(this->lock);

        /* Ensure we're not already mapped. */
        R_UNLESS(!this->is_mapped, svc::ResultInvalidState());

        /* Map the memory. */
        R_TRY(GetCurrentProcess().GetPageTable().MapPageGroup(address, GetReference(this->page_group), KMemoryState_CodeOut, KMemoryPermission_UserReadWrite));

        /* Mark ourselves as mapped. */
        this->is_mapped = true;

        return ResultSuccess();
    }

    Result KCodeMemory::Unmap(KProcessAddress address, size_t size) {
        MESOSPHERE_ASSERT_THIS();

        /* Validate the size. */
        R_UNLESS(GetReference(this->page_group).GetNumPages() == util::DivideUp(size, PageSize), svc::ResultInvalidSize());

        /* Lock ourselves. */
        KScopedLightLock lk(this->lock);

        /* Unmap the memory. */
        R_TRY(GetCurrentProcess().GetPageTable().UnmapPageGroup(address, GetReference(this->page_group), KMemoryState_CodeOut));

        /* Mark ourselves as unmapped. */
        MESOSPHERE_ASSERT(this->is_mapped);
        this->is_mapped = false;

        return ResultSuccess();
    }

    Result KCodeMemory::MapToOwner(KProcessAddress address, size_t size, ams::svc::MemoryPermission perm) {
        MESOSPHERE_ASSERT_THIS();

        /* Validate the size. */
        R_UNLESS(GetReference(this->page_group).GetNumPages() == util::DivideUp(size, PageSize), svc::ResultInvalidSize());

        /* Lock ourselves. */
        KScopedLightLock lk(this->lock);

        /* Ensure we're not already mapped. */
        R_UNLESS(!this->is_owner_mapped, svc::ResultInvalidState());

        /* Convert the memory permission. */
        KMemoryPermission k_perm;
        switch (perm) {
            case ams::svc::MemoryPermission_Read:        k_perm = KMemoryPermission_UserRead;        break;
            case ams::svc::MemoryPermission_ReadExecute: k_perm = KMemoryPermission_UserReadExecute; break;
            MESOSPHERE_UNREACHABLE_DEFAULT_CASE();
        }

        /* Map the memory. */
        R_TRY(this->owner->GetPageTable().MapPageGroup(address, GetReference(this->page_group), KMemoryState_GeneratedCode, k_perm));

        /* Mark ourselves as mapped. */
        this->is_owner_mapped = true;

        return ResultSuccess();
    }

    Result KCodeMemory::UnmapFromOwner(KProcessAddress address, size_t size) {
        MESOSPHERE_ASSERT_THIS();

        /* Validate the size. */
        R_UNLESS(GetReference(this->page_group).GetNumPages() == util::DivideUp(size, PageSize), svc::ResultInvalidSize());

        /* Lock ourselves. */
        KScopedLightLock lk(this->lock);

        /* Unmap the memory. */
        R_TRY(this->owner->GetPageTable().UnmapPageGroup(address, GetReference(this->page_group), KMemoryState_GeneratedCode));

        /* Mark ourselves as unmapped. */
        MESOSPHERE_ASSERT(this->is_owner_mapped);
        this->is_owner_mapped = false;

        return ResultSuccess();
    }

}
