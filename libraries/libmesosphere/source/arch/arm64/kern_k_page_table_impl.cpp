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

namespace ams::kern::arch::arm64 {

    void KPageTableImpl::InitializeForKernel(void *tb, KVirtualAddress start, KVirtualAddress end) {
        this->table       = static_cast<L1PageTableEntry *>(tb);
        this->is_kernel   = true;
        this->num_entries = util::AlignUp(end - start, L1BlockSize) / L1BlockSize;
    }

    L1PageTableEntry *KPageTableImpl::Finalize() {
        return this->table;
    }

    bool KPageTableImpl::GetPhysicalAddress(KPhysicalAddress *out, KProcessAddress address) const {
        /* Validate that we can read the actual entry. */
        const size_t l0_index = GetL0Index(address);
        const size_t l1_index = GetL1Index(address);
        if (this->is_kernel) {
            /* Kernel entries must be accessed via TTBR1. */
            if ((l0_index != MaxPageTableEntries - 1) || (l1_index < MaxPageTableEntries - this->num_entries)) {
                return false;
            }
        } else {
            /* User entries must be accessed with TTBR0. */
            if ((l0_index != 0) || l1_index >= this->num_entries) {
                return false;
            }
        }

        /* Try to get from l1 table. */
        const L1PageTableEntry *l1_entry = this->GetL1Entry(address);
        if (l1_entry->IsBlock()) {
            *out = l1_entry->GetBlock() + GetL1Offset(address);
            return true;
        } else if (!l1_entry->IsTable()) {
            return false;
        }

        /* Try to get from l2 table. */
        const L2PageTableEntry *l2_entry = this->GetL2Entry(l1_entry, address);
        if (l2_entry->IsBlock()) {
            *out = l2_entry->GetBlock() + GetL2Offset(address);
            return true;
        } else if (!l2_entry->IsTable()) {
            return false;
        }

        /* Try to get from l3 table. */
        const L3PageTableEntry *l3_entry = this->GetL3Entry(l2_entry, address);
        if (l3_entry->IsBlock()) {
            *out = l3_entry->GetBlock() + GetL3Offset(address);
            return true;
        }

        return false;
    }

}
