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

namespace ams::kern::arch::arm64 {

    void KPageTableImpl::InitializeForKernel(void *tb, KVirtualAddress start, KVirtualAddress end) {
        m_table       = static_cast<L1PageTableEntry *>(tb);
        m_is_kernel   = true;
        m_num_entries = util::AlignUp(end - start, L1BlockSize) / L1BlockSize;

        /* Page table entries created by KInitialPageTable need to be iterated and modified to ensure KPageTable invariants. */
        PageTableEntry *level_entries[EntryLevel_Count] = { nullptr, nullptr, m_table };
        u32 level = EntryLevel_L1;
        while (level != EntryLevel_L1 || (level_entries[EntryLevel_L1] - static_cast<PageTableEntry *>(m_table)) < m_num_entries) {
            /* Get the pte; it must never have the validity-extension flag set. */
            auto *pte = level_entries[level];
            MESOSPHERE_ASSERT((pte->GetSoftwareReservedBits() & PageTableEntry::SoftwareReservedBit_Valid) == 0);

            /* While we're a table, recurse, fixing up the reference counts. */
            while (level > EntryLevel_L3 && pte->IsMappedTable()) {
                /* Count how many references are in the table. */
                auto *table = GetPointer<PageTableEntry>(GetPageTableVirtualAddress(pte->GetTable()));

                size_t ref_count = 0;
                for (size_t i = 0; i < BlocksPerTable; ++i) {
                    if (table[i].IsMapped()) {
                        ++ref_count;
                    }
                }

                /* Set the reference count for our new page, adding one additional uncloseable reference; kernel pages must never be unreferenced. */
                pte->SetTableReferenceCount(ref_count + 1).SetValid();

                /* Iterate downwards. */
                level -= 1;
                level_entries[level] = table;
                pte = level_entries[level];

                /* Check that the entry isn't unexpected. */
                MESOSPHERE_ASSERT((pte->GetSoftwareReservedBits() & PageTableEntry::SoftwareReservedBit_Valid) == 0);
            }

            /* We're dealing with some block. If it's mapped, set it valid. */
            if (pte->IsMapped()) {
                pte->SetValid();
            }

            /* Advance. */
            while (true) {
                /* Advance to the next entry at the current level. */
                if (!util::IsAligned(reinterpret_cast<uintptr_t>(++level_entries[level]), PageSize)) {
                    break;
                }

                /* If we're at the end of a level, advance upwards. */
                level_entries[level++] = nullptr;

                if (level > EntryLevel_L1) {
                    return;
                }
            }
        }
    }

    void KPageTableImpl::InitializeForProcess(void *tb, KVirtualAddress start, KVirtualAddress end) {
        m_table       = static_cast<L1PageTableEntry *>(tb);
        m_is_kernel   = false;
        m_num_entries = util::AlignUp(end - start, L1BlockSize) / L1BlockSize;
    }

    L1PageTableEntry *KPageTableImpl::Finalize() {
        return m_table;
    }

    bool KPageTableImpl::BeginTraversal(TraversalEntry *out_entry, TraversalContext *out_context, KProcessAddress address) const {
        /* Setup invalid defaults. */
        *out_entry   = {};
        *out_context = {};

        /* Validate that we can read the actual entry. */
        const size_t l0_index = GetL0Index(address);
        const size_t l1_index = GetL1Index(address);
        if (m_is_kernel) {
            /* Kernel entries must be accessed via TTBR1. */
            if ((l0_index != MaxPageTableEntries - 1) || (l1_index < MaxPageTableEntries - m_num_entries)) {
                return false;
            }
        } else {
            /* User entries must be accessed with TTBR0. */
            if ((l0_index != 0) || l1_index >= m_num_entries) {
                return false;
            }
        }

        /* Get the L1 entry, and check if it's a table. */
        out_context->level_entries[EntryLevel_L1] = this->GetL1Entry(address);
        if (out_context->level_entries[EntryLevel_L1]->IsMappedTable()) {
            /* Get the L2 entry, and check if it's a table. */
            out_context->level_entries[EntryLevel_L2] = this->GetL2EntryFromTable(GetPageTableVirtualAddress(out_context->level_entries[EntryLevel_L1]->GetTable()), address);
            if (out_context->level_entries[EntryLevel_L2]->IsMappedTable()) {
                /* Get the L3 entry. */
                out_context->level_entries[EntryLevel_L3] = this->GetL3EntryFromTable(GetPageTableVirtualAddress(out_context->level_entries[EntryLevel_L2]->GetTable()), address);

                /* It's either a page or not. */
                out_context->level = EntryLevel_L3;
            } else {
                /* Not a L2 table, so possibly an L2 block. */
                out_context->level = EntryLevel_L2;
            }
        } else {
            /* Not a L1 table, so possibly an L1 block. */
            out_context->level = EntryLevel_L1;
        }

        /* Determine other fields. */
        const auto *pte = out_context->level_entries[out_context->level];

        out_context->is_contiguous = pte->IsContiguous();

        out_entry->sw_reserved_bits = pte->GetSoftwareReservedBits();
        out_entry->attr             = 0;
        out_entry->phys_addr        = this->GetBlock(pte, out_context->level) + this->GetOffset(address, out_context->level);
        out_entry->block_size       = static_cast<size_t>(1) << (PageBits + LevelBits * out_context->level + 4 * out_context->is_contiguous);

        return out_context->level == EntryLevel_L3 ? pte->IsPage() : pte->IsBlock();
    }

    bool KPageTableImpl::ContinueTraversal(TraversalEntry *out_entry, TraversalContext *context) const {
        /* Advance entry. */
        auto *cur_pte = context->level_entries[context->level];
        auto *next_pte = reinterpret_cast<PageTableEntry *>(context->is_contiguous ? util::AlignDown(reinterpret_cast<uintptr_t>(cur_pte), BlocksPerContiguousBlock * sizeof(PageTableEntry)) + BlocksPerContiguousBlock * sizeof(PageTableEntry) : reinterpret_cast<uintptr_t>(cur_pte) + sizeof(PageTableEntry));

        /* Set the pte. */
        context->level_entries[context->level] = next_pte;

        /* Advance appropriately. */
        while (context->level < EntryLevel_L1 && util::IsAligned(reinterpret_cast<uintptr_t>(context->level_entries[context->level]), PageSize)) {
            /* Advance the above table by one entry. */
            context->level_entries[context->level + 1]++;
            context->level = static_cast<EntryLevel>(util::ToUnderlying(context->level) + 1);
        }

        /* Check if we've hit the end of the L1 table. */
        if (context->level == EntryLevel_L1) {
            if (context->level_entries[EntryLevel_L1] - static_cast<const PageTableEntry *>(m_table) >= m_num_entries) {
                *context   = {};
                *out_entry = {};
                return false;
            }
        }

        /* We may have advanced to a new table, and if we have we should descend. */
        while (context->level > EntryLevel_L3 && context->level_entries[context->level]->IsMappedTable()) {
            context->level_entries[context->level - 1] = GetPointer<PageTableEntry>(GetPageTableVirtualAddress(context->level_entries[context->level]->GetTable()));
            context->level = static_cast<EntryLevel>(util::ToUnderlying(context->level) - 1);
        }

        const auto *pte = context->level_entries[context->level];

        context->is_contiguous = pte->IsContiguous();

        out_entry->sw_reserved_bits = pte->GetSoftwareReservedBits();
        out_entry->attr             = 0;
        out_entry->phys_addr        = this->GetBlock(pte, context->level);
        out_entry->block_size       = static_cast<size_t>(1) << (PageBits + LevelBits * context->level + 4 * context->is_contiguous);
        return context->level == EntryLevel_L3 ? pte->IsPage() : pte->IsBlock();
    }

    bool KPageTableImpl::GetPhysicalAddress(KPhysicalAddress *out, KProcessAddress address) const {
        /* Validate that we can read the actual entry. */
        const size_t l0_index = GetL0Index(address);
        const size_t l1_index = GetL1Index(address);
        if (m_is_kernel) {
            /* Kernel entries must be accessed via TTBR1. */
            if ((l0_index != MaxPageTableEntries - 1) || (l1_index < MaxPageTableEntries - m_num_entries)) {
                return false;
            }
        } else {
            /* User entries must be accessed with TTBR0. */
            if ((l0_index != 0) || l1_index >= m_num_entries) {
                return false;
            }
        }

        /* Get the L1 entry, and check if it's a table. */
        const PageTableEntry *pte = this->GetL1Entry(address);
        EntryLevel level          = EntryLevel_L1;
        if (pte->IsMappedTable()) {
            /* Get the L2 entry, and check if it's a table. */
            pte   = this->GetL2EntryFromTable(GetPageTableVirtualAddress(pte->GetTable()), address);
            level = EntryLevel_L2;
            if (pte->IsMappedTable()) {
                pte   = this->GetL3EntryFromTable(GetPageTableVirtualAddress(pte->GetTable()), address);
                level = EntryLevel_L3;
            }
        }

        const bool is_block = level == EntryLevel_L3 ? pte->IsPage() : pte->IsBlock();
        if (is_block) {
            *out = this->GetBlock(pte, level) + this->GetOffset(address, level);
        } else {
            *out = Null<KPhysicalAddress>;
        }

        return is_block;
    }

    bool KPageTableImpl::MergePages(KVirtualAddress *out, TraversalContext *context, EntryUpdatedCallback on_entry_updated, const void *pt) {
        /* We want to upgrade the pages by one step. */
        if (context->is_contiguous) {
            /* We can't merge an L1 table. */
            if (context->level == EntryLevel_L1) {
                return false;
            }

            /* We want to upgrade a contiguous mapping in a table to a block. */
            PageTableEntry *pte = reinterpret_cast<PageTableEntry *>(util::AlignDown(reinterpret_cast<uintptr_t>(context->level_entries[context->level]), BlocksPerTable * sizeof(PageTableEntry)));
            const KPhysicalAddress phys_addr = util::AlignDown(GetBlock(pte, context->level), GetBlockSize(static_cast<EntryLevel>(context->level + 1), false));

            /* First, check that all entries are valid for us to merge. */
            const u64 entry_template = pte->GetEntryTemplateForMerge();
            for (size_t i = 0; i < BlocksPerTable; ++i) {
                if (!pte[i].IsForMerge(entry_template | GetInteger(phys_addr + (i << (PageBits + LevelBits * context->level))) | PageTableEntry::ContigType_Contiguous | pte->GetTestTableMask())) {
                    return false;
                }
                if (i > 0 && pte[i].IsHeadOrHeadAndBodyMergeDisabled()) {
                    return false;
                }
                if (i < BlocksPerTable - 1 && pte[i].IsTailMergeDisabled()) {
                    return false;
                }
            }

            /* The entries are valid for us to merge, so merge them. */
            const auto *head_pte = pte;
            const auto *tail_pte = pte + BlocksPerTable - 1;
            const auto sw_reserved_bits = PageTableEntry::EncodeSoftwareReservedBits(head_pte->IsHeadMergeDisabled(), head_pte->IsHeadAndBodyMergeDisabled(), tail_pte->IsTailMergeDisabled());

            *context->level_entries[context->level + 1] = PageTableEntry(PageTableEntry::BlockTag{}, phys_addr, PageTableEntry(entry_template), sw_reserved_bits, false, false);
            on_entry_updated(pt);

            /* Update our context. */
            context->is_contiguous = false;
            context->level         = static_cast<EntryLevel>(util::ToUnderlying(context->level) + 1);

            /* Set the output to the table we just freed. */
            *out = KVirtualAddress(pte);
        } else {
            /* We want to upgrade a non-contiguous mapping to a contiguous mapping. */
            PageTableEntry *pte = reinterpret_cast<PageTableEntry *>(util::AlignDown(reinterpret_cast<uintptr_t>(context->level_entries[context->level]), BlocksPerContiguousBlock * sizeof(PageTableEntry)));
            const KPhysicalAddress phys_addr = util::AlignDown(GetBlock(pte, context->level), GetBlockSize(context->level, true));

            /* First, check that all entries are valid for us to merge. */
            const u64 entry_template = pte->GetEntryTemplateForMerge();
            for (size_t i = 0; i < BlocksPerContiguousBlock; ++i) {
                if (!pte[i].IsForMerge(entry_template | GetInteger(phys_addr + (i << (PageBits + LevelBits * context->level))) | pte->GetTestTableMask())) {
                    return false;
                }
                if (i > 0 && pte[i].IsHeadOrHeadAndBodyMergeDisabled()) {
                    return false;
                }
                if (i < BlocksPerContiguousBlock - 1 && pte[i].IsTailMergeDisabled()) {
                    return false;
                }
            }

            /* The entries are valid for us to merge, so merge them. */
            const auto *head_pte = pte;
            const auto *tail_pte = pte + BlocksPerContiguousBlock - 1;
            const auto sw_reserved_bits = PageTableEntry::EncodeSoftwareReservedBits(head_pte->IsHeadMergeDisabled(), head_pte->IsHeadAndBodyMergeDisabled(), tail_pte->IsTailMergeDisabled());

            for (size_t i = 0; i < BlocksPerContiguousBlock; ++i) {
                pte[i] = PageTableEntry(PageTableEntry::BlockTag{}, phys_addr + (i << (PageBits + LevelBits * context->level)), PageTableEntry(entry_template), sw_reserved_bits, true, context->level == EntryLevel_L3);
            }
            on_entry_updated(pt);

            /* Update our context. */
            context->level_entries[context->level] = pte;
            context->is_contiguous = true;
        }

        return true;
    }

    void KPageTableImpl::SeparatePages(TraversalEntry *entry, TraversalContext *context, KProcessAddress address, PageTableEntry *pte, EntryUpdatedCallback on_entry_updated, const void *pt) const {
        /* We want to downgrade the pages by one step. */
        if (context->is_contiguous) {
            /* We want to downgrade a contiguous mapping to a non-contiguous mapping. */
            pte = reinterpret_cast<PageTableEntry *>(util::AlignDown(reinterpret_cast<uintptr_t>(context->level_entries[context->level]), BlocksPerContiguousBlock * sizeof(PageTableEntry)));

            auto * const first = pte;
            const KPhysicalAddress block = this->GetBlock(first, context->level);
            for (size_t i = 0; i < BlocksPerContiguousBlock; ++i) {
                pte[i] = PageTableEntry(PageTableEntry::BlockTag{}, block + (i << (PageBits + LevelBits * context->level)), PageTableEntry(first->GetEntryTemplateForSeparateContiguous(i)), PageTableEntry::SeparateContiguousTag{});
            }
            on_entry_updated(pt);

            context->is_contiguous = false;

            context->level_entries[context->level] = pte + (this->GetLevelIndex(address, context->level) & (BlocksPerContiguousBlock - 1));
        } else {
            /* We want to downgrade a block into a table. */
            auto * const first = context->level_entries[context->level];
            const KPhysicalAddress block = this->GetBlock(first, context->level);
            for (size_t i = 0; i < BlocksPerTable; ++i) {
                pte[i] = PageTableEntry(PageTableEntry::BlockTag{}, block + (i << (PageBits + LevelBits * (context->level - 1))), PageTableEntry(first->GetEntryTemplateForSeparate(i)), PageTableEntry::SoftwareReservedBit_None, true, context->level - 1 == EntryLevel_L3);
            }

            context->is_contiguous = true;
            context->level         = static_cast<EntryLevel>(util::ToUnderlying(context->level) - 1);

            /* Wait for pending stores to complete. */
            cpu::DataSynchronizationBarrierInnerShareableStore();

            /* Update the block entry to be a table entry. */
            *context->level_entries[context->level + 1] = PageTableEntry(PageTableEntry::TableTag{}, KPageTable::GetPageTablePhysicalAddress(KVirtualAddress(pte)), m_is_kernel, true, BlocksPerTable);
            on_entry_updated(pt);

            context->level_entries[context->level] = pte + this->GetLevelIndex(address, context->level);
        }

        entry->sw_reserved_bits = context->level_entries[context->level]->GetSoftwareReservedBits();
        entry->attr             = 0;
        entry->phys_addr        = this->GetBlock(context->level_entries[context->level], context->level) + this->GetOffset(address, context->level);
        entry->block_size       = static_cast<size_t>(1) << (PageBits + LevelBits * context->level + 4 * context->is_contiguous);
    }

    void KPageTableImpl::Dump(uintptr_t start, size_t size) const {
        /* If zero size, there's nothing to dump. */
        if (size == 0) {
            return;
        }

        /* Define extents. */
        const uintptr_t end  = start + size;
        const uintptr_t last = end - 1;

        /* Define tracking variables. */
        bool unmapped = false;
        uintptr_t unmapped_start = 0;

        /* Walk the table. */
        uintptr_t cur = start;
        while (cur < end) {
            /* Validate that we can read the actual entry. */
            const size_t l0_index = GetL0Index(cur);
            const size_t l1_index = GetL1Index(cur);
            if (m_is_kernel) {
                /* Kernel entries must be accessed via TTBR1. */
                if ((l0_index != MaxPageTableEntries - 1) || (l1_index < MaxPageTableEntries - m_num_entries)) {
                    return;
                }
            } else {
                /* User entries must be accessed with TTBR0. */
                if ((l0_index != 0) || l1_index >= m_num_entries) {
                    return;
                }
            }

            /* Try to get from l1 table. */
            const L1PageTableEntry *l1_entry = this->GetL1Entry(cur);
            if (l1_entry->IsBlock()) {
                /* Update. */
                cur = util::AlignDown(cur, L1BlockSize);
                if (unmapped) {
                    unmapped = false;
                    MESOSPHERE_RELEASE_LOG("%016lx - %016lx: not mapped\n", unmapped_start, cur - 1);
                }

                /* Print. */
                MESOSPHERE_RELEASE_LOG("%016lx: %016lx PA=%p SZ=1G Mapped=%d UXN=%d PXN=%d Cont=%d nG=%d AF=%d SH=%x RO=%d UA=%d NS=%d AttrIndx=%d NoMerge=%d,%d,%d\n", cur,
                                                                                                                                                                        *reinterpret_cast<const u64 *>(l1_entry),
                                                                                                                                                                        reinterpret_cast<void *>(GetInteger(l1_entry->GetBlock())),
                                                                                                                                                                        l1_entry->IsMapped(),
                                                                                                                                                                        l1_entry->IsUserExecuteNever(),
                                                                                                                                                                        l1_entry->IsPrivilegedExecuteNever(),
                                                                                                                                                                        l1_entry->IsContiguous(),
                                                                                                                                                                        !l1_entry->IsGlobal(),
                                                                                                                                                                        static_cast<int>(l1_entry->GetAccessFlagInteger()),
                                                                                                                                                                        static_cast<unsigned int>(l1_entry->GetShareableInteger()),
                                                                                                                                                                        l1_entry->IsReadOnly(),
                                                                                                                                                                        l1_entry->IsUserAccessible(),
                                                                                                                                                                        l1_entry->IsNonSecure(),
                                                                                                                                                                        static_cast<int>(l1_entry->GetPageAttributeInteger()),
                                                                                                                                                                        l1_entry->IsHeadMergeDisabled(),
                                                                                                                                                                        l1_entry->IsHeadAndBodyMergeDisabled(),
                                                                                                                                                                        l1_entry->IsTailMergeDisabled());

                /* Advance. */
                cur += L1BlockSize;
                continue;
            } else if (!l1_entry->IsTable()) {
                /* Update. */
                cur = util::AlignDown(cur, L1BlockSize);
                if (!unmapped) {
                    unmapped_start = cur;
                    unmapped       = true;
                }

                /* Advance. */
                cur += L1BlockSize;
                continue;
            }

            /* Try to get from l2 table. */
            const L2PageTableEntry *l2_entry = this->GetL2Entry(l1_entry, cur);
            if (l2_entry->IsBlock()) {
                /* Update. */
                cur = util::AlignDown(cur, L2BlockSize);
                if (unmapped) {
                    unmapped = false;
                    MESOSPHERE_RELEASE_LOG("%016lx - %016lx: not mapped\n", unmapped_start, cur - 1);
                }

                /* Print. */
                MESOSPHERE_RELEASE_LOG("%016lx: %016lx PA=%p SZ=2M Mapped=%d UXN=%d PXN=%d Cont=%d nG=%d AF=%d SH=%x RO=%d UA=%d NS=%d AttrIndx=%d NoMerge=%d,%d,%d\n", cur,
                                                                                                                                                                        *reinterpret_cast<const u64 *>(l2_entry),
                                                                                                                                                                        reinterpret_cast<void *>(GetInteger(l2_entry->GetBlock())),
                                                                                                                                                                        l2_entry->IsMapped(),
                                                                                                                                                                        l2_entry->IsUserExecuteNever(),
                                                                                                                                                                        l2_entry->IsPrivilegedExecuteNever(),
                                                                                                                                                                        l2_entry->IsContiguous(),
                                                                                                                                                                        !l2_entry->IsGlobal(),
                                                                                                                                                                        static_cast<int>(l2_entry->GetAccessFlagInteger()),
                                                                                                                                                                        static_cast<unsigned int>(l2_entry->GetShareableInteger()),
                                                                                                                                                                        l2_entry->IsReadOnly(),
                                                                                                                                                                        l2_entry->IsUserAccessible(),
                                                                                                                                                                        l2_entry->IsNonSecure(),
                                                                                                                                                                        static_cast<int>(l2_entry->GetPageAttributeInteger()),
                                                                                                                                                                        l2_entry->IsHeadMergeDisabled(),
                                                                                                                                                                        l2_entry->IsHeadAndBodyMergeDisabled(),
                                                                                                                                                                        l2_entry->IsTailMergeDisabled());

                /* Advance. */
                cur += L2BlockSize;
                continue;
            } else if (!l2_entry->IsTable()) {
                /* Update. */
                cur = util::AlignDown(cur, L2BlockSize);
                if (!unmapped) {
                    unmapped_start = cur;
                    unmapped       = true;
                }

                /* Advance. */
                cur += L2BlockSize;
                continue;
            }

            /* Try to get from l3 table. */
            const L3PageTableEntry *l3_entry = this->GetL3Entry(l2_entry, cur);
            if (l3_entry->IsBlock()) {
                /* Update. */
                cur = util::AlignDown(cur, L3BlockSize);
                if (unmapped) {
                    unmapped = false;
                    MESOSPHERE_RELEASE_LOG("%016lx - %016lx: not mapped\n", unmapped_start, cur - 1);
                }

                /* Print. */
                MESOSPHERE_RELEASE_LOG("%016lx: %016lx PA=%p SZ=4K Mapped=%d UXN=%d PXN=%d Cont=%d nG=%d AF=%d SH=%x RO=%d UA=%d NS=%d AttrIndx=%d NoMerge=%d,%d,%d\n", cur,
                                                                                                                                                                        *reinterpret_cast<const u64 *>(l3_entry),
                                                                                                                                                                        reinterpret_cast<void *>(GetInteger(l3_entry->GetBlock())),
                                                                                                                                                                        l3_entry->IsMapped(),
                                                                                                                                                                        l3_entry->IsUserExecuteNever(),
                                                                                                                                                                        l3_entry->IsPrivilegedExecuteNever(),
                                                                                                                                                                        l3_entry->IsContiguous(),
                                                                                                                                                                        !l3_entry->IsGlobal(),
                                                                                                                                                                        static_cast<int>(l3_entry->GetAccessFlagInteger()),
                                                                                                                                                                        static_cast<unsigned int>(l3_entry->GetShareableInteger()),
                                                                                                                                                                        l3_entry->IsReadOnly(),
                                                                                                                                                                        l3_entry->IsUserAccessible(),
                                                                                                                                                                        l3_entry->IsNonSecure(),
                                                                                                                                                                        static_cast<int>(l3_entry->GetPageAttributeInteger()),
                                                                                                                                                                        l3_entry->IsHeadMergeDisabled(),
                                                                                                                                                                        l3_entry->IsHeadAndBodyMergeDisabled(),
                                                                                                                                                                        l3_entry->IsTailMergeDisabled());

                /* Advance. */
                cur += L3BlockSize;
                continue;
            } else {
                /* Update. */
                cur = util::AlignDown(cur, L3BlockSize);
                if (!unmapped) {
                    unmapped_start = cur;
                    unmapped       = true;
                }

                /* Advance. */
                cur += L3BlockSize;
                continue;
            }
        }

        /* Print the last unmapped range if necessary. */
        if (unmapped) {
            MESOSPHERE_RELEASE_LOG("%016lx - %016lx: not mapped\n", unmapped_start, last);
        }
    }

    size_t KPageTableImpl::CountPageTables() const {
        size_t num_tables = 0;

        #if defined(MESOSPHERE_BUILD_FOR_DEBUGGING)
        {
            ++num_tables;
            for (size_t l1_index = 0; l1_index < m_num_entries; ++l1_index) {
                auto &l1_entry = m_table[l1_index];
                if (l1_entry.IsTable()) {
                    ++num_tables;
                    for (size_t l2_index = 0; l2_index < MaxPageTableEntries; ++l2_index) {
                        auto *l2_entry = GetPointer<L2PageTableEntry>(GetTableEntry(KMemoryLayout::GetLinearVirtualAddress(l1_entry.GetTable()), l2_index));
                        if (l2_entry->IsTable()) {
                            ++num_tables;
                        }
                    }
                }
            }
        }
        #endif

        return num_tables;
    }

}
