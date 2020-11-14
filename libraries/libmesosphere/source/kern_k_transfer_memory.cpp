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

    Result KTransferMemory::Initialize(KProcessAddress addr, size_t size, ams::svc::MemoryPermission own_perm) {
        MESOSPHERE_ASSERT_THIS();

        /* Set members. */
        this->owner = GetCurrentProcessPointer();

        /* Initialize the page group. */
        auto &page_table = this->owner->GetPageTable();
        new (GetPointer(this->page_group)) KPageGroup(page_table.GetBlockInfoManager());

        /* Ensure that our page group's state is valid on exit. */
        auto pg_guard = SCOPE_GUARD { GetReference(this->page_group).~KPageGroup(); };

        /* Lock the memory. */
        R_TRY(page_table.LockForTransferMemory(GetPointer(this->page_group), addr, size, ConvertToKMemoryPermission(own_perm)));

        /* Set remaining tracking members. */
        this->owner->Open();
        this->owner_perm     = own_perm;
        this->address        = addr;
        this->is_initialized = true;
        this->is_mapped      = false;

        /* We succeeded. */
        pg_guard.Cancel();
        return ResultSuccess();
    }

    void KTransferMemory::Finalize() {
        MESOSPHERE_ASSERT_THIS();

        /* Unlock. */
        if (!this->is_mapped) {
            const size_t size = GetReference(this->page_group).GetNumPages() * PageSize;
            MESOSPHERE_R_ABORT_UNLESS(this->owner->GetPageTable().UnlockForTransferMemory(this->address, size, GetReference(this->page_group)));
        }

        /* Close the page group. */
        GetReference(this->page_group).Close();
        GetReference(this->page_group).Finalize();

        /* Perform inherited finalization. */
        KAutoObjectWithSlabHeapAndContainer<KTransferMemory, KAutoObjectWithList>::Finalize();
    }

    void KTransferMemory::PostDestroy(uintptr_t arg) {
        KProcess *owner = reinterpret_cast<KProcess *>(arg);
        owner->ReleaseResource(ams::svc::LimitableResource_TransferMemoryCountMax, 1);
        owner->Close();
    }

    Result KTransferMemory::Map(KProcessAddress address, size_t size, ams::svc::MemoryPermission map_perm) {
        MESOSPHERE_ASSERT_THIS();

        /* Validate the size. */
        R_UNLESS(GetReference(this->page_group).GetNumPages() == util::DivideUp(size, PageSize), svc::ResultInvalidSize());

        /* Validate the permission. */
        R_UNLESS(this->owner_perm == map_perm, svc::ResultInvalidState());

        /* Lock ourselves. */
        KScopedLightLock lk(this->lock);

        /* Ensure we're not already mapped. */
        R_UNLESS(!this->is_mapped, svc::ResultInvalidState());

        /* Map the memory. */
        const KMemoryState state = (this->owner_perm == ams::svc::MemoryPermission_None) ? KMemoryState_Transfered : KMemoryState_SharedTransfered;
        R_TRY(GetCurrentProcess().GetPageTable().MapPageGroup(address, GetReference(this->page_group), state, KMemoryPermission_UserReadWrite));

        /* Mark ourselves as mapped. */
        this->is_mapped = true;

        return ResultSuccess();
    }

    Result KTransferMemory::Unmap(KProcessAddress address, size_t size) {
        MESOSPHERE_ASSERT_THIS();

        /* Validate the size. */
        R_UNLESS(GetReference(this->page_group).GetNumPages() == util::DivideUp(size, PageSize), svc::ResultInvalidSize());

        /* Lock ourselves. */
        KScopedLightLock lk(this->lock);

        /* Unmap the memory. */
        const KMemoryState state = (this->owner_perm == ams::svc::MemoryPermission_None) ? KMemoryState_Transfered : KMemoryState_SharedTransfered;
        R_TRY(GetCurrentProcess().GetPageTable().UnmapPageGroup(address, GetReference(this->page_group), state));

        /* Mark ourselves as unmapped. */
        MESOSPHERE_ASSERT(this->is_mapped);
        this->is_mapped = false;

        return ResultSuccess();
    }

}
