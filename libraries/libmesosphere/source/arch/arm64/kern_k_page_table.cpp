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

    Result KPageTable::Operate(PageLinkedList *page_list, KProcessAddress virt_addr, size_t num_pages, KPhysicalAddress phys_addr, bool is_pa_valid, const KPageProperties properties, OperationType operation, bool reuse_ll) {
        /* Check validity of parameters. */
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());
        MESOSPHERE_ASSERT(num_pages > 0);
        MESOSPHERE_ASSERT(util::IsAligned(GetInteger(virt_addr), PageSize));
        MESOSPHERE_ASSERT(this->ContainsPages(virt_addr, num_pages));

        if (operation == OperationType_Map) {
            MESOSPHERE_ABORT_UNLESS(is_pa_valid);
            MESOSPHERE_ASSERT(util::IsAligned(GetInteger(phys_addr), PageSize));
        } else {
            MESOSPHERE_ABORT_UNLESS(!is_pa_valid);
        }

        if (operation == OperationType_Unmap) {
            return this->Unmap(virt_addr, num_pages, page_list, false, reuse_ll);
        } else {
            auto entry_template = this->GetEntryTemplate(properties);

            switch (operation) {
                case OperationType_Map:
                    return this->MapContiguous(virt_addr, phys_addr, num_pages, entry_template, page_list, reuse_ll);
                MESOSPHERE_UNREACHABLE_DEFAULT_CASE();
            }
        }
    }

    Result KPageTable::Operate(PageLinkedList *page_list, KProcessAddress virt_addr, size_t num_pages, const KPageGroup *page_group, const KPageProperties properties, OperationType operation, bool reuse_ll) {
        MESOSPHERE_TODO_IMPLEMENT();
    }

    Result KPageTable::Map(KProcessAddress virt_addr, KPhysicalAddress phys_addr, size_t num_pages, PageTableEntry entry_template, PageLinkedList *page_list, bool reuse_ll) {
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());
        MESOSPHERE_ASSERT(util::IsAligned(GetInteger(virt_addr), PageSize));
        MESOSPHERE_ASSERT(util::IsAligned(GetInteger(phys_addr), PageSize));

        auto &impl = this->GetImpl();
        KVirtualAddress l2_virt = Null<KVirtualAddress>;
        KVirtualAddress l3_virt = Null<KVirtualAddress>;
        int l2_open_count = 0;
        int l3_open_count = 0;

        /* Iterate, mapping each page. */
        for (size_t i = 0; i < num_pages; i++) {
            KPhysicalAddress l3_phys = Null<KPhysicalAddress>;
            bool l2_allocated = false;

            /* If we have no L3 table, we should get or allocate one. */
            if (l3_virt == Null<KVirtualAddress>) {
                KPhysicalAddress l2_phys = Null<KPhysicalAddress>;

                /* If we have no L2 table, we should get or allocate one. */
                if (l2_virt == Null<KVirtualAddress>) {
                    if (L1PageTableEntry *l1_entry = impl.GetL1Entry(virt_addr); !l1_entry->GetTable(l2_phys)) {
                        /* Allocate table. */
                        l2_virt = AllocatePageTable(page_list, reuse_ll);
                        R_UNLESS(l2_virt != Null<KVirtualAddress>, svc::ResultOutOfResource());

                        /* Set the entry. */
                        l2_phys = GetPageTablePhysicalAddress(l2_virt);
                        PteDataSynchronizationBarrier();
                        *l1_entry = L1PageTableEntry(l2_phys, this->IsKernel(), true);
                        PteDataSynchronizationBarrier();
                        l2_allocated = true;
                    } else {
                        l2_virt = GetPageTableVirtualAddress(l2_phys);
                    }
                }
                MESOSPHERE_ASSERT(l2_virt != Null<KVirtualAddress>);

                if (L2PageTableEntry *l2_entry = impl.GetL2EntryFromTable(l2_virt, virt_addr); !l2_entry->GetTable(l3_phys)) {
                        /* Allocate table. */
                        l3_virt = AllocatePageTable(page_list, reuse_ll);
                        if (l3_virt == Null<KVirtualAddress>) {
                            /* Cleanup the L2 entry. */
                            if (l2_allocated) {
                                *impl.GetL1Entry(virt_addr) = InvalidL1PageTableEntry;
                                this->NoteUpdated();
                                FreePageTable(page_list, l2_virt);
                            } else if (this->GetPageTableManager().IsInPageTableHeap(l2_virt) && l2_open_count > 0) {
                                this->GetPageTableManager().Open(l2_virt, l2_open_count);
                            }
                            return svc::ResultOutOfResource();
                        }

                        /* Set the entry. */
                        l3_phys = GetPageTablePhysicalAddress(l3_virt);
                        PteDataSynchronizationBarrier();
                        *l2_entry = L2PageTableEntry(l3_phys, this->IsKernel(), true);
                        PteDataSynchronizationBarrier();
                        l2_open_count++;
                } else {
                    l3_virt = GetPageTableVirtualAddress(l3_phys);
                }
            }
            MESOSPHERE_ASSERT(l3_virt != Null<KVirtualAddress>);

            /* Map the page. */
            *impl.GetL3EntryFromTable(l3_virt, virt_addr) = L3PageTableEntry(phys_addr, entry_template, false);
            l3_open_count++;
            virt_addr += PageSize;
            phys_addr += PageSize;

            /* Account for hitting end of table. */
            if (util::IsAligned(GetInteger(virt_addr), L2BlockSize)) {
                if (this->GetPageTableManager().IsInPageTableHeap(l3_virt)) {
                    this->GetPageTableManager().Open(l3_virt, l3_open_count);
                }
                l3_virt = Null<KVirtualAddress>;
                l3_open_count = 0;

                if (util::IsAligned(GetInteger(virt_addr), L1BlockSize)) {
                    if (this->GetPageTableManager().IsInPageTableHeap(l2_virt) && l2_open_count > 0) {
                        this->GetPageTableManager().Open(l2_virt, l2_open_count);
                    }
                    l2_virt = Null<KVirtualAddress>;
                    l2_open_count = 0;
                }
            }
        }

        /* Perform any remaining opens. */
        if (l2_open_count > 0 && this->GetPageTableManager().IsInPageTableHeap(l2_virt)) {
            this->GetPageTableManager().Open(l2_virt, l2_open_count);
        }
        if (l3_open_count > 0 && this->GetPageTableManager().IsInPageTableHeap(l3_virt)) {
            this->GetPageTableManager().Open(l3_virt, l3_open_count);
        }

        return ResultSuccess();
    }

    Result KPageTable::Unmap(KProcessAddress virt_addr, size_t num_pages, PageLinkedList *page_list, bool force, bool reuse_ll) {
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

        auto &impl = this->GetImpl();

        /* If we're not forcing an unmap, separate pages immediately. */
        if (!force) {
            const size_t size = num_pages * PageSize;
            R_TRY(this->SeparatePages(virt_addr, std::min(GetInteger(virt_addr) & -GetInteger(virt_addr), size), page_list, reuse_ll));
            if (num_pages > 1) {
                const auto end_page  = virt_addr + size;
                const auto last_page = end_page - PageSize;

                auto merge_guard = SCOPE_GUARD { this->MergePages(virt_addr, page_list); };
                R_TRY(this->SeparatePages(last_page, std::min(GetInteger(end_page) & -GetInteger(end_page), size), page_list, reuse_ll));
                merge_guard.Cancel();
            }
        }

        /* Cache initial addresses for use on cleanup. */
        const KProcessAddress orig_virt_addr = virt_addr;
        size_t remaining_pages = num_pages;

        /* Ensure that any pages we track close on exit. */
        KPageGroup pages_to_close(this->GetBlockInfoManager());
        KScopedPageGroup spg(pages_to_close);

        /* Begin traversal. */
        TraversalContext context;
        TraversalEntry   next_entry;
        bool next_valid = impl.BeginTraversal(std::addressof(next_entry), std::addressof(context), virt_addr);

        while (remaining_pages > 0) {
            /* Handle the case where we're not valid. */
            if (!next_valid) {
                MESOSPHERE_ABORT_UNLESS(force);
                const size_t cur_size = std::min(next_entry.block_size - (GetInteger(virt_addr) & (next_entry.block_size - 1)), remaining_pages * PageSize);
                remaining_pages -= cur_size / PageSize;
                virt_addr += cur_size;
                continue;
            }

            /* Handle the case where the block is bigger than it should be. */
            if (next_entry.block_size > remaining_pages * PageSize) {
                MESOSPHERE_ABORT_UNLESS(force);
                MESOSPHERE_R_ABORT_UNLESS(this->SeparatePages(virt_addr, remaining_pages * PageSize, page_list, reuse_ll));
                next_valid = impl.BeginTraversal(std::addressof(next_entry), std::addressof(context), virt_addr);
                MESOSPHERE_ASSERT(next_valid);
            }

            /* Check that our state is coherent. */
            MESOSPHERE_ASSERT((next_entry.block_size / PageSize) <= remaining_pages);
            MESOSPHERE_ASSERT(util::IsAligned(GetInteger(next_entry.phys_addr), next_entry.block_size));

            /* Unmap the block. */
            L1PageTableEntry *l1_entry = impl.GetL1Entry(virt_addr);
            switch (next_entry.block_size) {
                case L1BlockSize:
                    {
                        /* Clear the entry. */
                        *l1_entry = InvalidL1PageTableEntry;
                    }
                    break;
                case L2ContiguousBlockSize:
                case L2BlockSize:
                    {
                        /* Get the number of L2 blocks. */
                        const size_t num_l2_blocks = next_entry.block_size / L2BlockSize;

                        /* Get the L2 entry. */
                        KPhysicalAddress l2_phys = Null<KPhysicalAddress>;
                        MESOSPHERE_ABORT_UNLESS(l1_entry->GetTable(l2_phys));
                        const KVirtualAddress  l2_virt = GetPageTableVirtualAddress(l2_phys);

                        /* Clear the entry. */
                        for (size_t i = 0; i < num_l2_blocks; i++) {
                            *impl.GetL2EntryFromTable(l2_virt, virt_addr + L2BlockSize * i) = InvalidL2PageTableEntry;
                        }
                        PteDataSynchronizationBarrier();

                        /* Close references to the L2 table. */
                        if (this->GetPageTableManager().IsInPageTableHeap(l2_virt)) {
                            if (this->GetPageTableManager().Close(l2_virt, num_l2_blocks)) {
                                *l1_entry = InvalidL1PageTableEntry;
                                this->NoteUpdated();
                                this->FreePageTable(page_list, l2_virt);
                            }
                        }
                    }
                    break;
                case L3ContiguousBlockSize:
                case L3BlockSize:
                    {
                        /* Get the number of L3 blocks. */
                        const size_t num_l3_blocks = next_entry.block_size / L3BlockSize;

                        /* Get the L2 entry. */
                        KPhysicalAddress l2_phys = Null<KPhysicalAddress>;
                        MESOSPHERE_ABORT_UNLESS(l1_entry->GetTable(l2_phys));
                        const KVirtualAddress  l2_virt = GetPageTableVirtualAddress(l2_phys);
                        L2PageTableEntry *l2_entry = impl.GetL2EntryFromTable(l2_virt, virt_addr);

                        /* Get the L3 entry. */
                        KPhysicalAddress l3_phys = Null<KPhysicalAddress>;
                        MESOSPHERE_ABORT_UNLESS(l2_entry->GetTable(l3_phys));
                        const KVirtualAddress  l3_virt = GetPageTableVirtualAddress(l3_phys);

                        /* Clear the entry. */
                        for (size_t i = 0; i < num_l3_blocks; i++) {
                            *impl.GetL3EntryFromTable(l3_virt, virt_addr + L3BlockSize * i) = InvalidL3PageTableEntry;
                        }
                        PteDataSynchronizationBarrier();

                        /* Close references to the L3 table. */
                        if (this->GetPageTableManager().IsInPageTableHeap(l3_virt)) {
                            if (this->GetPageTableManager().Close(l3_virt, num_l3_blocks)) {
                                *l2_entry = InvalidL2PageTableEntry;
                                this->NoteUpdated();

                                /* Close reference to the L2 table. */
                                if (this->GetPageTableManager().IsInPageTableHeap(l2_virt)) {
                                    if (this->GetPageTableManager().Close(l2_virt, 1)) {
                                        *l1_entry = InvalidL1PageTableEntry;
                                        this->NoteUpdated();
                                        this->FreePageTable(page_list, l2_virt);
                                    }
                                }

                                this->FreePageTable(page_list, l3_virt);
                            }
                        }
                    }
                    break;
            }

            /* Close the blocks. */
            if (!force && IsHeapPhysicalAddress(next_entry.phys_addr)) {
                const KVirtualAddress block_virt_addr = GetHeapVirtualAddress(next_entry.phys_addr);
                const size_t block_num_pages = next_entry.block_size / PageSize;
                if (R_FAILED(pages_to_close.AddBlock(block_virt_addr, block_num_pages))) {
                    this->NoteUpdated();
                    Kernel::GetMemoryManager().Close(block_virt_addr, block_num_pages);
                }
            }

            /* Advance. */
            virt_addr       += next_entry.block_size;
            remaining_pages -= next_entry.block_size / PageSize;
            next_valid       = impl.ContinueTraversal(std::addressof(next_entry), std::addressof(context));
        }

        /* Ensure we remain coherent. */
        if (this->IsKernel() && num_pages == 1) {
            this->NoteSingleKernelPageUpdated(orig_virt_addr);
        } else {
            this->NoteUpdated();
        }

        return ResultSuccess();
    }

    Result KPageTable::MapContiguous(KProcessAddress virt_addr, KPhysicalAddress phys_addr, size_t num_pages, PageTableEntry entry_template, PageLinkedList *page_list, bool reuse_ll) {
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

        /* Cache initial addresses for use on cleanup. */
        const KProcessAddress  orig_virt_addr = virt_addr;
        const KPhysicalAddress orig_phys_addr = phys_addr;

        size_t remaining_pages = num_pages;

        /* Map the pages, using a guard to ensure we don't leak. */
        {
            auto map_guard = SCOPE_GUARD { MESOSPHERE_R_ABORT_UNLESS(this->Unmap(orig_virt_addr, num_pages, page_list, true, true)); };

            if (num_pages < ContiguousPageSize / PageSize) {
                R_TRY(this->Map(virt_addr, phys_addr, num_pages, entry_template, L3BlockSize, page_list, reuse_ll));
                remaining_pages -= num_pages;
                virt_addr += num_pages * PageSize;
                phys_addr += num_pages * PageSize;
            } else {
                /* Map the fractional part of the pages. */
                size_t alignment;
                for (alignment = ContiguousPageSize; (virt_addr & (alignment - 1)) == (phys_addr & (alignment - 1)); alignment = GetLargerAlignment(alignment)) {
                    /* Check if this would be our last map. */
                    const size_t pages_to_map = (alignment - (virt_addr & (alignment - 1))) & (alignment - 1);
                    if (pages_to_map + (alignment / PageSize) > remaining_pages) {
                        break;
                    }

                    /* Map pages, if we should. */
                    if (pages_to_map > 0) {
                        R_TRY(this->Map(virt_addr, phys_addr, pages_to_map, entry_template, GetSmallerAlignment(alignment), page_list, reuse_ll));
                        remaining_pages -= pages_to_map;
                        virt_addr += pages_to_map * PageSize;
                        phys_addr += pages_to_map * PageSize;
                    }

                    /* Don't go further than L1 block. */
                    if (alignment == L1BlockSize) {
                        break;
                    }
                }

                while (remaining_pages > 0) {
                    /* Select the next smallest alignment. */
                    alignment = GetSmallerAlignment(alignment);
                    MESOSPHERE_ASSERT((virt_addr & (alignment - 1)) == 0);
                    MESOSPHERE_ASSERT((phys_addr & (alignment - 1)) == 0);

                    /* Map pages, if we should. */
                    const size_t pages_to_map = util::AlignDown(remaining_pages, alignment / PageSize);
                    if (pages_to_map > 0) {
                        R_TRY(this->Map(virt_addr, phys_addr, pages_to_map, entry_template, alignment, page_list, reuse_ll));
                        remaining_pages -= pages_to_map;
                        virt_addr += pages_to_map * PageSize;
                        phys_addr += pages_to_map * PageSize;
                    }
                }
            }

            map_guard.Cancel();
        }

        /* Perform what coalescing we can. */
        this->MergePages(orig_virt_addr, page_list);
        if (num_pages > 1) {
            this->MergePages(orig_virt_addr + (num_pages - 1) * PageSize, page_list);
        }

        /* Open references to the pages, if we should. */
        if (IsHeapPhysicalAddress(orig_phys_addr)) {
            Kernel::GetMemoryManager().Open(GetHeapVirtualAddress(orig_phys_addr), num_pages);
        }

        return ResultSuccess();
    }

    bool KPageTable::MergePages(KProcessAddress virt_addr, PageLinkedList *page_list) {
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

        auto &impl = this->GetImpl();
        bool merged = false;

        /* If there's no L1 table, don't bother. */
        L1PageTableEntry *l1_entry = impl.GetL1Entry(virt_addr);
        if (!l1_entry->IsTable()) {
            return merged;
        }

        /* Examine and try to merge the L2 table. */
        L2PageTableEntry *l2_entry = impl.GetL2Entry(l1_entry, virt_addr);
        if (l2_entry->IsTable()) {
            /* We have an L3 entry. */
            L3PageTableEntry *l3_entry = impl.GetL3Entry(l2_entry, virt_addr);
            if (!l3_entry->IsBlock() || !l3_entry->IsContiguousAllowed()) {
                return merged;
            }

            /* If it's not contiguous, try to make it so. */
            if (!l3_entry->IsContiguous()) {
                virt_addr = util::AlignDown(GetInteger(virt_addr), L3ContiguousBlockSize);
                KPhysicalAddress phys_addr = util::AlignDown(GetInteger(l3_entry->GetBlock()), L3ContiguousBlockSize);
                const u64 entry_template = l3_entry->GetEntryTemplate();

                /* Validate that we can merge. */
                for (size_t i = 0; i < L3ContiguousBlockSize / L3BlockSize; i++) {
                    if (!impl.GetL3Entry(l2_entry, virt_addr + L3BlockSize * i)->Is(entry_template | GetInteger(phys_addr + PageSize * i) | PageTableEntry::Type_L3Block)) {
                        return merged;
                    }
                }

                /* Merge! */
                for (size_t i = 0; i < L3ContiguousBlockSize / L3BlockSize; i++) {
                    impl.GetL3Entry(l2_entry, virt_addr + L3BlockSize * i)->SetContiguous(true);
                }

                /* Note that we updated. */
                this->NoteUpdated();
                merged = true;
            }

            /* We might be able to upgrade a contiguous set of L3 entries into an L2 block. */
            virt_addr = util::AlignDown(GetInteger(virt_addr), L2BlockSize);
            KPhysicalAddress phys_addr = util::AlignDown(GetInteger(l3_entry->GetBlock()), L2BlockSize);
            const u64 entry_template = l3_entry->GetEntryTemplate();

            /* Validate that we can merge. */
            for (size_t i = 0; i < L2BlockSize / L3ContiguousBlockSize; i++) {
                if (!impl.GetL3Entry(l2_entry, virt_addr + L3BlockSize * i)->Is(entry_template | GetInteger(phys_addr + L3ContiguousBlockSize * i) | PageTableEntry::ContigType_Contiguous)) {
                    return merged;
                }
            }

            /* Merge! */
            PteDataSynchronizationBarrier();
            *l2_entry = L2PageTableEntry(phys_addr, entry_template, false);

            /* Note that we updated. */
            this->NoteUpdated();
            merged = true;

            /* Free the L3 table. */
            KVirtualAddress l3_table = util::AlignDown(reinterpret_cast<uintptr_t>(l3_entry), PageSize);
            if (this->GetPageTableManager().IsInPageTableHeap(l3_table)) {
                this->GetPageTableManager().Close(l3_table, L2BlockSize / L3BlockSize);
                this->FreePageTable(page_list, l3_table);
            }
        }
        if (l2_entry->IsBlock()) {
            /* If it's not contiguous, try to make it so. */
            if (!l2_entry->IsContiguous()) {
                virt_addr = util::AlignDown(GetInteger(virt_addr), L2ContiguousBlockSize);
                KPhysicalAddress phys_addr = util::AlignDown(GetInteger(l2_entry->GetBlock()), L2ContiguousBlockSize);
                const u64 entry_template = l2_entry->GetEntryTemplate();

                /* Validate that we can merge. */
                for (size_t i = 0; i < L2ContiguousBlockSize / L2BlockSize; i++) {
                    if (!impl.GetL2Entry(l1_entry, virt_addr + L2BlockSize * i)->Is(entry_template | GetInteger(phys_addr + PageSize * i) | PageTableEntry::Type_L2Block)) {
                        return merged;
                    }
                }

                /* Merge! */
                for (size_t i = 0; i < L2ContiguousBlockSize / L2BlockSize; i++) {
                    impl.GetL2Entry(l1_entry, virt_addr + L2BlockSize * i)->SetContiguous(true);
                }

                /* Note that we updated. */
                this->NoteUpdated();
                merged = true;
            }

            /* We might be able to upgrade a contiguous set of L2 entries into an L1 block. */
            virt_addr = util::AlignDown(GetInteger(virt_addr), L1BlockSize);
            KPhysicalAddress phys_addr = util::AlignDown(GetInteger(l2_entry->GetBlock()), L1BlockSize);
            const u64 entry_template = l2_entry->GetEntryTemplate();

            /* Validate that we can merge. */
            for (size_t i = 0; i < L1BlockSize / L2ContiguousBlockSize; i++) {
                if (!impl.GetL2Entry(l1_entry, virt_addr + L3BlockSize * i)->Is(entry_template | GetInteger(phys_addr + L2ContiguousBlockSize * i) | PageTableEntry::ContigType_Contiguous)) {
                    return merged;
                }
            }

            /* Merge! */
            PteDataSynchronizationBarrier();
            *l1_entry = L1PageTableEntry(phys_addr, entry_template, false);

            /* Note that we updated. */
            this->NoteUpdated();
            merged = true;

            /* Free the L2 table. */
            KVirtualAddress l2_table = util::AlignDown(reinterpret_cast<uintptr_t>(l2_entry), PageSize);
            if (this->GetPageTableManager().IsInPageTableHeap(l2_table)) {
                this->GetPageTableManager().Close(l2_table, L1BlockSize / L2BlockSize);
                this->FreePageTable(page_list, l2_table);
            }
        }

        return merged;
    }

    Result KPageTable::SeparatePagesImpl(KProcessAddress virt_addr, size_t block_size, PageLinkedList *page_list, bool reuse_ll) {
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

        auto &impl = this->GetImpl();

        /* First, try to separate an L1 block into contiguous L2 blocks. */
        L1PageTableEntry *l1_entry = impl.GetL1Entry(virt_addr);
        if (l1_entry->IsBlock()) {
            /* If our block size is too big, don't bother. */
            R_UNLESS(block_size < L1BlockSize, ResultSuccess());

            /* Get the addresses we're working with. */
            const KProcessAddress block_virt_addr  = util::AlignDown(GetInteger(virt_addr), L1BlockSize);
            const KPhysicalAddress block_phys_addr = l1_entry->GetBlock();

            /* Allocate a new page for the L2 table. */
            const KVirtualAddress l2_table = this->AllocatePageTable(page_list, reuse_ll);
            R_UNLESS(l2_table != Null<KVirtualAddress>, svc::ResultOutOfResource());
            const KPhysicalAddress l2_phys = GetPageTablePhysicalAddress(l2_table);

            /* Set the entries in the L2 table. */
            const u64 entry_template = l1_entry->GetEntryTemplate();
            for (size_t i = 0; i < L1BlockSize / L2BlockSize; i++) {
                *(impl.GetL2EntryFromTable(l2_table, block_virt_addr + L2BlockSize * i)) = L2PageTableEntry(block_phys_addr + L2BlockSize * i, entry_template, true);
            }

            /* Open references to the L2 table. */
            Kernel::GetPageTableManager().Open(l2_table, L1BlockSize / L2BlockSize);

            /* Replace the L1 entry with one to the new table. */
            PteDataSynchronizationBarrier();
            *l1_entry = L1PageTableEntry(l2_phys, this->IsKernel(), true);
            this->NoteUpdated();
        }

        /* If we don't have an l1 table, we're done. */
        R_UNLESS(l1_entry->IsTable(), ResultSuccess());

        /* We want to separate L2 contiguous blocks into L2 blocks, so check that our size permits that. */
        R_UNLESS(block_size < L2ContiguousBlockSize, ResultSuccess());

        L2PageTableEntry *l2_entry = impl.GetL2Entry(l1_entry, virt_addr);
        if (l2_entry->IsBlock()) {
            /* If we're contiguous, try to separate. */
            if (l2_entry->IsContiguous()) {
                const KProcessAddress block_virt_addr  = util::AlignDown(GetInteger(virt_addr), L2ContiguousBlockSize);

                /* Mark the entries as non-contiguous. */
                for (size_t i = 0; i < L2ContiguousBlockSize / L2BlockSize; i++) {
                    impl.GetL2Entry(l1_entry, block_virt_addr + L2BlockSize * i)->SetContiguous(false);
                }
                this->NoteUpdated();
            }

            /* We want to separate L2 blocks into L3 contiguous blocks, so check that our size permits that. */
            R_UNLESS(block_size < L2BlockSize, ResultSuccess());

            /* Get the addresses we're working with. */
            const KProcessAddress block_virt_addr  = util::AlignDown(GetInteger(virt_addr), L2BlockSize);
            const KPhysicalAddress block_phys_addr = l2_entry->GetBlock();

            /* Allocate a new page for the L3 table. */
            const KVirtualAddress l3_table = this->AllocatePageTable(page_list, reuse_ll);
            R_UNLESS(l3_table != Null<KVirtualAddress>, svc::ResultOutOfResource());
            const KPhysicalAddress l3_phys = GetPageTablePhysicalAddress(l3_table);

            /* Set the entries in the L3 table. */
            const u64 entry_template = l2_entry->GetEntryTemplate();
            for (size_t i = 0; i < L2BlockSize / L3BlockSize; i++) {
                *(impl.GetL3EntryFromTable(l3_table, block_virt_addr + L3BlockSize * i)) = L3PageTableEntry(block_phys_addr + L3BlockSize * i, entry_template, true);
            }

            /* Open references to the L3 table. */
            Kernel::GetPageTableManager().Open(l3_table, L2BlockSize / L3BlockSize);

            /* Replace the L2 entry with one to the new table. */
            PteDataSynchronizationBarrier();
            *l2_entry = L2PageTableEntry(l3_phys, this->IsKernel(), true);
            this->NoteUpdated();
        }

        /* If we don't have an L3 table, we're done. */
        R_UNLESS(l2_entry->IsTable(), ResultSuccess());

        /* We want to separate L3 contiguous blocks into L2 blocks, so check that our size permits that. */
        R_UNLESS(block_size < L3ContiguousBlockSize, ResultSuccess());

        /* If we're contiguous, try to separate. */
        L3PageTableEntry *l3_entry = impl.GetL3Entry(l2_entry, virt_addr);
        if (l3_entry->IsBlock() && l3_entry->IsContiguous()) {
            const KProcessAddress block_virt_addr  = util::AlignDown(GetInteger(virt_addr), L3ContiguousBlockSize);

            /* Mark the entries as non-contiguous. */
            for (size_t i = 0; i < L3ContiguousBlockSize / L3BlockSize; i++) {
                impl.GetL3Entry(l2_entry, block_virt_addr + L3BlockSize * i)->SetContiguous(false);
            }
            this->NoteUpdated();
        }

        /* We're done! */
        return ResultSuccess();
    }

    Result KPageTable::SeparatePages(KProcessAddress virt_addr, size_t block_size, PageLinkedList *page_list, bool reuse_ll) {
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

        /* Try to separate pages, re-merging if we fail. */
        auto guard = SCOPE_GUARD { this->MergePages(virt_addr, page_list); };
        R_TRY(this->SeparatePagesImpl(virt_addr, block_size, page_list, reuse_ll));
        guard.Cancel();

        return ResultSuccess();
    }

    void KPageTable::FinalizeUpdate(PageLinkedList *page_list) {
        while (page_list->Peek()) {
            KVirtualAddress page = KVirtualAddress(page_list->Pop());
            MESOSPHERE_ASSERT(this->GetPageTableManager().IsInPageTableHeap(page));
            MESOSPHERE_ASSERT(this->GetPageTableManager().GetRefCount(page) == 0);
            this->GetPageTableManager().Free(page);
        }
    }

}
