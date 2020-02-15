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
#pragma once
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_select_cpu.hpp>
#include <mesosphere/kern_k_typed_address.hpp>
#include <mesosphere/kern_k_memory_layout.hpp>
#include <mesosphere/arch/arm64/kern_k_page_table_entry.hpp>

namespace ams::kern::arch::arm64 {

    /* TODO: This seems worse than KInitialPageTable. Can we fulfill Nintendo's API using KInitialPageTable? */
    /* KInitialPageTable is significantly nicer, but doesn't have KPageTableImpl's traversal semantics. */
    /* Perhaps we could implement those on top of it? */
    class KPageTableImpl {
        NON_COPYABLE(KPageTableImpl);
        NON_MOVEABLE(KPageTableImpl);
        private:
            static constexpr size_t PageBits  = __builtin_ctzll(PageSize);
            static constexpr size_t NumLevels = 3;
            static constexpr size_t LevelBits = 9;
            static_assert(NumLevels > 0);

            static constexpr size_t AddressBits = (NumLevels - 1) * LevelBits + PageBits;
            static_assert(AddressBits <= BITSIZEOF(u64));
            static constexpr size_t AddressSpaceSize = (1ull << AddressBits);
        private:
            L1PageTableEntry *table;
            bool is_kernel;
            u32  num_entries;
        public:
            ALWAYS_INLINE KVirtualAddress GetTableEntry(KVirtualAddress table, size_t index) {
                return table + index * sizeof(PageTableEntry);
            }

            ALWAYS_INLINE L1PageTableEntry *GetL1Entry(KProcessAddress address) {
                return GetPointer<L1PageTableEntry>(GetTableEntry(KVirtualAddress(this->table), (GetInteger(address) >> (PageBits + LevelBits * 2)) & (this->num_entries - 1)));
            }

            ALWAYS_INLINE L2PageTableEntry *GetL2EntryFromTable(KVirtualAddress table, KProcessAddress address) {
                return GetPointer<L2PageTableEntry>(GetTableEntry(table, (GetInteger(address) >> (PageBits + LevelBits * 1)) & ((1ul << LevelBits) - 1)));
            }

            ALWAYS_INLINE L2PageTableEntry *GetL2Entry(const L1PageTableEntry *entry, KProcessAddress address) {
                return GetL2EntryFromTable(KMemoryLayout::GetLinearVirtualAddress(entry->GetTable()), address);
            }

            ALWAYS_INLINE L3PageTableEntry *GetL3EntryFromTable(KVirtualAddress table, KProcessAddress address) {
                return GetPointer<L3PageTableEntry>(GetTableEntry(table, (GetInteger(address) >> (PageBits + LevelBits * 0)) & ((1ul << LevelBits) - 1)));
            }

            ALWAYS_INLINE L3PageTableEntry *GetL3Entry(const L2PageTableEntry *entry, KProcessAddress address) {
                return GetL3EntryFromTable(KMemoryLayout::GetLinearVirtualAddress(entry->GetTable()), address);
            }
        public:
            constexpr KPageTableImpl() : table(), is_kernel(), num_entries() { /* ... */ }

            NOINLINE void InitializeForKernel(void *tb, KVirtualAddress start, KVirtualAddress end);

            L1PageTableEntry *Finalize();
    };

}
