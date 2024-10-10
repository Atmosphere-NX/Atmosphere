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

    namespace {

        class AlignedMemoryBlock {
            private:
                uintptr_t m_before_start;
                uintptr_t m_before_end;
                uintptr_t m_after_start;
                uintptr_t m_after_end;
                size_t m_current_alignment;
            public:
                constexpr AlignedMemoryBlock(uintptr_t start, size_t num_pages, size_t alignment) : m_before_start(0), m_before_end(0), m_after_start(0), m_after_end(0), m_current_alignment(0) {
                    MESOSPHERE_ASSERT(util::IsAligned(start, PageSize));
                    MESOSPHERE_ASSERT(num_pages > 0);

                    /* Find an alignment that allows us to divide into at least two regions.*/
                    uintptr_t start_page = start / PageSize;
                    alignment /= PageSize;
                    while (util::AlignUp(start_page, alignment) >= util::AlignDown(start_page + num_pages, alignment)) {
                        alignment = KPageTable::GetSmallerAlignment(alignment * PageSize) / PageSize;
                    }

                    m_before_start      = start_page;
                    m_before_end        = util::AlignUp(start_page, alignment);
                    m_after_start       = m_before_end;
                    m_after_end         = start_page + num_pages;
                    m_current_alignment = alignment;
                    MESOSPHERE_ASSERT(m_current_alignment > 0);
                }

                constexpr void SetAlignment(size_t alignment) {
                    /* We can only ever decrease the granularity. */
                    MESOSPHERE_ASSERT(m_current_alignment >= alignment / PageSize);
                    m_current_alignment = alignment / PageSize;
                }

                constexpr size_t GetAlignment() const {
                    return m_current_alignment * PageSize;
                }

                constexpr void FindBlock(uintptr_t &out, size_t &num_pages) {
                    if ((m_after_end - m_after_start) >= m_current_alignment) {
                        /* Select aligned memory from after block. */
                        const size_t available_pages = util::AlignDown(m_after_end, m_current_alignment) - m_after_start;
                        if (num_pages == 0 || available_pages < num_pages) {
                            num_pages = available_pages;
                        }
                        out = m_after_start * PageSize;
                        m_after_start += num_pages;
                    } else if ((m_before_end - m_before_start) >= m_current_alignment) {
                        /* Select aligned memory from before block. */
                        const size_t available_pages = m_before_end - util::AlignUp(m_before_start, m_current_alignment);
                        if (num_pages == 0 || available_pages < num_pages) {
                            num_pages = available_pages;
                        }
                        m_before_end -= num_pages;
                        out = m_before_end * PageSize;
                    } else {
                        /* Neither after or before can get an aligned bit of memory. */
                        out = 0;
                        num_pages = 0;
                    }
                }
        };

        constexpr u64 EncodeTtbr(KPhysicalAddress table, u8 asid) {
            return (static_cast<u64>(asid) << 48) | (static_cast<u64>(GetInteger(table)));
        }

    }

    ALWAYS_INLINE void KPageTable::NoteUpdated() const {
        cpu::DataSynchronizationBarrierInnerShareableStore();

        /* Mark ourselves as in a tlb maintenance operation. */
        GetCurrentThread().SetInTlbMaintenanceOperation();
        ON_SCOPE_EXIT { GetCurrentThread().ClearInTlbMaintenanceOperation(); __asm__ __volatile__("" ::: "memory"); };

        if (this->IsKernel()) {
            this->OnKernelTableUpdated();
        } else {
            this->OnTableUpdated();
        }
    }

    ALWAYS_INLINE void KPageTable::NoteSingleKernelPageUpdated(KProcessAddress virt_addr) const {
        MESOSPHERE_ASSERT(this->IsKernel());

        cpu::DataSynchronizationBarrierInnerShareableStore();

        /* Mark ourselves as in a tlb maintenance operation. */
        GetCurrentThread().SetInTlbMaintenanceOperation();
        ON_SCOPE_EXIT { GetCurrentThread().ClearInTlbMaintenanceOperation(); __asm__ __volatile__("" ::: "memory"); };

        this->OnKernelTableSinglePageUpdated(virt_addr);
    }


    void KPageTable::Initialize(s32 core_id) {
        /* Nothing actually needed here. */
        MESOSPHERE_UNUSED(core_id);
    }

    Result KPageTable::InitializeForKernel(void *table, KVirtualAddress start, KVirtualAddress end) {
        /* Initialize basic fields. */
        m_asid = 0;
        m_manager = Kernel::GetSystemSystemResource().GetPageTableManagerPointer();

        /* Initialize the base page table. */
        MESOSPHERE_R_ABORT_UNLESS(KPageTableBase::InitializeForKernel(true, table, start, end));

        R_SUCCEED();
    }

    Result KPageTable::InitializeForProcess(ams::svc::CreateProcessFlag flags, bool from_back, KMemoryManager::Pool pool, KProcessAddress code_address, size_t code_size, KSystemResource *system_resource, KResourceLimit *resource_limit, size_t process_index) {
        /* Determine our ASID */
        m_asid = process_index + 1;
        MESOSPHERE_ABORT_UNLESS(0 < m_asid && m_asid < util::size(s_ttbr0_entries));

        /* Set our manager. */
        m_manager = system_resource->GetPageTableManagerPointer();

        /* Get the virtual address of our L1 table. */
        const KPhysicalAddress ttbr0_phys = KPhysicalAddress(s_ttbr0_entries[m_asid] & UINT64_C(0xFFFFFFFFFFFE));
        const KVirtualAddress  ttbr0_virt = KMemoryLayout::GetLinearVirtualAddress(ttbr0_phys);

        /* Initialize our base table. */
        const size_t as_width = GetAddressSpaceWidth(flags);
        const KProcessAddress as_start = 0;
        const KProcessAddress as_end   = (1ul << as_width);
        R_TRY(KPageTableBase::InitializeForProcess(flags, from_back, pool, GetVoidPointer(ttbr0_virt), as_start, as_end, code_address, code_size, system_resource, resource_limit));

        /* Note that we've updated the table (since we created it). */
        this->NoteUpdated();
        R_SUCCEED();
    }

    Result KPageTable::Finalize() {
        /* Only process tables should be finalized. */
        MESOSPHERE_ASSERT(!this->IsKernel());

        /* NOTE: Here Nintendo calls an unknown OnFinalize function. */
        /* this->OnFinalize(); */

        /* Note that we've updated (to ensure we're synchronized). */
        this->NoteUpdated();

        /* NOTE: Here Nintendo calls a second unknown OnFinalize function. */
        /* this->OnFinalize2(); */

        /* Free all pages in the table. */
        {
            /* Get implementation objects. */
            auto &impl = this->GetImpl();
            auto &mm   = Kernel::GetMemoryManager();

            /* Traverse, freeing all pages. */
            {
                /* Get the address space size. */
                const size_t as_size = this->GetAddressSpaceSize();

                /* Begin the traversal. */
                TraversalContext context;
                TraversalEntry   cur_entry  = { .phys_addr = Null<KPhysicalAddress>, .block_size = 0, .sw_reserved_bits = 0, .attr = 0 };
                bool             cur_valid  = false;
                TraversalEntry   next_entry;
                bool             next_valid;
                size_t           tot_size   = 0;

                next_valid = impl.BeginTraversal(std::addressof(next_entry), std::addressof(context), this->GetAddressSpaceStart());

                /* Iterate over entries. */
                while (true) {
                    /* NOTE: Nintendo really does check next_entry.attr == (cur_entry.attr != 0)...but attr is always zero as of 18.0.0, and this is "probably" for the new console or debug-only anyway, */
                    /* so we'll implement the weird logic verbatim even though it doesn't match the GetContiguousRange logic. */
                    if ((!next_valid && !cur_valid) || (next_valid && cur_valid && next_entry.phys_addr == cur_entry.phys_addr + cur_entry.block_size && next_entry.attr == (cur_entry.attr ? 1 : 0))) {
                        cur_entry.block_size += next_entry.block_size;
                    } else {
                        if (cur_valid && IsHeapPhysicalAddressForFinalize(cur_entry.phys_addr)) {
                            mm.Close(cur_entry.phys_addr, cur_entry.block_size / PageSize);
                        }

                        /* Update tracking variables. */
                        tot_size += cur_entry.block_size;
                        cur_entry = next_entry;
                        cur_valid = next_valid;
                    }

                    if (cur_entry.block_size + tot_size >= as_size) {
                        break;
                    }

                    next_valid = impl.ContinueTraversal(std::addressof(next_entry), std::addressof(context));
                }

                /* Handle the last block. */
                if (cur_valid && IsHeapPhysicalAddressForFinalize(cur_entry.phys_addr)) {
                    mm.Close(cur_entry.phys_addr, cur_entry.block_size / PageSize);
                }
            }

            /* Cache address space extents for convenience. */
            const KProcessAddress as_start = this->GetAddressSpaceStart();
            const KProcessAddress as_last  = as_start + this->GetAddressSpaceSize() - 1;

            /* Free all L3 tables. */
            for (KProcessAddress cur_address = as_start; cur_address <= as_last; cur_address += L2BlockSize) {
                L1PageTableEntry *l1_entry = impl.GetL1Entry(cur_address);
                if (l1_entry->IsTable()) {
                    L2PageTableEntry *l2_entry = impl.GetL2Entry(l1_entry, cur_address);
                    if (l2_entry->IsTable()) {
                        const KVirtualAddress l3_table = GetPageTableVirtualAddress(l2_entry->GetTable());
                        if (this->GetPageTableManager().IsInPageTableHeap(l3_table)) {
                            while (!this->GetPageTableManager().Close(l3_table, 1)) { /* ... */ }
                            ClearPageTable(l3_table);
                            this->GetPageTableManager().Free(l3_table);
                        }
                    }
                }
            }

            /* Free all L2 tables. */
            for (KProcessAddress cur_address = as_start; cur_address <= as_last; cur_address += L1BlockSize) {
                L1PageTableEntry *l1_entry = impl.GetL1Entry(cur_address);
                if (l1_entry->IsTable()) {
                    const KVirtualAddress l2_table = GetPageTableVirtualAddress(l1_entry->GetTable());
                    if (this->GetPageTableManager().IsInPageTableHeap(l2_table)) {
                        while (!this->GetPageTableManager().Close(l2_table, 1)) { /* ... */ }
                        ClearPageTable(l2_table);
                        this->GetPageTableManager().Free(l2_table);
                    }
                }
            }

            /* Clear the L1 table. */
            {
                const KVirtualAddress l1_table = reinterpret_cast<uintptr_t>(impl.Finalize());
                ClearPageTable(l1_table);
            }

            /* Perform inherited finalization. */
            KPageTableBase::Finalize();
        }

        R_SUCCEED();
    }

    Result KPageTable::OperateImpl(PageLinkedList *page_list, KProcessAddress virt_addr, size_t num_pages, KPhysicalAddress phys_addr, bool is_pa_valid, const KPageProperties properties, OperationType operation, bool reuse_ll) {
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
            R_RETURN(this->Unmap(virt_addr, num_pages, page_list, false, reuse_ll));
        } else if (operation == OperationType_Separate) {
            R_RETURN(this->SeparatePages(virt_addr, num_pages, page_list, reuse_ll));
        } else {
            auto entry_template = this->GetEntryTemplate(properties);

            switch (operation) {
                case OperationType_Map:
                    R_RETURN(this->MapContiguous(virt_addr, phys_addr, num_pages, entry_template, properties.disable_merge_attributes == DisableMergeAttribute_DisableHead, page_list, reuse_ll));
                case OperationType_ChangePermissions:
                    R_RETURN(this->ChangePermissions(virt_addr, num_pages, entry_template, properties.disable_merge_attributes, false, false, page_list, reuse_ll));
                case OperationType_ChangePermissionsAndRefresh:
                    R_RETURN(this->ChangePermissions(virt_addr, num_pages, entry_template, properties.disable_merge_attributes, true, false, page_list, reuse_ll));
                case OperationType_ChangePermissionsAndRefreshAndFlush:
                    R_RETURN(this->ChangePermissions(virt_addr, num_pages, entry_template, properties.disable_merge_attributes, true, true, page_list, reuse_ll));
                MESOSPHERE_UNREACHABLE_DEFAULT_CASE();
            }
        }
    }

    Result KPageTable::OperateImpl(PageLinkedList *page_list, KProcessAddress virt_addr, size_t num_pages, const KPageGroup &page_group, const KPageProperties properties, OperationType operation, bool reuse_ll) {
        /* Check validity of parameters. */
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());
        MESOSPHERE_ASSERT(util::IsAligned(GetInteger(virt_addr), PageSize));
        MESOSPHERE_ASSERT(num_pages > 0);
        MESOSPHERE_ASSERT(num_pages == page_group.GetNumPages());

        /* Map the page group. */
        auto entry_template = this->GetEntryTemplate(properties);
        switch (operation) {
            case OperationType_MapGroup:
            case OperationType_MapFirstGroup:
                R_RETURN(this->MapGroup(virt_addr, page_group, num_pages, entry_template, properties.disable_merge_attributes == DisableMergeAttribute_DisableHead, operation != OperationType_MapFirstGroup, page_list, reuse_ll));
            MESOSPHERE_UNREACHABLE_DEFAULT_CASE();
        }
    }

    Result KPageTable::MapL1Blocks(KProcessAddress virt_addr, KPhysicalAddress phys_addr, size_t num_pages, PageTableEntry entry_template, bool disable_head_merge, PageLinkedList *page_list, bool reuse_ll) {
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());
        MESOSPHERE_ASSERT(util::IsAligned(GetInteger(virt_addr), L1BlockSize));
        MESOSPHERE_ASSERT(util::IsAligned(GetInteger(phys_addr), L1BlockSize));
        MESOSPHERE_ASSERT(util::IsAligned(num_pages * PageSize,  L1BlockSize));

        /* Allocation is never needed for L1 block mapping. */
        MESOSPHERE_UNUSED(page_list, reuse_ll);

        auto &impl = this->GetImpl();

        u8 sw_reserved_bits = PageTableEntry::EncodeSoftwareReservedBits(disable_head_merge, false, false);

        /* Iterate, mapping each block. */
        for (size_t i = 0; i < num_pages; i += L1BlockSize / PageSize) {
            /* Map the block. */
            *impl.GetL1Entry(virt_addr) = L1PageTableEntry(PageTableEntry::BlockTag{}, phys_addr, PageTableEntry(entry_template), sw_reserved_bits, false);
            sw_reserved_bits &= ~(PageTableEntry::SoftwareReservedBit_DisableMergeHead);
            virt_addr += L1BlockSize;
            phys_addr += L1BlockSize;
        }

        R_SUCCEED();
    }

    Result KPageTable::MapL2Blocks(KProcessAddress virt_addr, KPhysicalAddress phys_addr, size_t num_pages, PageTableEntry entry_template, bool disable_head_merge, PageLinkedList *page_list, bool reuse_ll) {
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());
        MESOSPHERE_ASSERT(util::IsAligned(GetInteger(virt_addr), L2BlockSize));
        MESOSPHERE_ASSERT(util::IsAligned(GetInteger(phys_addr), L2BlockSize));
        MESOSPHERE_ASSERT(util::IsAligned(num_pages * PageSize,  L2BlockSize));

        auto &impl = this->GetImpl();
        KVirtualAddress l2_virt = Null<KVirtualAddress>;
        int l2_open_count = 0;

        u8 sw_reserved_bits = PageTableEntry::EncodeSoftwareReservedBits(disable_head_merge, false, false);

        /* Iterate, mapping each block. */
        for (size_t i = 0; i < num_pages; i += L2BlockSize / PageSize) {
            KPhysicalAddress l2_phys = Null<KPhysicalAddress>;

            /* If we have no L2 table, we should get or allocate one. */
            if (l2_virt == Null<KVirtualAddress>) {
                if (L1PageTableEntry *l1_entry = impl.GetL1Entry(virt_addr); !l1_entry->GetTable(l2_phys)) {
                    /* Allocate table. */
                    l2_virt = AllocatePageTable(page_list, reuse_ll);
                    R_UNLESS(l2_virt != Null<KVirtualAddress>, svc::ResultOutOfResource());

                    /* Set the entry. */
                    l2_phys = GetPageTablePhysicalAddress(l2_virt);
                    PteDataMemoryBarrier();
                    *l1_entry = L1PageTableEntry(PageTableEntry::TableTag{}, l2_phys, this->IsKernel(), true);
                } else {
                    l2_virt = GetPageTableVirtualAddress(l2_phys);
                }
            }
            MESOSPHERE_ASSERT(l2_virt != Null<KVirtualAddress>);

            /* Map the block. */
            *impl.GetL2EntryFromTable(l2_virt, virt_addr) = L2PageTableEntry(PageTableEntry::BlockTag{}, phys_addr, PageTableEntry(entry_template), sw_reserved_bits, false);
            sw_reserved_bits &= ~(PageTableEntry::SoftwareReservedBit_DisableMergeHead);
            l2_open_count++;
            virt_addr += L2BlockSize;
            phys_addr += L2BlockSize;

            /* Account for hitting end of table. */
            if (util::IsAligned(GetInteger(virt_addr), L1BlockSize)) {
                if (this->GetPageTableManager().IsInPageTableHeap(l2_virt)) {
                    this->GetPageTableManager().Open(l2_virt, l2_open_count);
                }
                l2_virt = Null<KVirtualAddress>;
                l2_open_count = 0;
            }
        }

        /* Perform any remaining opens. */
        if (l2_open_count > 0 && this->GetPageTableManager().IsInPageTableHeap(l2_virt)) {
            this->GetPageTableManager().Open(l2_virt, l2_open_count);
        }

        R_SUCCEED();
    }

    Result KPageTable::MapL3Blocks(KProcessAddress virt_addr, KPhysicalAddress phys_addr, size_t num_pages, PageTableEntry entry_template, bool disable_head_merge, PageLinkedList *page_list, bool reuse_ll) {
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());
        MESOSPHERE_ASSERT(util::IsAligned(GetInteger(virt_addr), PageSize));
        MESOSPHERE_ASSERT(util::IsAligned(GetInteger(phys_addr), PageSize));

        auto &impl = this->GetImpl();
        KVirtualAddress l2_virt = Null<KVirtualAddress>;
        KVirtualAddress l3_virt = Null<KVirtualAddress>;
        int l2_open_count = 0;
        int l3_open_count = 0;

        u8 sw_reserved_bits = PageTableEntry::EncodeSoftwareReservedBits(disable_head_merge, false, false);

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
                        PteDataMemoryBarrier();
                        *l1_entry = L1PageTableEntry(PageTableEntry::TableTag{}, l2_phys, this->IsKernel(), true);
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

                            R_THROW(svc::ResultOutOfResource());
                        }

                        /* Set the entry. */
                        l3_phys = GetPageTablePhysicalAddress(l3_virt);
                        PteDataMemoryBarrier();
                        *l2_entry = L2PageTableEntry(PageTableEntry::TableTag{}, l3_phys, this->IsKernel(), true);
                        l2_open_count++;
                } else {
                    l3_virt = GetPageTableVirtualAddress(l3_phys);
                }
            }
            MESOSPHERE_ASSERT(l3_virt != Null<KVirtualAddress>);

            /* Map the page. */
            *impl.GetL3EntryFromTable(l3_virt, virt_addr) = L3PageTableEntry(PageTableEntry::BlockTag{}, phys_addr, PageTableEntry(entry_template), sw_reserved_bits, false);
            sw_reserved_bits &= ~(PageTableEntry::SoftwareReservedBit_DisableMergeHead);
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

        R_SUCCEED();
    }

    Result KPageTable::Unmap(KProcessAddress virt_addr, size_t num_pages, PageLinkedList *page_list, bool force, bool reuse_ll) {
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

        auto &impl = this->GetImpl();

        /* If we're not forcing an unmap, separate pages immediately. */
        if (!force) {
            R_TRY(this->SeparatePages(virt_addr, num_pages, page_list, reuse_ll));
        }

        /* Cache initial addresses for use on cleanup. */
        const KProcessAddress orig_virt_addr = virt_addr;
        size_t remaining_pages = num_pages;

        /* Ensure that any pages we track close on exit. */
        KPageGroup pages_to_close(this->GetBlockInfoManager());
        ON_SCOPE_EXIT { pages_to_close.CloseAndReset(); };

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
                virt_addr       += cur_size;
                next_valid       = impl.ContinueTraversal(std::addressof(next_entry), std::addressof(context));
                continue;
            }

            /* Handle the case where the block is bigger than it should be. */
            if (next_entry.block_size > remaining_pages * PageSize) {
                MESOSPHERE_ABORT_UNLESS(force);
                MESOSPHERE_R_ABORT_UNLESS(this->SeparatePagesImpl(std::addressof(next_entry), std::addressof(context), virt_addr, remaining_pages * PageSize, page_list, reuse_ll));
            }

            /* Check that our state is coherent. */
            MESOSPHERE_ASSERT((next_entry.block_size / PageSize) <= remaining_pages);
            MESOSPHERE_ASSERT(util::IsAligned(GetInteger(next_entry.phys_addr), next_entry.block_size));

            /* Unmap the block. */
            bool freeing_table = false;
            while (true) {
                /* Clear the entries. */
                const size_t num_to_clear = (!freeing_table && context.is_contiguous) ? BlocksPerContiguousBlock : 1;
                auto *pte = reinterpret_cast<PageTableEntry *>(context.is_contiguous ? util::AlignDown(reinterpret_cast<uintptr_t>(context.level_entries[context.level]), BlocksPerContiguousBlock * sizeof(PageTableEntry)) : reinterpret_cast<uintptr_t>(context.level_entries[context.level]));
                for (size_t i = 0; i < num_to_clear; ++i) {
                    pte[i] = InvalidPageTableEntry;
                }

                /* Remove the entries from the previous table. */
                if (context.level != KPageTableImpl::EntryLevel_L1) {
                    context.level_entries[context.level + 1]->RemoveTableEntries(num_to_clear);
                }

                /* If we cleared a table, we need to note that we updated and free the table. */
                if (freeing_table) {
                    this->NoteUpdated();
                    this->FreePageTable(page_list, KVirtualAddress(util::AlignDown(reinterpret_cast<uintptr_t>(context.level_entries[context.level - 1]), PageSize)));
                }

                /* Advance; we're no longer contiguous. */
                context.is_contiguous                = false;
                context.level_entries[context.level] = pte + num_to_clear - 1;

                /* We may have removed the last entries in a table, in which case we can free an unmap the tables. */
                if (context.level >= KPageTableImpl::EntryLevel_L1 || context.level_entries[context.level + 1]->GetTableNumEntries() != 0) {
                    break;
                }

                /* Advance; we will not be working with blocks any more. */
                context.level = static_cast<KPageTableImpl::EntryLevel>(util::ToUnderlying(context.level) + 1);
                freeing_table = true;
            }

            /* Close the blocks. */
            if (!force && IsHeapPhysicalAddress(next_entry.phys_addr)) {
                const size_t block_num_pages = next_entry.block_size / PageSize;
                if (R_FAILED(pages_to_close.AddBlock(next_entry.phys_addr, block_num_pages))) {
                    this->NoteUpdated();
                    Kernel::GetMemoryManager().Close(next_entry.phys_addr, block_num_pages);
                    pages_to_close.CloseAndReset();
                }
            }

            /* Advance. */
            size_t freed_size = next_entry.block_size;
            if (freeing_table) {
                /* We advanced more than by the block, so we need to calculate the actual advanced size. */
                const KProcessAddress new_virt_addr = util::AlignUp(GetInteger(virt_addr), impl.GetBlockSize(context.level, context.is_contiguous));
                MESOSPHERE_ABORT_UNLESS(new_virt_addr >= virt_addr + next_entry.block_size);

                freed_size = std::min<size_t>(new_virt_addr - virt_addr, remaining_pages * PageSize);
            }

            /* We can just advance by the block size. */
            virt_addr       += freed_size;
            remaining_pages -= freed_size / PageSize;

            next_valid       = impl.ContinueTraversal(std::addressof(next_entry), std::addressof(context));
        }

        /* Ensure we remain coherent. */
        if (this->IsKernel() && num_pages == 1) {
            this->NoteSingleKernelPageUpdated(orig_virt_addr);
        } else {
            this->NoteUpdated();
        }

        R_SUCCEED();
    }

    Result KPageTable::MapContiguous(KProcessAddress virt_addr, KPhysicalAddress phys_addr, size_t num_pages, PageTableEntry entry_template, bool disable_head_merge, PageLinkedList *page_list, bool reuse_ll) {
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

        /* Cache initial addresses for use on cleanup. */
        const KProcessAddress  orig_virt_addr = virt_addr;
        const KPhysicalAddress orig_phys_addr = phys_addr;

        size_t remaining_pages = num_pages;

        /* Map the pages, using a guard to ensure we don't leak. */
        {
            ON_RESULT_FAILURE { MESOSPHERE_R_ABORT_UNLESS(this->Unmap(orig_virt_addr, num_pages, page_list, true, true)); };

            if (num_pages < ContiguousPageSize / PageSize) {
                R_TRY(this->Map(virt_addr, phys_addr, num_pages, entry_template, disable_head_merge && virt_addr == orig_virt_addr, L3BlockSize, page_list, reuse_ll));
                remaining_pages -= num_pages;
                virt_addr += num_pages * PageSize;
                phys_addr += num_pages * PageSize;
            } else {
                /* Map the fractional part of the pages. */
                size_t alignment;
                for (alignment = ContiguousPageSize; (virt_addr & (alignment - 1)) == (phys_addr & (alignment - 1)); alignment = GetLargerAlignment(alignment)) {
                    /* Check if this would be our last map. */
                    const size_t pages_to_map = ((alignment - (virt_addr & (alignment - 1))) & (alignment - 1)) / PageSize;
                    if (pages_to_map + (alignment / PageSize) > remaining_pages) {
                        break;
                    }

                    /* Map pages, if we should. */
                    if (pages_to_map > 0) {
                        R_TRY(this->Map(virt_addr, phys_addr, pages_to_map, entry_template, disable_head_merge && virt_addr == orig_virt_addr, GetSmallerAlignment(alignment), page_list, reuse_ll));
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
                        R_TRY(this->Map(virt_addr, phys_addr, pages_to_map, entry_template, disable_head_merge && virt_addr == orig_virt_addr, alignment, page_list, reuse_ll));
                        remaining_pages -= pages_to_map;
                        virt_addr += pages_to_map * PageSize;
                        phys_addr += pages_to_map * PageSize;
                    }
                }
            }
        }

        /* Perform what coalescing we can. */
        this->MergePages(orig_virt_addr, num_pages, page_list);

        /* Open references to the pages, if we should. */
        if (IsHeapPhysicalAddress(orig_phys_addr)) {
            Kernel::GetMemoryManager().Open(orig_phys_addr, num_pages);
        }

        R_SUCCEED();
    }

    Result KPageTable::MapGroup(KProcessAddress virt_addr, const KPageGroup &pg, size_t num_pages, PageTableEntry entry_template, bool disable_head_merge, bool not_first, PageLinkedList *page_list, bool reuse_ll) {
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

        /* We want to maintain a new reference to every page in the group. */
        KScopedPageGroup spg(pg, not_first);

        /* Cache initial address for use on cleanup. */
        const KProcessAddress  orig_virt_addr = virt_addr;

        size_t mapped_pages = 0;

        /* Map the pages, using a guard to ensure we don't leak. */
        {
            ON_RESULT_FAILURE { MESOSPHERE_R_ABORT_UNLESS(this->Unmap(orig_virt_addr, num_pages, page_list, true, true)); };

            if (num_pages < ContiguousPageSize / PageSize) {
                for (const auto &block : pg) {
                    const KPhysicalAddress block_phys_addr = block.GetAddress();
                    const size_t cur_pages = block.GetNumPages();
                    R_TRY(this->Map(virt_addr, block_phys_addr, cur_pages, entry_template, disable_head_merge && virt_addr == orig_virt_addr, L3BlockSize, page_list, reuse_ll));

                    virt_addr    += cur_pages * PageSize;
                    mapped_pages += cur_pages;
                }
            } else {
                /* Create a block representing our virtual space. */
                AlignedMemoryBlock virt_block(GetInteger(virt_addr), num_pages, L1BlockSize);
                for (const auto &block : pg) {
                    /* Create a block representing this physical group, synchronize its alignment to our virtual block. */
                    const KPhysicalAddress block_phys_addr = block.GetAddress();
                    size_t cur_pages = block.GetNumPages();

                    AlignedMemoryBlock phys_block(GetInteger(block_phys_addr), cur_pages, virt_block.GetAlignment());
                    virt_block.SetAlignment(phys_block.GetAlignment());

                    while (cur_pages > 0) {
                        /* Find a physical region for us to map at. */
                        uintptr_t phys_choice = 0;
                        size_t phys_pages = 0;
                        phys_block.FindBlock(phys_choice, phys_pages);

                        /* If we didn't find a region, try decreasing our alignment. */
                        if (phys_pages == 0) {
                            const size_t next_alignment = KPageTable::GetSmallerAlignment(phys_block.GetAlignment());
                            MESOSPHERE_ASSERT(next_alignment >= PageSize);
                            phys_block.SetAlignment(next_alignment);
                            virt_block.SetAlignment(next_alignment);
                            continue;
                        }

                        /* Begin choosing virtual blocks to map at the region we chose. */
                        while (phys_pages > 0) {
                            /* Find a virtual region for us to map at. */
                            uintptr_t virt_choice = 0;
                            size_t virt_pages = phys_pages;
                            virt_block.FindBlock(virt_choice, virt_pages);

                            /* If we didn't find a region, try decreasing our alignment. */
                            if (virt_pages == 0) {
                                const size_t next_alignment = KPageTable::GetSmallerAlignment(virt_block.GetAlignment());
                                MESOSPHERE_ASSERT(next_alignment >= PageSize);
                                phys_block.SetAlignment(next_alignment);
                                virt_block.SetAlignment(next_alignment);
                                continue;
                            }

                            /* Map! */
                            R_TRY(this->Map(virt_choice, phys_choice, virt_pages, entry_template, disable_head_merge && virt_addr == orig_virt_addr, virt_block.GetAlignment(), page_list, reuse_ll));

                            /* Advance. */
                            phys_choice  += virt_pages * PageSize;
                            phys_pages   -= virt_pages;
                            cur_pages    -= virt_pages;
                            mapped_pages += virt_pages;
                        }
                    }
                }
            }
        }
        MESOSPHERE_ASSERT(mapped_pages == num_pages);

        /* Perform what coalescing we can. */
        this->MergePages(orig_virt_addr, num_pages, page_list);

        /* We succeeded! We want to persist the reference to the pages. */
        spg.CancelClose();
        R_SUCCEED();
    }

    bool KPageTable::MergePages(TraversalContext *context, PageLinkedList *page_list) {
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

        auto &impl = this->GetImpl();

        /* Iteratively merge, until we can't. */
        bool merged = false;
        while (true) {
            /* Try to merge. */
            KVirtualAddress freed_table = Null<KVirtualAddress>;
            if (!impl.MergePages(std::addressof(freed_table), context)) {
                break;
            }

            /* Note that we updated. */
            this->NoteUpdated();

            /* Free the page. */
            if (freed_table != Null<KVirtualAddress>) {
                ClearPageTable(freed_table);
                this->FreePageTable(page_list, freed_table);
            }

            /* We performed at least one merge. */
            merged = true;
        }

        return merged;
    }

    void KPageTable::MergePages(KProcessAddress virt_addr, size_t num_pages, PageLinkedList *page_list) {
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

        auto &impl = this->GetImpl();

        /* Begin traversal. */
        TraversalContext context;
        TraversalEntry   entry;
        MESOSPHERE_ABORT_UNLESS(impl.BeginTraversal(std::addressof(entry), std::addressof(context), virt_addr));

        /* Merge start of the range. */
        this->MergePages(std::addressof(context), page_list);

        /* If we have more than one page, do the same for the end of the range. */
        if (num_pages > 1) {
            /* Begin traversal for end of range. */
            const size_t size    = num_pages * PageSize;
            const auto end_page  = virt_addr + size;
            const auto last_page = end_page - PageSize;
            MESOSPHERE_ABORT_UNLESS(impl.BeginTraversal(std::addressof(entry), std::addressof(context), last_page));

            /* Merge. */
            this->MergePages(std::addressof(context), page_list);
        }
    }

    Result KPageTable::SeparatePagesImpl(TraversalEntry *entry, TraversalContext *context, KProcessAddress virt_addr, size_t block_size, PageLinkedList *page_list, bool reuse_ll) {
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

        auto &impl = this->GetImpl();

        /* If at any point we fail, we want to merge. */
        ON_RESULT_FAILURE { this->MergePages(context, page_list); };

        /* Iterate, separating until our block size is small enough. */
        while (entry->block_size > block_size) {
            /* If necessary, allocate a table. */
            KVirtualAddress table = Null<KVirtualAddress>;
            if (!context->is_contiguous) {
                table = this->AllocatePageTable(page_list, reuse_ll);
                R_UNLESS(table != Null<KVirtualAddress>, svc::ResultOutOfResource());
            }

            /* Separate. */
            impl.SeparatePages(entry, context, virt_addr, nullptr);
            this->NoteUpdated();
        }

        R_SUCCEED();
    }

    Result KPageTable::SeparatePages(KProcessAddress virt_addr, size_t num_pages, PageLinkedList *page_list, bool reuse_ll) {
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

        auto &impl = this->GetImpl();

        /* Begin traversal. */
        TraversalContext start_context;
        TraversalEntry   entry;
        MESOSPHERE_ABORT_UNLESS(impl.BeginTraversal(std::addressof(entry), std::addressof(start_context), virt_addr));

        /* Separate pages at the start of the range. */
        const size_t size = num_pages * PageSize;
        R_TRY(this->SeparatePagesImpl(std::addressof(entry), std::addressof(start_context), virt_addr, std::min(util::GetAlignment(GetInteger(virt_addr)), size), page_list, reuse_ll));

        /* If necessary, separate pages at the end of the range. */
        if (num_pages > 1) {
            const auto end_page  = virt_addr + size;
            const auto last_page = end_page - PageSize;

            /* Begin traversal. */
            TraversalContext end_context;
            MESOSPHERE_ABORT_UNLESS(impl.BeginTraversal(std::addressof(entry), std::addressof(end_context), last_page));


            ON_RESULT_FAILURE { this->MergePages(std::addressof(start_context), page_list); };

            R_TRY(this->SeparatePagesImpl(std::addressof(entry), std::addressof(end_context), last_page, std::min(util::GetAlignment(GetInteger(end_page)), size), page_list, reuse_ll));
        }

        R_SUCCEED();
    }

    Result KPageTable::ChangePermissions(KProcessAddress virt_addr, size_t num_pages, PageTableEntry entry_template, DisableMergeAttribute disable_merge_attr, bool refresh_mapping, bool flush_mapping, PageLinkedList *page_list, bool reuse_ll) {
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

        /* Separate pages before we change permissions. */
        R_TRY(this->SeparatePages(virt_addr, num_pages, page_list, reuse_ll));

        /* ===================================================== */

        /* Define a helper function which will apply our template to entries. */

        enum ApplyOption : u32 {
            ApplyOption_None           = 0,
            ApplyOption_FlushDataCache = (1u << 0),
            ApplyOption_MergeMappings  = (1u << 1),
        };

        auto ApplyEntryTemplate = [this, virt_addr, disable_merge_attr, num_pages, page_list](PageTableEntry entry_template, u32 apply_option) -> void {
            /* Create work variables for us to use. */
            const KProcessAddress orig_virt_addr = virt_addr;
            const KProcessAddress end_virt_addr  = orig_virt_addr + (num_pages * PageSize);
            KProcessAddress cur_virt_addr = virt_addr;
            size_t remaining_pages = num_pages;

            auto &impl = this->GetImpl();

            /* Parse the disable merge attrs. */
            const bool attr_disable_head           = (disable_merge_attr & DisableMergeAttribute_DisableHead) != 0;
            const bool attr_disable_head_body      = (disable_merge_attr & DisableMergeAttribute_DisableHeadAndBody) != 0;
            const bool attr_enable_head_body       = (disable_merge_attr & DisableMergeAttribute_EnableHeadAndBody) != 0;
            const bool attr_disable_tail           = (disable_merge_attr & DisableMergeAttribute_DisableTail) != 0;
            const bool attr_enable_tail            = (disable_merge_attr & DisableMergeAttribute_EnableTail) != 0;
            const bool attr_enable_and_merge       = (disable_merge_attr & DisableMergeAttribute_EnableAndMergeHeadBodyTail) != 0;

            /* Begin traversal. */
            TraversalContext context;
            TraversalEntry   next_entry;
            MESOSPHERE_ABORT_UNLESS(impl.BeginTraversal(std::addressof(next_entry), std::addressof(context), cur_virt_addr));

            /* Continue changing properties until we've changed them for all pages. */
            bool cleared_disable_merge_bits = false;
            while (remaining_pages > 0) {
                MESOSPHERE_ABORT_UNLESS(util::IsAligned(GetInteger(next_entry.phys_addr), next_entry.block_size));
                MESOSPHERE_ABORT_UNLESS(next_entry.block_size <= remaining_pages * PageSize);

                /* Determine if we're at the start. */
                const bool is_start = (cur_virt_addr == orig_virt_addr);
                const bool is_end   = ((cur_virt_addr + next_entry.block_size) == end_virt_addr);

                /* Determine the relevant merge attributes. */
                bool disable_head_merge, disable_head_body_merge, disable_tail_merge;
                if (next_entry.IsHeadMergeDisabled()) {
                    disable_head_merge = true;
                } else if (attr_disable_head) {
                    disable_head_merge = is_start;
                } else {
                    disable_head_merge = false;
                }
                if (is_start) {
                    if (attr_disable_head_body) {
                        disable_head_body_merge = true;
                    } else if (attr_enable_head_body) {
                        disable_head_body_merge = false;
                    } else {
                        disable_head_body_merge = (!attr_enable_and_merge && next_entry.IsHeadAndBodyMergeDisabled());
                    }
                } else {
                    disable_head_body_merge = (!attr_enable_and_merge && next_entry.IsHeadAndBodyMergeDisabled());
                    cleared_disable_merge_bits |= (attr_enable_and_merge && next_entry.IsHeadAndBodyMergeDisabled());
                }
                if (is_end) {
                    if (attr_disable_tail) {
                        disable_tail_merge = true;
                    } else if (attr_enable_tail) {
                        disable_tail_merge = false;
                    } else {
                        disable_tail_merge = (!attr_enable_and_merge && next_entry.IsTailMergeDisabled());
                    }
                } else {
                    disable_tail_merge = (!attr_enable_and_merge && next_entry.IsTailMergeDisabled());
                    cleared_disable_merge_bits |= (attr_enable_and_merge && next_entry.IsTailMergeDisabled());
                }

                /* Encode the merge disable flags into the software reserved bits. */
                u8 sw_reserved_bits = PageTableEntry::EncodeSoftwareReservedBits(disable_head_merge, disable_head_body_merge, disable_tail_merge);

                /* If we should flush entries, do so. */
                if ((apply_option & ApplyOption_FlushDataCache) != 0) {
                    if (IsHeapPhysicalAddress(next_entry.phys_addr)) {
                        cpu::FlushDataCache(GetVoidPointer(GetHeapVirtualAddress(next_entry.phys_addr)), next_entry.block_size);
                    }
                }

                /* Apply the entry template. */
                L1PageTableEntry *l1_entry = impl.GetL1Entry(cur_virt_addr);
                switch (next_entry.block_size) {
                    case L1BlockSize:
                        {
                            /* Write the updated entry. */
                            *l1_entry = L1PageTableEntry(PageTableEntry::BlockTag{}, next_entry.phys_addr, entry_template, sw_reserved_bits, false);
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
                            const KVirtualAddress l2_virt = GetPageTableVirtualAddress(l2_phys);

                            /* Write the updated entry. */
                            const bool contig = next_entry.block_size == L2ContiguousBlockSize;
                            for (size_t i = 0; i < num_l2_blocks; i++) {
                                *impl.GetL2EntryFromTable(l2_virt, cur_virt_addr + L2BlockSize * i) = L2PageTableEntry(PageTableEntry::BlockTag{}, next_entry.phys_addr + L2BlockSize * i, entry_template, sw_reserved_bits, contig);
                                sw_reserved_bits &= ~(PageTableEntry::SoftwareReservedBit_DisableMergeHead);
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
                            const KVirtualAddress l2_virt = GetPageTableVirtualAddress(l2_phys);
                            L2PageTableEntry *l2_entry = impl.GetL2EntryFromTable(l2_virt, cur_virt_addr);

                            /* Get the L3 entry. */
                            KPhysicalAddress l3_phys = Null<KPhysicalAddress>;
                            MESOSPHERE_ABORT_UNLESS(l2_entry->GetTable(l3_phys));
                            const KVirtualAddress l3_virt = GetPageTableVirtualAddress(l3_phys);

                            /* Write the updated entry. */
                            const bool contig = next_entry.block_size == L3ContiguousBlockSize;
                            for (size_t i = 0; i < num_l3_blocks; i++) {
                                *impl.GetL3EntryFromTable(l3_virt, cur_virt_addr + L3BlockSize * i) = L3PageTableEntry(PageTableEntry::BlockTag{}, next_entry.phys_addr + L3BlockSize * i, entry_template, sw_reserved_bits, contig);
                                sw_reserved_bits &= ~(PageTableEntry::SoftwareReservedBit_DisableMergeHead);
                            }
                        }
                        break;
                    MESOSPHERE_UNREACHABLE_DEFAULT_CASE();
                }

                /* If our option asks us to, try to merge mappings. */
                bool merge = ((apply_option & ApplyOption_MergeMappings) != 0 || cleared_disable_merge_bits) && next_entry.block_size < L1BlockSize;
                if (merge) {
                    const size_t larger_align = GetLargerAlignment(next_entry.block_size);
                    if (util::IsAligned(GetInteger(cur_virt_addr) + next_entry.block_size, larger_align)) {
                        const uintptr_t aligned_start = util::AlignDown(GetInteger(cur_virt_addr), larger_align);
                        if (orig_virt_addr <= aligned_start && aligned_start + larger_align - 1 < GetInteger(orig_virt_addr) + (num_pages * PageSize) - 1) {
                            merge = this->MergePages(std::addressof(context), page_list);
                        } else {
                            merge = false;
                        }
                    } else {
                        merge = false;
                    }
                }

                /* If we merged, correct the traversal to a sane state. */
                if (merge) {
                    /* NOTE: Nintendo does not verify the result of this BeginTraversal call. */
                    MESOSPHERE_ABORT_UNLESS(impl.BeginTraversal(std::addressof(next_entry), std::addressof(context), cur_virt_addr));

                    /* The actual size needs to not take into account the portion of the block before our virtual address. */
                    const size_t actual_size = next_entry.block_size - (GetInteger(next_entry.phys_addr) & (next_entry.block_size - 1));
                    remaining_pages -= std::min(remaining_pages, actual_size / PageSize);
                    cur_virt_addr   += actual_size;
                } else {
                    /* If we didn't merge, just advance. */
                    remaining_pages -= next_entry.block_size / PageSize;
                    cur_virt_addr   += next_entry.block_size;
                }

                /* Continue our traversal. */
                if (remaining_pages == 0) {
                    break;
                }
                MESOSPHERE_ABORT_UNLESS(impl.ContinueTraversal(std::addressof(next_entry), std::addressof(context)));
            }
        };


        /* ===================================================== */

        /* If we don't need to refresh the pages, we can just apply the mappings. */
        if (!refresh_mapping) {
            ApplyEntryTemplate(entry_template, ApplyOption_None);
            this->NoteUpdated();
        } else {
            /* We need to refresh the mappings. */
            /* First, apply the changes without the mapped bit. This will cause all entries to page fault if accessed. */
            {
                PageTableEntry unmapped_template = entry_template;
                unmapped_template.SetMapped(false);
                ApplyEntryTemplate(unmapped_template, ApplyOption_MergeMappings);
                this->NoteUpdated();
            }

            /* Next, take and immediately release the scheduler lock. This will force a reschedule. */
            {
                KScopedSchedulerLock sl;
            }

            /* Finally, apply the changes as directed, flushing the mappings before they're applied (if we should). */
            ApplyEntryTemplate(entry_template, flush_mapping ? ApplyOption_FlushDataCache : ApplyOption_None);
        }

        /* We've succeeded, now perform what coalescing we can. */
        this->MergePages(virt_addr, num_pages, page_list);

        R_SUCCEED();
    }

    void KPageTable::FinalizeUpdateImpl(PageLinkedList *page_list) {
        while (page_list->Peek()) {
            KVirtualAddress page = KVirtualAddress(page_list->Pop());
            MESOSPHERE_ASSERT(this->GetPageTableManager().IsInPageTableHeap(page));
            MESOSPHERE_ASSERT(this->GetPageTableManager().GetRefCount(page) == 0);
            this->GetPageTableManager().Free(page);
        }
    }

}
