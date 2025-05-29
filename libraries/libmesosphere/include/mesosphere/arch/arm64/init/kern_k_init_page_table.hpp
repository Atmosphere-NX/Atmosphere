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
#pragma once
#include <vapours.hpp>
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_k_typed_address.hpp>
#include <mesosphere/kern_select_cpu.hpp>
#include <mesosphere/arch/arm64/kern_k_page_table_entry.hpp>
#include <mesosphere/kern_select_system_control.hpp>

namespace ams::kern::arch::arm64::init {

    /* NOTE: Nintendo uses virtual functions, rather than a concept + template. */
    template<typename T>
    concept IsInitialPageAllocator = requires (T &t, KPhysicalAddress phys_addr, size_t size) {
        { t.Allocate(size) }        -> std::same_as<KPhysicalAddress>;
        { t.Free(phys_addr, size) } -> std::same_as<void>;
    };

    class KInitialPageTable {
        private:
            KPhysicalAddress m_l1_tables[2];
            u32 m_num_entries[2];
        public:
            template<IsInitialPageAllocator PageAllocator>
            KInitialPageTable(KVirtualAddress start_address, KVirtualAddress end_address, PageAllocator &allocator) {
                /* Set tables. */
                m_l1_tables[0] = AllocateNewPageTable(allocator, 0);
                m_l1_tables[1] = AllocateNewPageTable(allocator, 0);

                /* Set counts. */
                m_num_entries[0] = MaxPageTableEntries;
                m_num_entries[1] = ((end_address / L1BlockSize) & (MaxPageTableEntries - 1)) - ((start_address / L1BlockSize) & (MaxPageTableEntries - 1)) + 1;
            }

            KInitialPageTable() {
                /* Set tables. */
                m_l1_tables[0] = util::AlignDown(cpu::GetTtbr0El1(), PageSize);
                m_l1_tables[1] = util::AlignDown(cpu::GetTtbr1El1(), PageSize);

                /* Set counts. */
                cpu::TranslationControlRegisterAccessor tcr;
                m_num_entries[0] = tcr.GetT0Size() / L1BlockSize;
                m_num_entries[1] = tcr.GetT1Size() / L1BlockSize;

                /* Check counts. */
                MESOSPHERE_INIT_ABORT_UNLESS(0 < m_num_entries[0] && m_num_entries[0] <= MaxPageTableEntries);
                MESOSPHERE_INIT_ABORT_UNLESS(0 < m_num_entries[1] && m_num_entries[1] <= MaxPageTableEntries);
            }

            constexpr ALWAYS_INLINE uintptr_t GetTtbr0L1TableAddress() const {
                return GetInteger(m_l1_tables[0]);
            }

            constexpr ALWAYS_INLINE uintptr_t GetTtbr1L1TableAddress() const {
                return GetInteger(m_l1_tables[1]);
            }
        private:
            constexpr ALWAYS_INLINE L1PageTableEntry *GetL1Entry(KVirtualAddress address, u64 phys_to_virt_offset = 0) const {
                const size_t index = (GetInteger(address) >> (BITSIZEOF(address) - 1)) & 1;
                L1PageTableEntry *l1_table = reinterpret_cast<L1PageTableEntry *>(GetInteger(m_l1_tables[index]) + phys_to_virt_offset);
                return l1_table + ((GetInteger(address) / L1BlockSize) & (m_num_entries[index] - 1));
            }

            static constexpr ALWAYS_INLINE L2PageTableEntry *GetL2Entry(const L1PageTableEntry *entry, KVirtualAddress address, u64 phys_to_virt_offset = 0) {
                L2PageTableEntry *l2_table = reinterpret_cast<L2PageTableEntry *>(GetInteger(entry->GetTable()) + phys_to_virt_offset);
                return l2_table + ((GetInteger(address) / L2BlockSize) & (MaxPageTableEntries - 1));
            }

            static constexpr ALWAYS_INLINE L3PageTableEntry *GetL3Entry(const L2PageTableEntry *entry, KVirtualAddress address, u64 phys_to_virt_offset = 0) {
                L3PageTableEntry *l3_table = reinterpret_cast<L3PageTableEntry *>(GetInteger(entry->GetTable()) + phys_to_virt_offset);
                return l3_table + ((GetInteger(address) / L3BlockSize) & (MaxPageTableEntries - 1));
            }

            template<IsInitialPageAllocator PageAllocator>
            static ALWAYS_INLINE KPhysicalAddress AllocateNewPageTable(PageAllocator &allocator, u64 phys_to_virt_offset) {
                MESOSPHERE_UNUSED(phys_to_virt_offset);
                return allocator.Allocate(PageSize);
            }

            static ALWAYS_INLINE void ClearNewPageTable(KPhysicalAddress address, u64 phys_to_virt_offset) {
                /* Convert to a deferenceable address, and clear. */
                volatile u64 *ptr = reinterpret_cast<volatile u64 *>(GetInteger(address) + phys_to_virt_offset);
                for (size_t i = 0; i < PageSize / sizeof(u64); ++i) {
                    ptr[i] = 0;
                }
            }
        public:
            static consteval size_t GetMaximumOverheadSize(size_t size) {
                return (util::DivideUp(size, L1BlockSize) + util::DivideUp(size, L2BlockSize)) * PageSize;
            }
        private:
            size_t NOINLINE GetBlockCount(KVirtualAddress virt_addr, size_t size, size_t block_size) {
                const KVirtualAddress end_virt_addr = virt_addr + size;
                size_t count = 0;
                while (virt_addr < end_virt_addr) {
                    L1PageTableEntry *l1_entry = this->GetL1Entry(virt_addr);

                    /* If an L1 block is mapped or we're empty, advance by L1BlockSize. */
                    if (l1_entry->IsMappedBlock() || l1_entry->IsMappedEmpty()) {
                        MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(GetInteger(virt_addr), L1BlockSize));
                        MESOSPHERE_INIT_ABORT_UNLESS(static_cast<size_t>(end_virt_addr - virt_addr) >= L1BlockSize);
                        virt_addr += L1BlockSize;
                        if (l1_entry->IsMappedBlock() && block_size == L1BlockSize) {
                            count++;
                        }
                        continue;
                    }

                    /* Non empty and non-block must be table. */
                    MESOSPHERE_INIT_ABORT_UNLESS(l1_entry->IsMappedTable());

                    /* Table, so check if we're mapped in L2. */
                    L2PageTableEntry *l2_entry = GetL2Entry(l1_entry, virt_addr);

                    if (l2_entry->IsMappedBlock() || l2_entry->IsMappedEmpty()) {
                        const size_t advance_size = (l2_entry->IsMappedBlock() && l2_entry->IsContiguous()) ? L2ContiguousBlockSize : L2BlockSize;
                        MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(GetInteger(virt_addr), advance_size));
                        MESOSPHERE_INIT_ABORT_UNLESS(static_cast<size_t>(end_virt_addr - virt_addr) >= advance_size);
                        virt_addr += advance_size;
                        if (l2_entry->IsMappedBlock() && block_size == advance_size) {
                            count++;
                        }
                        continue;
                    }

                    /* Non empty and non-block must be table. */
                    MESOSPHERE_INIT_ABORT_UNLESS(l2_entry->IsMappedTable());

                    /* Table, so check if we're mapped in L3. */
                    L3PageTableEntry *l3_entry = GetL3Entry(l2_entry, virt_addr);

                    /* L3 must be block or empty. */
                    MESOSPHERE_INIT_ABORT_UNLESS(l3_entry->IsMappedBlock() || l3_entry->IsMappedEmpty());

                    const size_t advance_size = (l3_entry->IsMappedBlock() && l3_entry->IsContiguous()) ? L3ContiguousBlockSize : L3BlockSize;
                    MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(GetInteger(virt_addr), advance_size));
                    MESOSPHERE_INIT_ABORT_UNLESS(static_cast<size_t>(end_virt_addr - virt_addr) >= advance_size);
                    virt_addr += advance_size;
                    if (l3_entry->IsMappedBlock() && block_size == advance_size) {
                        count++;
                    }
                }
                return count;
            }

            KVirtualAddress NOINLINE GetBlockByIndex(KVirtualAddress virt_addr, size_t size, size_t block_size, size_t index) {
                const KVirtualAddress end_virt_addr = virt_addr + size;
                size_t count = 0;
                while (virt_addr < end_virt_addr) {
                    L1PageTableEntry *l1_entry = this->GetL1Entry(virt_addr);

                    /* If an L1 block is mapped or we're empty, advance by L1BlockSize. */
                    if (l1_entry->IsMappedBlock() || l1_entry->IsMappedEmpty()) {
                        MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(GetInteger(virt_addr), L1BlockSize));
                        MESOSPHERE_INIT_ABORT_UNLESS(static_cast<size_t>(end_virt_addr - virt_addr) >= L1BlockSize);
                        if (l1_entry->IsMappedBlock() && block_size == L1BlockSize) {
                            if ((count++) == index) {
                                return virt_addr;
                            }
                        }
                        virt_addr += L1BlockSize;
                        continue;
                    }

                    /* Non empty and non-block must be table. */
                    MESOSPHERE_INIT_ABORT_UNLESS(l1_entry->IsMappedTable());

                    /* Table, so check if we're mapped in L2. */
                    L2PageTableEntry *l2_entry = GetL2Entry(l1_entry, virt_addr);

                    if (l2_entry->IsMappedBlock() || l2_entry->IsMappedEmpty()) {
                        const size_t advance_size = (l2_entry->IsMappedBlock() && l2_entry->IsContiguous()) ? L2ContiguousBlockSize : L2BlockSize;
                        MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(GetInteger(virt_addr), advance_size));
                        MESOSPHERE_INIT_ABORT_UNLESS(static_cast<size_t>(end_virt_addr - virt_addr) >= advance_size);
                        if (l2_entry->IsMappedBlock() && block_size == advance_size) {
                            if ((count++) == index) {
                                return virt_addr;
                            }
                        }
                        virt_addr += advance_size;
                        continue;
                    }

                    /* Non empty and non-block must be table. */
                    MESOSPHERE_INIT_ABORT_UNLESS(l2_entry->IsMappedTable());

                    /* Table, so check if we're mapped in L3. */
                    L3PageTableEntry *l3_entry = GetL3Entry(l2_entry, virt_addr);

                    /* L3 must be block or empty. */
                    MESOSPHERE_INIT_ABORT_UNLESS(l3_entry->IsMappedBlock() || l3_entry->IsMappedEmpty());

                    const size_t advance_size = (l3_entry->IsMappedBlock() && l3_entry->IsContiguous()) ? L3ContiguousBlockSize : L3BlockSize;
                    MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(GetInteger(virt_addr), advance_size));
                    MESOSPHERE_INIT_ABORT_UNLESS(static_cast<size_t>(end_virt_addr - virt_addr) >= advance_size);
                    if (l3_entry->IsMappedBlock() && block_size == advance_size) {
                        if ((count++) == index) {
                            return virt_addr;
                        }
                    }
                    virt_addr += advance_size;
                }
                return Null<KVirtualAddress>;
            }

            PageTableEntry *GetMappingEntry(KVirtualAddress virt_addr, size_t block_size) {
                L1PageTableEntry *l1_entry = this->GetL1Entry(virt_addr);

                if (l1_entry->IsMappedBlock()) {
                    MESOSPHERE_INIT_ABORT_UNLESS(block_size == L1BlockSize);
                    return l1_entry;
                }

                MESOSPHERE_INIT_ABORT_UNLESS(l1_entry->IsMappedTable());

                /* Table, so check if we're mapped in L2. */
                L2PageTableEntry *l2_entry = GetL2Entry(l1_entry, virt_addr);

                if (l2_entry->IsMappedBlock()) {
                    const size_t real_size = (l2_entry->IsContiguous()) ? L2ContiguousBlockSize : L2BlockSize;
                    MESOSPHERE_INIT_ABORT_UNLESS(real_size == block_size);
                    return l2_entry;
                }

                MESOSPHERE_INIT_ABORT_UNLESS(l2_entry->IsMappedTable());

                /* Table, so check if we're mapped in L3. */
                L3PageTableEntry *l3_entry = GetL3Entry(l2_entry, virt_addr);

                /* L3 must be block. */
                MESOSPHERE_INIT_ABORT_UNLESS(l3_entry->IsMappedBlock());

                const size_t real_size = (l3_entry->IsContiguous()) ? L3ContiguousBlockSize : L3BlockSize;
                MESOSPHERE_INIT_ABORT_UNLESS(real_size == block_size);
                return l3_entry;
            }

            void NOINLINE SwapBlocks(KVirtualAddress src_virt_addr, KVirtualAddress dst_virt_addr, size_t block_size, bool do_copy) {
                static_assert(L2ContiguousBlockSize / L2BlockSize == L3ContiguousBlockSize / L3BlockSize);
                const bool contig = (block_size == L2ContiguousBlockSize || block_size == L3ContiguousBlockSize);
                const size_t num_mappings = contig ? L2ContiguousBlockSize / L2BlockSize : 1;

                /* Unmap the source. */
                PageTableEntry *src_entry = this->GetMappingEntry(src_virt_addr, block_size);
                const auto src_saved = *src_entry;
                for (size_t i = 0; i < num_mappings; i++) {
                    src_entry[i] = InvalidPageTableEntry;
                }

                /* Unmap the target. */
                PageTableEntry *dst_entry = this->GetMappingEntry(dst_virt_addr, block_size);
                const auto dst_saved = *dst_entry;
                for (size_t i = 0; i < num_mappings; i++) {
                    dst_entry[i] = InvalidPageTableEntry;
                }

                /* Invalidate the entire tlb. */
                cpu::DataSynchronizationBarrierInnerShareable();
                cpu::InvalidateEntireTlb();

                /* Copy data, if we should. */
                const u64 negative_block_size_for_mask = static_cast<u64>(-static_cast<s64>(block_size));
                const u64 offset_mask                  = negative_block_size_for_mask & ((1ul << 48) - 1);
                const KVirtualAddress copy_src_addr = KVirtualAddress(src_saved.GetRawAttributesUnsafeForSwap() & offset_mask);
                const KVirtualAddress copy_dst_addr = KVirtualAddress(dst_saved.GetRawAttributesUnsafeForSwap() & offset_mask);
                if (do_copy) {
                    u8 tmp[0x100];
                    for (size_t ofs = 0; ofs < block_size; ofs += sizeof(tmp)) {
                        std::memcpy(tmp, GetVoidPointer(copy_src_addr + ofs), sizeof(tmp));
                        std::memcpy(GetVoidPointer(copy_src_addr + ofs), GetVoidPointer(copy_dst_addr + ofs), sizeof(tmp));
                        std::memcpy(GetVoidPointer(copy_dst_addr + ofs), tmp, sizeof(tmp));
                    }
                    cpu::DataSynchronizationBarrierInnerShareable();
                }

                /* Swap the mappings. */
                const u64 attr_preserve_mask  = (block_size - 1) | 0xFFFF000000000000ul;
                const size_t shift_for_contig = contig ? 4 : 0;
                size_t advanced_size = 0;
                const u64 src_attr_val = src_saved.GetRawAttributesUnsafeForSwap() & attr_preserve_mask;
                const u64 dst_attr_val = dst_saved.GetRawAttributesUnsafeForSwap() & attr_preserve_mask;
                for (size_t i = 0; i < num_mappings; i++) {
                    reinterpret_cast<u64 *>(src_entry)[i] = GetInteger(copy_dst_addr + (advanced_size >> shift_for_contig)) | src_attr_val;
                    reinterpret_cast<u64 *>(dst_entry)[i] = GetInteger(copy_src_addr + (advanced_size >> shift_for_contig)) | dst_attr_val;
                    advanced_size += block_size;
                }

                cpu::DataSynchronizationBarrierInnerShareable();
            }

            void NOINLINE PhysicallyRandomize(KVirtualAddress virt_addr, size_t size, size_t block_size, bool do_copy) {
                const size_t block_count = this->GetBlockCount(virt_addr, size, block_size);
                if (block_count > 1) {
                    for (size_t cur_block = 0; cur_block < block_count; cur_block++) {
                        const size_t target_block = KSystemControl::Init::GenerateRandomRange(cur_block, block_count - 1);
                        if (cur_block != target_block) {
                            const KVirtualAddress cur_virt_addr    = this->GetBlockByIndex(virt_addr, size, block_size, cur_block);
                            const KVirtualAddress target_virt_addr = this->GetBlockByIndex(virt_addr, size, block_size, target_block);
                            MESOSPHERE_INIT_ABORT_UNLESS(cur_virt_addr    != Null<KVirtualAddress>);
                            MESOSPHERE_INIT_ABORT_UNLESS(target_virt_addr != Null<KVirtualAddress>);
                            this->SwapBlocks(cur_virt_addr, target_virt_addr, block_size, do_copy);
                        }
                    }
                }
            }
        public:
            template<IsInitialPageAllocator PageAllocator>
            void NOINLINE Map(KVirtualAddress virt_addr, size_t size, KPhysicalAddress phys_addr, const PageTableEntry &attr, PageAllocator &allocator, u64 phys_to_virt_offset) {
                /* Ensure that addresses and sizes are page aligned. */
                MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(GetInteger(virt_addr),    PageSize));
                MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(GetInteger(phys_addr),    PageSize));
                MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(size,                     PageSize));

                /* Iteratively map pages until the requested region is mapped. */
                while (size > 0) {
                    L1PageTableEntry *l1_entry = this->GetL1Entry(virt_addr, phys_to_virt_offset);

                    /* Can we make an L1 block? */
                    if (util::IsAligned(GetInteger(virt_addr), L1BlockSize) && util::IsAligned(GetInteger(phys_addr), L1BlockSize) && size >= L1BlockSize) {
                        *l1_entry = L1PageTableEntry(PageTableEntry::BlockTag{}, phys_addr, attr, PageTableEntry::SoftwareReservedBit_None, false);

                        virt_addr += L1BlockSize;
                        phys_addr += L1BlockSize;
                        size      -= L1BlockSize;
                        continue;
                    }

                    /* If we don't already have an L2 table, we need to make a new one. */
                    if (!l1_entry->IsMappedTable()) {
                        KPhysicalAddress new_table = AllocateNewPageTable(allocator, phys_to_virt_offset);
                        cpu::DataSynchronizationBarrierInnerShareable();
                        *l1_entry = L1PageTableEntry(PageTableEntry::TableTag{}, new_table, attr.IsPrivilegedExecuteNever());
                    }

                    L2PageTableEntry *l2_entry = GetL2Entry(l1_entry, virt_addr, phys_to_virt_offset);

                    /* Can we make a contiguous L2 block? */
                    if (util::IsAligned(GetInteger(virt_addr), L2ContiguousBlockSize) && util::IsAligned(GetInteger(phys_addr), L2ContiguousBlockSize) && size >= L2ContiguousBlockSize) {
                        for (size_t i = 0; i < L2ContiguousBlockSize / L2BlockSize; i++) {
                            l2_entry[i] = L2PageTableEntry(PageTableEntry::BlockTag{}, phys_addr, attr, PageTableEntry::SoftwareReservedBit_None, true);

                            virt_addr += L2BlockSize;
                            phys_addr += L2BlockSize;
                            size      -= L2BlockSize;
                        }
                        continue;
                    }

                    /* Can we make an L2 block? */
                    if (util::IsAligned(GetInteger(virt_addr), L2BlockSize) && util::IsAligned(GetInteger(phys_addr), L2BlockSize) && size >= L2BlockSize) {
                        *l2_entry = L2PageTableEntry(PageTableEntry::BlockTag{}, phys_addr, attr, PageTableEntry::SoftwareReservedBit_None, false);

                        virt_addr += L2BlockSize;
                        phys_addr += L2BlockSize;
                        size      -= L2BlockSize;
                        continue;
                    }

                    /* If we don't already have an L3 table, we need to make a new one. */
                    if (!l2_entry->IsMappedTable()) {
                        KPhysicalAddress new_table = AllocateNewPageTable(allocator, phys_to_virt_offset);
                        cpu::DataSynchronizationBarrierInnerShareable();
                        *l2_entry = L2PageTableEntry(PageTableEntry::TableTag{}, new_table, attr.IsPrivilegedExecuteNever());
                    }

                    L3PageTableEntry *l3_entry = GetL3Entry(l2_entry, virt_addr, phys_to_virt_offset);

                    /* Can we make a contiguous L3 block? */
                    if (util::IsAligned(GetInteger(virt_addr), L3ContiguousBlockSize) && util::IsAligned(GetInteger(phys_addr), L3ContiguousBlockSize) && size >= L3ContiguousBlockSize) {
                        for (size_t i = 0; i < L3ContiguousBlockSize / L3BlockSize; i++) {
                            l3_entry[i] = L3PageTableEntry(PageTableEntry::BlockTag{}, phys_addr, attr, PageTableEntry::SoftwareReservedBit_None, true);

                            virt_addr += L3BlockSize;
                            phys_addr += L3BlockSize;
                            size      -= L3BlockSize;
                        }
                        continue;
                    }

                    /* Make an L3 block. */
                    *l3_entry = L3PageTableEntry(PageTableEntry::BlockTag{}, phys_addr, attr, PageTableEntry::SoftwareReservedBit_None, false);
                    virt_addr += L3BlockSize;
                    phys_addr += L3BlockSize;
                    size      -= L3BlockSize;
                }

                /* Ensure data consistency after our mapping is added. */
                cpu::DataSynchronizationBarrierInnerShareable();
            }

            void UnmapTtbr0Entries(u64 phys_to_virt_offset) {
                /* Ensure data consistency before we unmap. */
                cpu::DataSynchronizationBarrierInnerShareable();

                /* Define helper, as we only want to clear non-nGnRE pages. */
                constexpr auto ShouldUnmap = [](const PageTableEntry *entry) ALWAYS_INLINE_LAMBDA -> bool {
                    return entry->GetPageAttribute() != PageTableEntry::PageAttribute_Device_nGnRE;
                };

                /* Iterate all L1 entries. */
                L1PageTableEntry * const l1_table = reinterpret_cast<L1PageTableEntry *>(GetInteger(m_l1_tables[0]) + phys_to_virt_offset);
                for (size_t l1_index = 0; l1_index < m_num_entries[0]; l1_index++) {
                    /* Get L1 entry. */
                    L1PageTableEntry * const l1_entry = l1_table + l1_index;
                    if (l1_entry->IsMappedBlock()) {
                        /* Unmap the L1 entry, if we should. */
                        if (ShouldUnmap(l1_entry)) {
                            *static_cast<PageTableEntry *>(l1_entry) = InvalidPageTableEntry;
                        }
                    } else if (l1_entry->IsMappedTable()) {
                        /* Get the L2 table. */
                        L2PageTableEntry * const l2_table = reinterpret_cast<L2PageTableEntry *>(GetInteger(l1_entry->GetTable()) + phys_to_virt_offset);

                        /* Unmap all L2 entries, as relevant. */
                        size_t remaining_l2_entries = 0;
                        for (size_t l2_index = 0; l2_index < MaxPageTableEntries; ++l2_index) {
                            /* Get L2 entry. */
                            L2PageTableEntry * const l2_entry = l2_table + l2_index;
                            if (l2_entry->IsMappedBlock()) {
                                const size_t num_to_clear = (l2_entry->IsContiguous() ? L2ContiguousBlockSize : L2BlockSize) / L2BlockSize;

                                if (ShouldUnmap(l2_entry)) {
                                    for (size_t i = 0; i < num_to_clear; ++i) {
                                        static_cast<PageTableEntry *>(l2_entry)[i] = InvalidPageTableEntry;
                                    }
                                } else {
                                    remaining_l2_entries += num_to_clear;
                                }

                                l2_index = l2_index + num_to_clear - 1;
                            } else if (l2_entry->IsMappedTable()) {
                                /* Get the L3 table. */
                                L3PageTableEntry * const l3_table = reinterpret_cast<L3PageTableEntry *>(GetInteger(l2_entry->GetTable()) + phys_to_virt_offset);

                                /* Unmap all L3 entries, as relevant. */
                                size_t remaining_l3_entries = 0;
                                for (size_t l3_index = 0; l3_index < MaxPageTableEntries; ++l3_index) {
                                    /* Get L3 entry. */
                                    if (L3PageTableEntry * const l3_entry = l3_table + l3_index; l3_entry->IsMappedBlock()) {
                                        const size_t num_to_clear = (l3_entry->IsContiguous() ? L3ContiguousBlockSize : L3BlockSize) / L3BlockSize;

                                        if (ShouldUnmap(l3_entry)) {
                                            for (size_t i = 0; i < num_to_clear; ++i) {
                                                static_cast<PageTableEntry *>(l3_entry)[i] = InvalidPageTableEntry;
                                            }
                                        } else {
                                            remaining_l3_entries += num_to_clear;
                                        }

                                        l3_index = l3_index + num_to_clear - 1;
                                    }
                                }

                                /* If we unmapped all L3 entries, clear the L2 entry. */
                                if (remaining_l3_entries == 0) {
                                    *static_cast<PageTableEntry *>(l2_entry) = InvalidPageTableEntry;

                                    /* Invalidate the entire tlb. */
                                    cpu::DataSynchronizationBarrierInnerShareable();
                                    cpu::InvalidateEntireTlb();
                                } else {
                                    remaining_l2_entries++;
                                }
                            }
                        }

                        /* If we unmapped all L2 entries, clear the L1 entry. */
                        if (remaining_l2_entries == 0) {
                            *static_cast<PageTableEntry *>(l1_entry) = InvalidPageTableEntry;

                            /* Invalidate the entire tlb. */
                            cpu::DataSynchronizationBarrierInnerShareable();
                            cpu::InvalidateEntireTlb();
                        }
                    }
                }

                /* Invalidate the entire tlb. */
                cpu::DataSynchronizationBarrierInnerShareable();
                cpu::InvalidateEntireTlb();
            }

            KPhysicalAddress GetPhysicalAddress(KVirtualAddress virt_addr) const {
                /* Get the L1 entry. */
                const L1PageTableEntry *l1_entry = this->GetL1Entry(virt_addr);

                if (l1_entry->IsMappedBlock()) {
                    return l1_entry->GetBlock() + (GetInteger(virt_addr) & (L1BlockSize - 1));
                }

                MESOSPHERE_INIT_ABORT_UNLESS(l1_entry->IsMappedTable());

                /* Get the L2 entry. */
                const L2PageTableEntry *l2_entry = GetL2Entry(l1_entry, virt_addr);

                if (l2_entry->IsMappedBlock()) {
                    return l2_entry->GetBlock() + (GetInteger(virt_addr) & (L2BlockSize - 1));
                }

                MESOSPHERE_INIT_ABORT_UNLESS(l2_entry->IsMappedTable());

                /* Get the L3 entry. */
                const L3PageTableEntry *l3_entry = GetL3Entry(l2_entry, virt_addr);

                MESOSPHERE_INIT_ABORT_UNLESS(l3_entry->IsMappedBlock());

                return l3_entry->GetBlock() + (GetInteger(virt_addr) & (L3BlockSize - 1));
            }

            KPhysicalAddress GetPhysicalAddressOfRandomizedRange(KVirtualAddress virt_addr, size_t size) const {
                /* Define tracking variables for ourselves to use. */
                KPhysicalAddress min_phys_addr = Null<KPhysicalAddress>;
                KPhysicalAddress max_phys_addr = Null<KPhysicalAddress>;

                /* Ensure the range we're querying is valid. */
                const KVirtualAddress end_virt_addr = virt_addr + size;
                if (virt_addr > end_virt_addr) {
                    MESOSPHERE_INIT_ABORT_UNLESS(size == 0);
                    return min_phys_addr;
                }

                auto UpdateExtents = [&](const KPhysicalAddress block, size_t block_size) ALWAYS_INLINE_LAMBDA {
                    /* Ensure that we are allowed to have the block here. */
                    MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(GetInteger(virt_addr), block_size));
                    MESOSPHERE_INIT_ABORT_UNLESS(block_size <= GetInteger(end_virt_addr) - GetInteger(virt_addr));
                    MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(GetInteger(block),     block_size));
                    MESOSPHERE_INIT_ABORT_UNLESS(size >= block_size);

                    const KPhysicalAddress block_end = block + block_size;

                    /* We want to update min phys addr when it's 0 or > block. */
                    /* This is equivalent in two's complement to (n - 1) >= block. */
                    if ((GetInteger(min_phys_addr) - 1) >= GetInteger(block)) {
                        min_phys_addr = block;
                    }

                    /* Update max phys addr when it's 0 or < block_end. */
                    if (GetInteger(max_phys_addr) < GetInteger(block_end) || GetInteger(max_phys_addr) == 0) {
                        max_phys_addr = block_end;
                    }

                    /* Traverse onwards. */
                    virt_addr += block_size;
                };

                while (virt_addr < end_virt_addr) {
                    L1PageTableEntry *l1_entry = this->GetL1Entry(virt_addr);

                    /* If an L1 block is mapped, update. */
                    if (l1_entry->IsMappedBlock()) {
                        UpdateExtents(l1_entry->GetBlock(), L1BlockSize);
                        continue;
                    }

                    /* Not a block, so we must have a table. */
                    MESOSPHERE_INIT_ABORT_UNLESS(l1_entry->IsMappedTable());

                    L2PageTableEntry *l2_entry = GetL2Entry(l1_entry, virt_addr);
                    if (l2_entry->IsMappedBlock()) {
                        UpdateExtents(l2_entry->GetBlock(), l2_entry->IsContiguous() ? L2ContiguousBlockSize : L2BlockSize);
                        continue;
                    }

                    /* Not a block, so we must have a table. */
                    MESOSPHERE_INIT_ABORT_UNLESS(l2_entry->IsMappedTable());

                    /* We must have a mapped l3 entry to inspect. */
                    L3PageTableEntry *l3_entry = GetL3Entry(l2_entry, virt_addr);
                    MESOSPHERE_INIT_ABORT_UNLESS(l3_entry->IsMappedBlock());

                    UpdateExtents(l3_entry->GetBlock(), l3_entry->IsContiguous() ? L3ContiguousBlockSize : L3BlockSize);
                }

                /* Ensure we got the right range. */
                MESOSPHERE_INIT_ABORT_UNLESS(GetInteger(max_phys_addr) - GetInteger(min_phys_addr) == size);

                /* Write the address that we found. */
                return min_phys_addr;
            }

            bool IsFree(KVirtualAddress virt_addr, size_t size) {
                /* Ensure that addresses and sizes are page aligned. */
                MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(GetInteger(virt_addr),    PageSize));
                MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(size,                     PageSize));

                const KVirtualAddress end_virt_addr = virt_addr + size;
                while (virt_addr < end_virt_addr) {
                    L1PageTableEntry *l1_entry = this->GetL1Entry(virt_addr);

                    /* If an L1 block is mapped, the address isn't free. */
                    if (l1_entry->IsMappedBlock()) {
                        return false;
                    }

                    if (!l1_entry->IsMappedTable()) {
                        /* Not a table, so just move to check the next region. */
                        virt_addr = util::AlignDown(GetInteger(virt_addr) + L1BlockSize, L1BlockSize);
                        continue;
                    }

                    /* Table, so check if we're mapped in L2. */
                    L2PageTableEntry *l2_entry = GetL2Entry(l1_entry, virt_addr);

                    if (l2_entry->IsMappedBlock()) {
                        return false;
                    }

                    if (!l2_entry->IsMappedTable()) {
                        /* Not a table, so just move to check the next region. */
                        virt_addr = util::AlignDown(GetInteger(virt_addr) + L2BlockSize, L2BlockSize);
                        continue;
                    }

                    /* Table, so check if we're mapped in L3. */
                    L3PageTableEntry *l3_entry = GetL3Entry(l2_entry, virt_addr);

                    if (l3_entry->IsMappedBlock()) {
                        return false;
                    }

                    /* Not a block, so move on to check the next page. */
                    virt_addr = util::AlignDown(GetInteger(virt_addr) + L3BlockSize, L3BlockSize);
                }
                return true;
            }

            void Reprotect(KVirtualAddress virt_addr, size_t size, const PageTableEntry &attr_before, const PageTableEntry &attr_after) {
                /* Ensure that addresses and sizes are page aligned. */
                MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(GetInteger(virt_addr),    PageSize));
                MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(size,                     PageSize));

                /* Iteratively reprotect pages until the requested region is reprotected. */
                while (size > 0) {
                    L1PageTableEntry *l1_entry = this->GetL1Entry(virt_addr);

                    /* Check if an L1 block is present. */
                    if (l1_entry->IsMappedBlock()) {
                        /* Ensure that we are allowed to have an L1 block here. */
                        const KPhysicalAddress block = l1_entry->GetBlock();
                        MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(GetInteger(virt_addr), L1BlockSize));
                        MESOSPHERE_INIT_ABORT_UNLESS(size >= L1BlockSize);
                        MESOSPHERE_INIT_ABORT_UNLESS(l1_entry->IsCompatibleWithAttribute(attr_before, PageTableEntry::SoftwareReservedBit_None, false));

                        /* Invalidate the existing L1 block. */
                        *static_cast<PageTableEntry *>(l1_entry) = InvalidPageTableEntry;
                        cpu::DataSynchronizationBarrierInnerShareable();
                        cpu::InvalidateEntireTlb();

                        /* Create new L1 block. */
                        *l1_entry = L1PageTableEntry(PageTableEntry::BlockTag{}, block, attr_after, PageTableEntry::SoftwareReservedBit_None, false);

                        virt_addr += L1BlockSize;
                        size      -= L1BlockSize;
                        continue;
                    }

                    /* Not a block, so we must be a table. */
                    MESOSPHERE_INIT_ABORT_UNLESS(l1_entry->IsMappedTable());

                    L2PageTableEntry *l2_entry = GetL2Entry(l1_entry, virt_addr);
                    if (l2_entry->IsMappedBlock()) {
                        const KPhysicalAddress block = l2_entry->GetBlock();

                        if (l2_entry->IsContiguous()) {
                            /* Ensure that we are allowed to have a contiguous L2 block here. */
                            MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(GetInteger(virt_addr), L2ContiguousBlockSize));
                            MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(GetInteger(block),     L2ContiguousBlockSize));
                            MESOSPHERE_INIT_ABORT_UNLESS(size >= L2ContiguousBlockSize);

                            /* Invalidate the existing contiguous L2 block. */
                            for (size_t i = 0; i < L2ContiguousBlockSize / L2BlockSize; i++) {
                                /* Ensure that the entry is valid. */
                                MESOSPHERE_INIT_ABORT_UNLESS(l2_entry[i].IsCompatibleWithAttribute(attr_before, PageTableEntry::SoftwareReservedBit_None, true));
                                static_cast<PageTableEntry *>(l2_entry)[i] = InvalidPageTableEntry;
                            }
                            cpu::DataSynchronizationBarrierInnerShareable();
                            cpu::InvalidateEntireTlb();

                            /* Create a new contiguous L2 block. */
                            for (size_t i = 0; i < L2ContiguousBlockSize / L2BlockSize; i++) {
                                l2_entry[i] = L2PageTableEntry(PageTableEntry::BlockTag{}, block + L2BlockSize * i, attr_after, PageTableEntry::SoftwareReservedBit_None, true);
                            }

                            virt_addr += L2ContiguousBlockSize;
                            size      -= L2ContiguousBlockSize;
                        } else {
                            /* Ensure that we are allowed to have an L2 block here. */
                            MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(GetInteger(virt_addr), L2BlockSize));
                            MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(GetInteger(block),     L2BlockSize));
                            MESOSPHERE_INIT_ABORT_UNLESS(size >= L2BlockSize);
                            MESOSPHERE_INIT_ABORT_UNLESS(l2_entry->IsCompatibleWithAttribute(attr_before, PageTableEntry::SoftwareReservedBit_None, false));

                            /* Invalidate the existing L2 block. */
                            *static_cast<PageTableEntry *>(l2_entry) = InvalidPageTableEntry;
                            cpu::DataSynchronizationBarrierInnerShareable();
                            cpu::InvalidateEntireTlb();

                            /* Create new L2 block. */
                            *l2_entry = L2PageTableEntry(PageTableEntry::BlockTag{}, block, attr_after, PageTableEntry::SoftwareReservedBit_None, false);

                            virt_addr += L2BlockSize;
                            size      -= L2BlockSize;
                        }

                        continue;
                    }

                    /* Not a block, so we must be a table. */
                    MESOSPHERE_INIT_ABORT_UNLESS(l2_entry->IsMappedTable());

                    /* We must have a mapped l3 entry to reprotect. */
                    L3PageTableEntry *l3_entry = GetL3Entry(l2_entry, virt_addr);
                    MESOSPHERE_INIT_ABORT_UNLESS(l3_entry->IsMappedBlock());
                    const KPhysicalAddress block = l3_entry->GetBlock();

                    if (l3_entry->IsContiguous()) {
                        /* Ensure that we are allowed to have a contiguous L3 block here. */
                        MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(GetInteger(virt_addr), L3ContiguousBlockSize));
                        MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(GetInteger(block),     L3ContiguousBlockSize));
                        MESOSPHERE_INIT_ABORT_UNLESS(size >= L3ContiguousBlockSize);

                        /* Invalidate the existing contiguous L3 block. */
                        for (size_t i = 0; i < L3ContiguousBlockSize / L3BlockSize; i++) {
                            /* Ensure that the entry is valid. */
                            MESOSPHERE_INIT_ABORT_UNLESS(l3_entry[i].IsCompatibleWithAttribute(attr_before, PageTableEntry::SoftwareReservedBit_None, true));
                            static_cast<PageTableEntry *>(l3_entry)[i] = InvalidPageTableEntry;
                        }
                        cpu::DataSynchronizationBarrierInnerShareable();
                        cpu::InvalidateEntireTlb();

                        /* Create a new contiguous L3 block. */
                        for (size_t i = 0; i < L3ContiguousBlockSize / L3BlockSize; i++) {
                            l3_entry[i] = L3PageTableEntry(PageTableEntry::BlockTag{}, block + L3BlockSize * i, attr_after, PageTableEntry::SoftwareReservedBit_None, true);
                        }

                        virt_addr += L3ContiguousBlockSize;
                        size      -= L3ContiguousBlockSize;
                    } else {
                        /* Ensure that we are allowed to have an L3 block here. */
                        MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(GetInteger(virt_addr), L3BlockSize));
                        MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(GetInteger(block),     L3BlockSize));
                        MESOSPHERE_INIT_ABORT_UNLESS(size >= L3BlockSize);
                        MESOSPHERE_INIT_ABORT_UNLESS(l3_entry->IsCompatibleWithAttribute(attr_before, PageTableEntry::SoftwareReservedBit_None, false));

                        /* Invalidate the existing L3 block. */
                        *static_cast<PageTableEntry *>(l3_entry) = InvalidPageTableEntry;
                        cpu::DataSynchronizationBarrierInnerShareable();
                        cpu::InvalidateEntireTlb();

                        /* Create new L3 block. */
                        *l3_entry = L3PageTableEntry(PageTableEntry::BlockTag{}, block, attr_after, PageTableEntry::SoftwareReservedBit_None, false);

                        virt_addr += L3BlockSize;
                        size      -= L3BlockSize;
                    }
                }

                /* Ensure data consistency after we complete reprotection. */
                cpu::DataSynchronizationBarrierInnerShareable();
            }

            void PhysicallyRandomize(KVirtualAddress virt_addr, size_t size, bool do_copy) {
                this->PhysicallyRandomize(virt_addr, size, L1BlockSize, do_copy);
                this->PhysicallyRandomize(virt_addr, size, L2ContiguousBlockSize, do_copy);
                this->PhysicallyRandomize(virt_addr, size, L2BlockSize, do_copy);
                this->PhysicallyRandomize(virt_addr, size, L3ContiguousBlockSize, do_copy);
                this->PhysicallyRandomize(virt_addr, size, L3BlockSize, do_copy);
                cpu::StoreCacheForInit(GetVoidPointer(virt_addr), size);
            }
    };

    class KInitialPageAllocator final {
        private:
            static constexpr inline size_t FreeUnitSize = BITSIZEOF(u64) * PageSize;
            struct FreeListEntry {
                FreeListEntry *next;
                size_t size;
            };
        public:
            struct State {
                uintptr_t start_address;
                uintptr_t end_address;
                FreeListEntry *free_head;
            };
        private:
            State m_state;
        public:
            constexpr ALWAYS_INLINE KInitialPageAllocator() : m_state{} { /* ... */ }

            ALWAYS_INLINE void Initialize(uintptr_t address) {
                m_state.start_address = address;
                m_state.end_address  = address;
            }

            ALWAYS_INLINE void InitializeFromState(const State *state) {
                m_state = *state;
            }

            ALWAYS_INLINE void GetFinalState(State *out) {
                *out = m_state;
                m_state = {};
            }
        private:
            bool CanAllocate(size_t align, size_t size) const {
                for (auto *cur = m_state.free_head; cur != nullptr; cur = cur->next) {
                    const uintptr_t cur_last   = reinterpret_cast<uintptr_t>(cur) + cur->size - 1;
                    const uintptr_t alloc_last = util::AlignUp(reinterpret_cast<uintptr_t>(cur), align) + size - 1;
                    if (alloc_last <= cur_last) {
                        return true;
                    }
                }
                return false;
            }

            bool TryAllocate(uintptr_t address, size_t size) {
                /* Try to allocate the region. */
                auto **prev_next = std::addressof(m_state.free_head);
                for (auto *cur = m_state.free_head; cur != nullptr; prev_next = std::addressof(cur->next), cur = cur->next) {
                    const uintptr_t cur_start = reinterpret_cast<uintptr_t>(cur);
                    const uintptr_t cur_last  = cur_start + cur->size - 1;
                    if (cur_start <= address && address + size - 1 <= cur_last) {
                        auto *alloc = reinterpret_cast<FreeListEntry *>(address);

                        /* Perform fragmentation at front. */
                        if (cur != alloc) {
                            prev_next = std::addressof(cur->next);
                            *alloc = {
                                .next = cur->next,
                                .size = cur_start + cur->size - address,
                            };
                            *cur = {
                                .next = alloc,
                                .size = address - cur_start,
                            };
                        }

                        /* Perform fragmentation at tail. */
                        if (alloc->size != size) {
                            auto *next = reinterpret_cast<FreeListEntry *>(address + size);
                            *next = {
                                .next = alloc->next,
                                .size = alloc->size - size,
                            };
                            *alloc = {
                                .next = next,
                                .size = size,
                            };
                        }

                        *prev_next = alloc->next;
                        return true;
                    }
                }

                return false;
            }
        public:
            KPhysicalAddress Allocate(size_t align, size_t size) {
                /* Ensure that the free list is non-empty. */
                while (!this->CanAllocate(align, size)) {
                    this->Free(m_state.end_address, FreeUnitSize);
                    m_state.end_address += FreeUnitSize;
                }

                /* Allocate a random address. */
                const uintptr_t aligned_start = util::AlignUp(m_state.start_address, align);
                const uintptr_t aligned_end   = util::AlignDown(m_state.end_address, align);
                const size_t ind_max = ((aligned_end - aligned_start) / align) - 1;
                while (true) {
                    if (const uintptr_t random_address = aligned_start + (KSystemControl::Init::GenerateRandomRange(0, ind_max) * align); this->TryAllocate(random_address, size)) {
                        /* Clear the allocated pages. */
                        volatile u64 *ptr = reinterpret_cast<volatile u64 *>(random_address);
                        for (size_t i = 0; i < size / sizeof(u64); ++i) {
                            ptr[i] = 0;
                        }

                        return random_address;
                    }
                }
            }

            KPhysicalAddress Allocate(size_t size) {
                return this->Allocate(size, size);
            }

            void Free(KPhysicalAddress phys_addr, size_t size) {
                auto **prev_next = std::addressof(m_state.free_head);
                auto *new_chunk  = reinterpret_cast<FreeListEntry *>(GetInteger(phys_addr));
                if (auto *cur = m_state.free_head; cur != nullptr) {
                    const uintptr_t new_start = reinterpret_cast<uintptr_t>(new_chunk);
                    const uintptr_t new_end   = GetInteger(phys_addr) + size;
                    while (true) {
                        /* Attempt coalescing. */
                        const uintptr_t cur_start = reinterpret_cast<uintptr_t>(cur);
                        const uintptr_t cur_end   = cur_start + cur->size;
                        if (new_start < new_end) {
                            if (new_end < cur_start) {
                                *new_chunk = {
                                    .next = cur,
                                    .size = size,
                                };
                                break;
                            } else if (new_end == cur_start) {
                                *new_chunk = {
                                    .next = cur->next,
                                    .size = cur->size + size,
                                };
                                break;
                            }
                        } else if (cur_end == new_start) {
                            cur->size += size;
                            return;
                        }

                        prev_next = std::addressof(cur->next);
                        if (cur->next != nullptr) {
                            cur = cur->next;
                        } else {
                            *new_chunk = {
                                .next = nullptr,
                                .size = size,
                            };
                            cur->next = new_chunk;
                            return;
                        }
                    }

                } else {
                    *new_chunk = {
                        .next = nullptr,
                        .size = size,
                    };
                }

                *prev_next = new_chunk;
            }
    };
    static_assert(IsInitialPageAllocator<KInitialPageAllocator>);

}
