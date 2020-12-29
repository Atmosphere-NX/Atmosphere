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
#include <vapours.hpp>
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_k_typed_address.hpp>
#include <mesosphere/kern_select_cpu.hpp>
#include <mesosphere/arch/arm64/kern_k_page_table_entry.hpp>
#include <mesosphere/kern_select_system_control.hpp>

namespace ams::kern::arch::arm64::init {

    inline void ClearPhysicalMemory(KPhysicalAddress address, size_t size) {
        MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(size, sizeof(u64)));

        /* This Physical Address -> void * conversion is valid, because this is init page table code. */
        /* The MMU is necessarily not yet turned on, if we are creating an initial page table. */
        volatile u64 *ptr = reinterpret_cast<volatile u64 *>(GetInteger(address));
        for (size_t i = 0; i < size / sizeof(u64); ++i) {
            ptr[i] = 0;
        }
    }

    class KInitialPageTable {
        public:
            class IPageAllocator {
                public:
                    virtual KPhysicalAddress Allocate()           { return Null<KPhysicalAddress>; }
                    virtual void Free(KPhysicalAddress phys_addr) { /* Nothing to do here. */ (void)(phys_addr); }
            };

            struct NoClear{};
        private:
            KPhysicalAddress m_l1_table;
        public:
            constexpr ALWAYS_INLINE KInitialPageTable(KPhysicalAddress l1, NoClear) : m_l1_table(l1) { /* ... */ }

            constexpr ALWAYS_INLINE KInitialPageTable(KPhysicalAddress l1) : KInitialPageTable(l1, NoClear{}) {
                ClearNewPageTable(m_l1_table);
            }

            constexpr ALWAYS_INLINE uintptr_t GetL1TableAddress() const {
                return GetInteger(m_l1_table);
            }
        private:
            static constexpr ALWAYS_INLINE L1PageTableEntry *GetL1Entry(KPhysicalAddress _l1_table, KVirtualAddress address) {
                L1PageTableEntry *l1_table = reinterpret_cast<L1PageTableEntry *>(GetInteger(_l1_table));
                return l1_table + ((GetInteger(address) >> 30) & (MaxPageTableEntries - 1));
            }

            static constexpr ALWAYS_INLINE L2PageTableEntry *GetL2Entry(const L1PageTableEntry *entry, KVirtualAddress address) {
                L2PageTableEntry *l2_table = reinterpret_cast<L2PageTableEntry *>(GetInteger(entry->GetTable()));
                return l2_table + ((GetInteger(address) >> 21) & (MaxPageTableEntries - 1));
            }

            static constexpr ALWAYS_INLINE L3PageTableEntry *GetL3Entry(const L2PageTableEntry *entry, KVirtualAddress address) {
                L3PageTableEntry *l3_table = reinterpret_cast<L3PageTableEntry *>(GetInteger(entry->GetTable()));
                return l3_table + ((GetInteger(address) >> 12) & (MaxPageTableEntries - 1));
            }

            static ALWAYS_INLINE void ClearNewPageTable(KPhysicalAddress address) {
                ClearPhysicalMemory(address, PageSize);
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
                    L1PageTableEntry *l1_entry = GetL1Entry(m_l1_table, virt_addr);

                    /* If an L1 block is mapped or we're empty, advance by L1BlockSize. */
                    if (l1_entry->IsBlock() || l1_entry->IsEmpty()) {
                        MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(GetInteger(virt_addr), L1BlockSize));
                        MESOSPHERE_INIT_ABORT_UNLESS(static_cast<size_t>(end_virt_addr - virt_addr) >= L1BlockSize);
                        virt_addr += L1BlockSize;
                        if (l1_entry->IsBlock() && block_size == L1BlockSize) {
                            count++;
                        }
                        continue;
                    }

                    /* Non empty and non-block must be table. */
                    MESOSPHERE_INIT_ABORT_UNLESS(l1_entry->IsTable());

                    /* Table, so check if we're mapped in L2. */
                    L2PageTableEntry *l2_entry = GetL2Entry(l1_entry, virt_addr);

                    if (l2_entry->IsBlock() || l2_entry->IsEmpty()) {
                        const size_t advance_size = (l2_entry->IsBlock() && l2_entry->IsContiguous()) ? L2ContiguousBlockSize : L2BlockSize;
                        MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(GetInteger(virt_addr), advance_size));
                        MESOSPHERE_INIT_ABORT_UNLESS(static_cast<size_t>(end_virt_addr - virt_addr) >= advance_size);
                        virt_addr += advance_size;
                        if (l2_entry->IsBlock() && block_size == advance_size) {
                            count++;
                        }
                        continue;
                    }

                    /* Non empty and non-block must be table. */
                    MESOSPHERE_INIT_ABORT_UNLESS(l2_entry->IsTable());

                    /* Table, so check if we're mapped in L3. */
                    L3PageTableEntry *l3_entry = GetL3Entry(l2_entry, virt_addr);

                    /* L3 must be block or empty. */
                    MESOSPHERE_INIT_ABORT_UNLESS(l3_entry->IsBlock() || l3_entry->IsEmpty());

                    const size_t advance_size = (l3_entry->IsBlock() && l3_entry->IsContiguous()) ? L3ContiguousBlockSize : L3BlockSize;
                    MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(GetInteger(virt_addr), advance_size));
                    MESOSPHERE_INIT_ABORT_UNLESS(static_cast<size_t>(end_virt_addr - virt_addr) >= advance_size);
                    virt_addr += advance_size;
                    if (l3_entry->IsBlock() && block_size == advance_size) {
                        count++;
                    }
                }
                return count;
            }

            KVirtualAddress NOINLINE GetBlockByIndex(KVirtualAddress virt_addr, size_t size, size_t block_size, size_t index) {
                const KVirtualAddress end_virt_addr = virt_addr + size;
                size_t count = 0;
                while (virt_addr < end_virt_addr) {
                    L1PageTableEntry *l1_entry = GetL1Entry(m_l1_table, virt_addr);

                    /* If an L1 block is mapped or we're empty, advance by L1BlockSize. */
                    if (l1_entry->IsBlock() || l1_entry->IsEmpty()) {
                        MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(GetInteger(virt_addr), L1BlockSize));
                        MESOSPHERE_INIT_ABORT_UNLESS(static_cast<size_t>(end_virt_addr - virt_addr) >= L1BlockSize);
                        if (l1_entry->IsBlock() && block_size == L1BlockSize) {
                            if ((count++) == index) {
                                return virt_addr;
                            }
                        }
                        virt_addr += L1BlockSize;
                        continue;
                    }

                    /* Non empty and non-block must be table. */
                    MESOSPHERE_INIT_ABORT_UNLESS(l1_entry->IsTable());

                    /* Table, so check if we're mapped in L2. */
                    L2PageTableEntry *l2_entry = GetL2Entry(l1_entry, virt_addr);

                    if (l2_entry->IsBlock() || l2_entry->IsEmpty()) {
                        const size_t advance_size = (l2_entry->IsBlock() && l2_entry->IsContiguous()) ? L2ContiguousBlockSize : L2BlockSize;
                        MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(GetInteger(virt_addr), advance_size));
                        MESOSPHERE_INIT_ABORT_UNLESS(static_cast<size_t>(end_virt_addr - virt_addr) >= advance_size);
                        if (l2_entry->IsBlock() && block_size == advance_size) {
                            if ((count++) == index) {
                                return virt_addr;
                            }
                        }
                        virt_addr += advance_size;
                        continue;
                    }

                    /* Non empty and non-block must be table. */
                    MESOSPHERE_INIT_ABORT_UNLESS(l2_entry->IsTable());

                    /* Table, so check if we're mapped in L3. */
                    L3PageTableEntry *l3_entry = GetL3Entry(l2_entry, virt_addr);

                    /* L3 must be block or empty. */
                    MESOSPHERE_INIT_ABORT_UNLESS(l3_entry->IsBlock() || l3_entry->IsEmpty());

                    const size_t advance_size = (l3_entry->IsBlock() && l3_entry->IsContiguous()) ? L3ContiguousBlockSize : L3BlockSize;
                    MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(GetInteger(virt_addr), advance_size));
                    MESOSPHERE_INIT_ABORT_UNLESS(static_cast<size_t>(end_virt_addr - virt_addr) >= advance_size);
                    if (l3_entry->IsBlock() && block_size == advance_size) {
                        if ((count++) == index) {
                            return virt_addr;
                        }
                    }
                    virt_addr += advance_size;
                }
                return Null<KVirtualAddress>;
            }

            PageTableEntry *GetMappingEntry(KVirtualAddress virt_addr, size_t block_size) {
                L1PageTableEntry *l1_entry = GetL1Entry(m_l1_table, virt_addr);

                if (l1_entry->IsBlock()) {
                    MESOSPHERE_INIT_ABORT_UNLESS(block_size == L1BlockSize);
                    return l1_entry;
                }

                MESOSPHERE_INIT_ABORT_UNLESS(l1_entry->IsTable());

                /* Table, so check if we're mapped in L2. */
                L2PageTableEntry *l2_entry = GetL2Entry(l1_entry, virt_addr);

                if (l2_entry->IsBlock()) {
                    const size_t real_size = (l2_entry->IsContiguous()) ? L2ContiguousBlockSize : L2BlockSize;
                    MESOSPHERE_INIT_ABORT_UNLESS(real_size == block_size);
                    return l2_entry;
                }

                MESOSPHERE_INIT_ABORT_UNLESS(l2_entry->IsTable());

                /* Table, so check if we're mapped in L3. */
                L3PageTableEntry *l3_entry = GetL3Entry(l2_entry, virt_addr);

                /* L3 must be block. */
                MESOSPHERE_INIT_ABORT_UNLESS(l3_entry->IsBlock());

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
                if (block_size && do_copy) {
                    u8 tmp[0x100];
                    for (size_t ofs = 0; ofs < block_size; ofs += sizeof(tmp)) {
                        std::memcpy(tmp, GetVoidPointer(copy_src_addr + ofs), sizeof(tmp));
                        std::memcpy(GetVoidPointer(copy_src_addr + ofs), GetVoidPointer(copy_dst_addr + ofs), sizeof(tmp));
                        std::memcpy(GetVoidPointer(copy_dst_addr + ofs), tmp, sizeof(tmp));
                    }
                }

                /* Swap the mappings. */
                const u64 attr_preserve_mask  = (negative_block_size_for_mask | 0xFFFF000000000000ul) ^ ((1ul << 48) - 1);
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
            void NOINLINE Map(KVirtualAddress virt_addr, size_t size, KPhysicalAddress phys_addr, const PageTableEntry &attr, IPageAllocator &allocator) {
                /* Ensure that addresses and sizes are page aligned. */
                MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(GetInteger(virt_addr),    PageSize));
                MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(GetInteger(phys_addr),    PageSize));
                MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(size,                     PageSize));

                /* Iteratively map pages until the requested region is mapped. */
                while (size > 0) {
                    L1PageTableEntry *l1_entry = GetL1Entry(m_l1_table, virt_addr);

                    /* Can we make an L1 block? */
                    if (util::IsAligned(GetInteger(virt_addr), L1BlockSize) && util::IsAligned(GetInteger(phys_addr), L1BlockSize) && size >= L1BlockSize) {
                        *l1_entry = L1PageTableEntry(PageTableEntry::BlockTag{}, phys_addr, attr, PageTableEntry::SoftwareReservedBit_None, false);
                        cpu::DataSynchronizationBarrierInnerShareable();

                        virt_addr += L1BlockSize;
                        phys_addr += L1BlockSize;
                        size      -= L1BlockSize;
                        continue;
                    }

                    /* If we don't already have an L2 table, we need to make a new one. */
                    if (!l1_entry->IsTable()) {
                        KPhysicalAddress new_table = allocator.Allocate();
                        ClearNewPageTable(new_table);
                        *l1_entry = L1PageTableEntry(PageTableEntry::TableTag{}, new_table, attr.IsPrivilegedExecuteNever());
                        cpu::DataSynchronizationBarrierInnerShareable();
                    }

                    L2PageTableEntry *l2_entry = GetL2Entry(l1_entry, virt_addr);

                    /* Can we make a contiguous L2 block? */
                    if (util::IsAligned(GetInteger(virt_addr), L2ContiguousBlockSize) && util::IsAligned(GetInteger(phys_addr), L2ContiguousBlockSize) && size >= L2ContiguousBlockSize) {
                        for (size_t i = 0; i < L2ContiguousBlockSize / L2BlockSize; i++) {
                            l2_entry[i] = L2PageTableEntry(PageTableEntry::BlockTag{}, phys_addr, attr, PageTableEntry::SoftwareReservedBit_None, true);
                            cpu::DataSynchronizationBarrierInnerShareable();

                            virt_addr += L2BlockSize;
                            phys_addr += L2BlockSize;
                            size      -= L2BlockSize;
                        }
                        continue;
                    }

                    /* Can we make an L2 block? */
                    if (util::IsAligned(GetInteger(virt_addr), L2BlockSize) && util::IsAligned(GetInteger(phys_addr), L2BlockSize) && size >= L2BlockSize) {
                        *l2_entry = L2PageTableEntry(PageTableEntry::BlockTag{}, phys_addr, attr, PageTableEntry::SoftwareReservedBit_None, false);
                        cpu::DataSynchronizationBarrierInnerShareable();

                        virt_addr += L2BlockSize;
                        phys_addr += L2BlockSize;
                        size      -= L2BlockSize;
                        continue;
                    }

                    /* If we don't already have an L3 table, we need to make a new one. */
                    if (!l2_entry->IsTable()) {
                        KPhysicalAddress new_table = allocator.Allocate();
                        ClearNewPageTable(new_table);
                        *l2_entry = L2PageTableEntry(PageTableEntry::TableTag{}, new_table, attr.IsPrivilegedExecuteNever());
                        cpu::DataSynchronizationBarrierInnerShareable();
                    }

                    L3PageTableEntry *l3_entry = GetL3Entry(l2_entry, virt_addr);

                    /* Can we make a contiguous L3 block? */
                    if (util::IsAligned(GetInteger(virt_addr), L3ContiguousBlockSize) && util::IsAligned(GetInteger(phys_addr), L3ContiguousBlockSize) && size >= L3ContiguousBlockSize) {
                        for (size_t i = 0; i < L3ContiguousBlockSize / L3BlockSize; i++) {
                            l3_entry[i] = L3PageTableEntry(PageTableEntry::BlockTag{}, phys_addr, attr, PageTableEntry::SoftwareReservedBit_None, true);
                            cpu::DataSynchronizationBarrierInnerShareable();

                            virt_addr += L3BlockSize;
                            phys_addr += L3BlockSize;
                            size      -= L3BlockSize;
                        }
                        continue;
                    }

                    /* Make an L3 block. */
                    *l3_entry = L3PageTableEntry(PageTableEntry::BlockTag{}, phys_addr, attr, PageTableEntry::SoftwareReservedBit_None, false);
                    cpu::DataSynchronizationBarrierInnerShareable();
                    virt_addr += L3BlockSize;
                    phys_addr += L3BlockSize;
                    size      -= L3BlockSize;
                }
            }

            KPhysicalAddress GetPhysicalAddress(KVirtualAddress virt_addr) const {
                /* Get the L1 entry. */
                const L1PageTableEntry *l1_entry = GetL1Entry(m_l1_table, virt_addr);

                if (l1_entry->IsBlock()) {
                    return l1_entry->GetBlock() + (GetInteger(virt_addr) & (L1BlockSize - 1));
                }

                MESOSPHERE_INIT_ABORT_UNLESS(l1_entry->IsTable());

                /* Get the L2 entry. */
                const L2PageTableEntry *l2_entry = GetL2Entry(l1_entry, virt_addr);

                if (l2_entry->IsBlock()) {
                    return l2_entry->GetBlock() + (GetInteger(virt_addr) & (L2BlockSize - 1));
                }

                MESOSPHERE_INIT_ABORT_UNLESS(l2_entry->IsTable());

                /* Get the L3 entry. */
                const L3PageTableEntry *l3_entry = GetL3Entry(l2_entry, virt_addr);

                MESOSPHERE_INIT_ABORT_UNLESS(l3_entry->IsBlock());

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
                    L1PageTableEntry *l1_entry = GetL1Entry(m_l1_table, virt_addr);

                    /* If an L1 block is mapped, update. */
                    if (l1_entry->IsBlock()) {
                        UpdateExtents(l1_entry->GetBlock(), L1BlockSize);
                        continue;
                    }

                    /* Not a block, so we must have a table. */
                    MESOSPHERE_INIT_ABORT_UNLESS(l1_entry->IsTable());

                    L2PageTableEntry *l2_entry = GetL2Entry(l1_entry, virt_addr);
                    if (l2_entry->IsBlock()) {
                        UpdateExtents(l2_entry->GetBlock(), l2_entry->IsContiguous() ? L2ContiguousBlockSize : L2BlockSize);
                        continue;
                    }

                    /* Not a block, so we must have a table. */
                    MESOSPHERE_INIT_ABORT_UNLESS(l2_entry->IsTable());

                    /* We must have a mapped l3 entry to inspect. */
                    L3PageTableEntry *l3_entry = GetL3Entry(l2_entry, virt_addr);
                    MESOSPHERE_INIT_ABORT_UNLESS(l3_entry->IsBlock());

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
                    L1PageTableEntry *l1_entry = GetL1Entry(m_l1_table, virt_addr);

                    /* If an L1 block is mapped, the address isn't free. */
                    if (l1_entry->IsBlock()) {
                        return false;
                    }

                    if (!l1_entry->IsTable()) {
                        /* Not a table, so just move to check the next region. */
                        virt_addr = util::AlignDown(GetInteger(virt_addr) + L1BlockSize, L1BlockSize);
                        continue;
                    }

                    /* Table, so check if we're mapped in L2. */
                    L2PageTableEntry *l2_entry = GetL2Entry(l1_entry, virt_addr);

                    if (l2_entry->IsBlock()) {
                        return false;
                    }

                    if (!l2_entry->IsTable()) {
                        /* Not a table, so just move to check the next region. */
                        virt_addr = util::AlignDown(GetInteger(virt_addr) + L2BlockSize, L2BlockSize);
                        continue;
                    }

                    /* Table, so check if we're mapped in L3. */
                    L3PageTableEntry *l3_entry = GetL3Entry(l2_entry, virt_addr);

                    if (l3_entry->IsBlock()) {
                        return false;
                    }

                    /* Not a block, so move on to check the next page. */
                    virt_addr = util::AlignDown(GetInteger(virt_addr) + L3BlockSize, L3BlockSize);
                }
                return true;
            }

            void Reprotect(KVirtualAddress virt_addr, size_t size, const PageTableEntry &attr_before, const PageTableEntry &attr_after) {
                /* Ensure data consistency before we begin reprotection. */
                cpu::DataSynchronizationBarrierInnerShareable();

                /* Ensure that addresses and sizes are page aligned. */
                MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(GetInteger(virt_addr),    PageSize));
                MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(size,                     PageSize));

                /* Iteratively reprotect pages until the requested region is reprotected. */
                while (size > 0) {
                    L1PageTableEntry *l1_entry = GetL1Entry(m_l1_table, virt_addr);

                    /* Check if an L1 block is present. */
                    if (l1_entry->IsBlock()) {
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
                    MESOSPHERE_INIT_ABORT_UNLESS(l1_entry->IsTable());

                    L2PageTableEntry *l2_entry = GetL2Entry(l1_entry, virt_addr);
                    if (l2_entry->IsBlock()) {
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
                    MESOSPHERE_INIT_ABORT_UNLESS(l2_entry->IsTable());

                    /* We must have a mapped l3 entry to reprotect. */
                    L3PageTableEntry *l3_entry = GetL3Entry(l2_entry, virt_addr);
                    MESOSPHERE_INIT_ABORT_UNLESS(l3_entry->IsBlock());
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
                cpu::StoreEntireCacheForInit();
            }

    };

    class KInitialPageAllocator : public KInitialPageTable::IPageAllocator {
        public:
            struct State {
                uintptr_t next_address;
                uintptr_t free_bitmap;
            };
        private:
            State m_state;
        public:
            constexpr ALWAYS_INLINE KInitialPageAllocator() : m_state{} { /* ... */ }

            ALWAYS_INLINE void Initialize(uintptr_t address) {
                m_state.next_address = address + BITSIZEOF(m_state.free_bitmap) * PageSize;
                m_state.free_bitmap  = ~uintptr_t();
            }

            ALWAYS_INLINE void InitializeFromState(uintptr_t state_val) {
                m_state = *reinterpret_cast<State *>(state_val);
            }

            ALWAYS_INLINE void GetFinalState(State *out) {
                *out = m_state;
                m_state = {};
            }
        public:
            virtual KPhysicalAddress Allocate() override {
                MESOSPHERE_INIT_ABORT_UNLESS(m_state.next_address != Null<uintptr_t>);
                uintptr_t allocated = m_state.next_address;
                if (m_state.free_bitmap != 0) {
                    u64 index;
                    uintptr_t mask;
                    do {
                        index = KSystemControl::Init::GenerateRandomRange(0, BITSIZEOF(m_state.free_bitmap) - 1);
                        mask  = (static_cast<uintptr_t>(1) << index);
                    } while ((m_state.free_bitmap & mask) == 0);
                    m_state.free_bitmap &= ~mask;
                    allocated = m_state.next_address - ((BITSIZEOF(m_state.free_bitmap) - index) * PageSize);
                } else {
                    m_state.next_address += PageSize;
                }

                ClearPhysicalMemory(allocated, PageSize);
                return allocated;
            }

            /* No need to override free. The default does nothing, and so would we. */
    };

}
