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

namespace ams::kern::arm64 {

    void KPageTable::Initialize(s32 core_id) {
        /* Nothing actually needed here. */
    }

    Result KPageTable::InitializeForKernel(void *table, KVirtualAddress start, KVirtualAddress end) {
        /* Initialize basic fields. */
        this->asid = 0;
        this->manager = std::addressof(Kernel::GetPageTableManager());

        /* Allocate a page for ttbr. */
        const u64 asid_tag = (static_cast<u64>(this->asid) << 48ul);
        const KVirtualAddress page = this->manager->Allocate();
        MESOSPHERE_ASSERT(page != Null<KVirtualAddress>);
        cpu::ClearPageToZero(GetVoidPointer(page));
        this->ttbr = GetInteger(KPageTableBase::GetLinearPhysicalAddress(page)) | asid_tag;

        /* Initialize the base page table. */
        MESOSPHERE_R_ABORT_UNLESS(KPageTableBase::InitializeForKernel(true, table, start, end));

        return ResultSuccess();
    }

    Result KPageTable::Finalize() {
        MESOSPHERE_TODO_IMPLEMENT();
    }
}
