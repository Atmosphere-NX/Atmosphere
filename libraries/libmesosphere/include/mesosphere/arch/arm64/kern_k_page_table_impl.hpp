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

    class KPageTableImpl {
        NON_COPYABLE(KPageTableImpl);
        NON_MOVEABLE(KPageTableImpl);
        public:
            struct TraversalEntry {
                KPhysicalAddress phys_addr;
                size_t block_size;
                u8 sw_reserved_bits;

                constexpr bool IsHeadMergeDisabled() const { return (this->sw_reserved_bits & PageTableEntry::SoftwareReservedBit_DisableMergeHead) != 0; }
                constexpr bool IsHeadAndBodyMergeDisabled() const { return (this->sw_reserved_bits & PageTableEntry::SoftwareReservedBit_DisableMergeHeadAndBody) != 0; }
                constexpr bool IsTailMergeDisabled() const { return (this->sw_reserved_bits & PageTableEntry::SoftwareReservedBit_DisableMergeHeadTail) != 0; }
            };

            struct TraversalContext {
                const L1PageTableEntry *l1_entry;
                const L2PageTableEntry *l2_entry;
                const L3PageTableEntry *l3_entry;
            };
        private:
            static constexpr size_t PageBits  = __builtin_ctzll(PageSize);
            static constexpr size_t NumLevels = 3;
            static constexpr size_t LevelBits = 9;
            static_assert(NumLevels > 0);

            template<size_t Offset, size_t Count>
            static constexpr ALWAYS_INLINE u64 GetBits(u64 value) {
                return (value >> Offset) & ((1ul << Count) - 1);
            }

            template<size_t Offset, size_t Count>
            constexpr ALWAYS_INLINE u64 SelectBits(u64 value) {
                return value & (((1ul << Count) - 1) << Offset);
            }

            static constexpr ALWAYS_INLINE uintptr_t GetL0Index(KProcessAddress addr) { return GetBits<PageBits + LevelBits * (NumLevels - 0), LevelBits>(GetInteger(addr)); }
            static constexpr ALWAYS_INLINE uintptr_t GetL1Index(KProcessAddress addr) { return GetBits<PageBits + LevelBits * (NumLevels - 1), LevelBits>(GetInteger(addr)); }
            static constexpr ALWAYS_INLINE uintptr_t GetL2Index(KProcessAddress addr) { return GetBits<PageBits + LevelBits * (NumLevels - 2), LevelBits>(GetInteger(addr)); }
            static constexpr ALWAYS_INLINE uintptr_t GetL3Index(KProcessAddress addr) { return GetBits<PageBits + LevelBits * (NumLevels - 3), LevelBits>(GetInteger(addr)); }

            static constexpr ALWAYS_INLINE uintptr_t GetL1Offset(KProcessAddress addr) { return GetBits<0, PageBits + LevelBits * (NumLevels - 1)>(GetInteger(addr)); }
            static constexpr ALWAYS_INLINE uintptr_t GetL2Offset(KProcessAddress addr) { return GetBits<0, PageBits + LevelBits * (NumLevels - 2)>(GetInteger(addr)); }
            static constexpr ALWAYS_INLINE uintptr_t GetL3Offset(KProcessAddress addr) { return GetBits<0, PageBits + LevelBits * (NumLevels - 3)>(GetInteger(addr)); }
            static constexpr ALWAYS_INLINE uintptr_t GetContiguousL1Offset(KProcessAddress addr) { return GetBits<0, PageBits + LevelBits * (NumLevels - 1) + 4>(GetInteger(addr)); }
            static constexpr ALWAYS_INLINE uintptr_t GetContiguousL2Offset(KProcessAddress addr) { return GetBits<0, PageBits + LevelBits * (NumLevels - 2) + 4>(GetInteger(addr)); }
            static constexpr ALWAYS_INLINE uintptr_t GetContiguousL3Offset(KProcessAddress addr) { return GetBits<0, PageBits + LevelBits * (NumLevels - 3) + 4>(GetInteger(addr)); }

            static ALWAYS_INLINE KVirtualAddress GetPageTableVirtualAddress(KPhysicalAddress addr) {
                return KMemoryLayout::GetLinearVirtualAddress(addr);
            }

            ALWAYS_INLINE bool ExtractL1Entry(TraversalEntry *out_entry, TraversalContext *out_context, const L1PageTableEntry *l1_entry, KProcessAddress virt_addr) const;
            ALWAYS_INLINE bool ExtractL2Entry(TraversalEntry *out_entry, TraversalContext *out_context, const L2PageTableEntry *l2_entry, KProcessAddress virt_addr) const;
            ALWAYS_INLINE bool ExtractL3Entry(TraversalEntry *out_entry, TraversalContext *out_context, const L3PageTableEntry *l3_entry, KProcessAddress virt_addr) const;
        private:
            L1PageTableEntry *m_table;
            bool m_is_kernel;
            u32  m_num_entries;
        public:
            ALWAYS_INLINE KVirtualAddress GetTableEntry(KVirtualAddress table, size_t index) const {
                return table + index * sizeof(PageTableEntry);
            }

            ALWAYS_INLINE L1PageTableEntry *GetL1Entry(KProcessAddress address) const {
                return GetPointer<L1PageTableEntry>(GetTableEntry(KVirtualAddress(m_table), GetL1Index(address) & (m_num_entries - 1)));
            }

            ALWAYS_INLINE L2PageTableEntry *GetL2EntryFromTable(KVirtualAddress table, KProcessAddress address) const {
                return GetPointer<L2PageTableEntry>(GetTableEntry(table, GetL2Index(address)));
            }

            ALWAYS_INLINE L2PageTableEntry *GetL2Entry(const L1PageTableEntry *entry, KProcessAddress address) const {
                return GetL2EntryFromTable(KMemoryLayout::GetLinearVirtualAddress(entry->GetTable()), address);
            }

            ALWAYS_INLINE L3PageTableEntry *GetL3EntryFromTable(KVirtualAddress table, KProcessAddress address) const {
                return GetPointer<L3PageTableEntry>(GetTableEntry(table, GetL3Index(address)));
            }

            ALWAYS_INLINE L3PageTableEntry *GetL3Entry(const L2PageTableEntry *entry, KProcessAddress address) const {
                return GetL3EntryFromTable(KMemoryLayout::GetLinearVirtualAddress(entry->GetTable()), address);
            }
        public:
            constexpr KPageTableImpl() : m_table(), m_is_kernel(), m_num_entries() { /* ... */ }

            NOINLINE void InitializeForKernel(void *tb, KVirtualAddress start, KVirtualAddress end);
            NOINLINE void InitializeForProcess(void *tb, KVirtualAddress start, KVirtualAddress end);
            L1PageTableEntry *Finalize();

            void Dump(uintptr_t start, size_t size) const;
            size_t CountPageTables() const;

            bool BeginTraversal(TraversalEntry *out_entry, TraversalContext *out_context, KProcessAddress address) const;
            bool ContinueTraversal(TraversalEntry *out_entry, TraversalContext *context) const;

            bool GetPhysicalAddress(KPhysicalAddress *out, KProcessAddress virt_addr) const;
    };

}
