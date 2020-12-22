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

        class KPageTableAsidManager {
            private:
                using WordType = u32;
                static constexpr u8 ReservedAsids[] = { 0 };
                static constexpr size_t NumReservedAsids = util::size(ReservedAsids);
                static constexpr size_t BitsPerWord = BITSIZEOF(WordType);
                static constexpr size_t AsidCount = 0x100;
                static constexpr size_t NumWords = AsidCount / BitsPerWord;
                static constexpr WordType FullWord = ~WordType(0u);
            private:
                WordType m_state[NumWords];
                KLightLock m_lock;
                u8 m_hint;
            private:
                constexpr bool TestImpl(u8 asid) const {
                    return m_state[asid / BitsPerWord] & (1u << (asid % BitsPerWord));
                }
                constexpr void ReserveImpl(u8 asid) {
                    MESOSPHERE_ASSERT(!this->TestImpl(asid));
                    m_state[asid / BitsPerWord] |= (1u << (asid % BitsPerWord));
                }

                constexpr void ReleaseImpl(u8 asid) {
                    MESOSPHERE_ASSERT(this->TestImpl(asid));
                    m_state[asid / BitsPerWord] &= ~(1u << (asid % BitsPerWord));
                }

                constexpr u8 FindAvailable() const {
                    for (size_t i = 0; i < util::size(m_state); i++) {
                        if (m_state[i] == FullWord) {
                            continue;
                        }
                        const WordType clear_bit = (m_state[i] + 1) ^ (m_state[i]);
                        return BitsPerWord * i + BitsPerWord - 1 - ClearLeadingZero(clear_bit);
                    }
                    if (m_state[util::size(m_state)-1] == FullWord) {
                        MESOSPHERE_PANIC("Unable to reserve ASID");
                    }
                    __builtin_unreachable();
                }

                static constexpr ALWAYS_INLINE WordType ClearLeadingZero(WordType value) {
                    return __builtin_clzll(value) - (BITSIZEOF(unsigned long long) - BITSIZEOF(WordType));
                }
            public:
                constexpr KPageTableAsidManager() : m_state(), m_lock(), m_hint() {
                    for (size_t i = 0; i < NumReservedAsids; i++) {
                        this->ReserveImpl(ReservedAsids[i]);
                    }
                }

                u8 Reserve() {
                    KScopedLightLock lk(m_lock);

                    if (this->TestImpl(m_hint)) {
                        m_hint = this->FindAvailable();
                    }

                    this->ReserveImpl(m_hint);

                    return m_hint++;
                }

                void Release(u8 asid) {
                    KScopedLightLock lk(m_lock);
                    this->ReleaseImpl(asid);
                }
        };

        KPageTableAsidManager g_asid_manager;

    }

    void KPageTable::Initialize(s32 core_id) {
        /* Nothing actually needed here. */
        MESOSPHERE_UNUSED(core_id);
    }

    Result KPageTable::InitializeForKernel(void *table, KVirtualAddress start, KVirtualAddress end) {
        /* Initialize basic fields. */
        m_asid = 0;
        m_manager = std::addressof(Kernel::GetPageTableManager());

        /* Allocate a page for ttbr. */
        const u64 asid_tag = (static_cast<u64>(m_asid) << 48ul);
        const KVirtualAddress page = m_manager->Allocate();
        MESOSPHERE_ASSERT(page != Null<KVirtualAddress>);
        cpu::ClearPageToZero(GetVoidPointer(page));
        m_ttbr = GetInteger(KPageTableBase::GetLinearMappedPhysicalAddress(page)) | asid_tag;

        /* Initialize the base page table. */
        MESOSPHERE_R_ABORT_UNLESS(KPageTableBase::InitializeForKernel(true, table, start, end));

        return ResultSuccess();
    }

    Result KPageTable::InitializeForProcess(u32 id, ams::svc::CreateProcessFlag as_type, bool enable_aslr, bool enable_das_merge, bool from_back, KMemoryManager::Pool pool, KProcessAddress code_address, size_t code_size, KMemoryBlockSlabManager *mem_block_slab_manager, KBlockInfoManager *block_info_manager, KPageTableManager *pt_manager) {
        /* The input ID isn't actually used. */
        MESOSPHERE_UNUSED(id);

        /* Get an ASID */
        m_asid = g_asid_manager.Reserve();
        auto asid_guard = SCOPE_GUARD { g_asid_manager.Release(m_asid); };

        /* Set our manager. */
        m_manager = pt_manager;

        /* Allocate a new table, and set our ttbr value. */
        const KVirtualAddress new_table = m_manager->Allocate();
        R_UNLESS(new_table != Null<KVirtualAddress>, svc::ResultOutOfResource());
        m_ttbr = EncodeTtbr(GetPageTablePhysicalAddress(new_table), m_asid);
        auto table_guard = SCOPE_GUARD { m_manager->Free(new_table); };

        /* Initialize our base table. */
        const size_t as_width = GetAddressSpaceWidth(as_type);
        const KProcessAddress as_start = 0;
        const KProcessAddress as_end   = (1ul << as_width);
        R_TRY(KPageTableBase::InitializeForProcess(as_type, enable_aslr, enable_das_merge, from_back, pool, GetVoidPointer(new_table), as_start, as_end, code_address, code_size, mem_block_slab_manager, block_info_manager));

        /* We succeeded! */
        table_guard.Cancel();
        asid_guard.Cancel();

        /* Note that we've updated the table (since we created it). */
        this->NoteUpdated();
        return ResultSuccess();
    }

    Result KPageTable::Finalize() {
        /* Only process tables should be finalized. */
        MESOSPHERE_ASSERT(!this->IsKernel());

        /* Note that we've updated (to ensure we're synchronized). */
        this->NoteUpdated();

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
                TraversalEntry   cur_entry  = {};
                bool             cur_valid  = false;
                TraversalEntry   next_entry;
                bool             next_valid;
                size_t           tot_size   = 0;

                next_valid = impl.BeginTraversal(std::addressof(next_entry), std::addressof(context), this->GetAddressSpaceStart());

                /* Iterate over entries. */
                while (true) {
                    if ((!next_valid && !cur_valid) || (next_valid && cur_valid && next_entry.phys_addr == cur_entry.phys_addr + cur_entry.block_size)) {
                        cur_entry.block_size += next_entry.block_size;
                    } else {
                        if (cur_valid && IsHeapPhysicalAddressForFinalize(cur_entry.phys_addr)) {
                            mm.Close(GetHeapVirtualAddress(cur_entry.phys_addr), cur_entry.block_size / PageSize);
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
                    mm.Close(GetHeapVirtualAddress(cur_entry.phys_addr), cur_entry.block_size / PageSize);
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
                        KVirtualAddress l3_table = GetPageTableVirtualAddress(l2_entry->GetTable());
                        if (this->GetPageTableManager().IsInPageTableHeap(l3_table)) {
                            while (!this->GetPageTableManager().Close(l3_table, 1)) { /* ... */ }
                            this->GetPageTableManager().Free(l3_table);
                        }
                    }
                }
            }

            /* Free all L2 tables. */
            for (KProcessAddress cur_address = as_start; cur_address <= as_last; cur_address += L1BlockSize) {
                L1PageTableEntry *l1_entry = impl.GetL1Entry(cur_address);
                if (l1_entry->IsTable()) {
                    KVirtualAddress l2_table = GetPageTableVirtualAddress(l1_entry->GetTable());
                    if (this->GetPageTableManager().IsInPageTableHeap(l2_table)) {
                        while (!this->GetPageTableManager().Close(l2_table, 1)) { /* ... */ }
                        this->GetPageTableManager().Free(l2_table);
                    }
                }
            }

            /* Free the L1 table. */
            this->GetPageTableManager().Free(reinterpret_cast<uintptr_t>(impl.Finalize()));

            /* Perform inherited finalization. */
            KPageTableBase::Finalize();
        }

        /* Release our asid. */
        g_asid_manager.Release(m_asid);

        return ResultSuccess();
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
                    return this->MapContiguous(virt_addr, phys_addr, num_pages, entry_template, properties.disable_merge_attributes == DisableMergeAttribute_DisableHead, page_list, reuse_ll);
                case OperationType_ChangePermissions:
                    return this->ChangePermissions(virt_addr, num_pages, entry_template, properties.disable_merge_attributes, false, page_list, reuse_ll);
                case OperationType_ChangePermissionsAndRefresh:
                    return this->ChangePermissions(virt_addr, num_pages, entry_template, properties.disable_merge_attributes, true, page_list, reuse_ll);
                MESOSPHERE_UNREACHABLE_DEFAULT_CASE();
            }
        }
    }

    Result KPageTable::Operate(PageLinkedList *page_list, KProcessAddress virt_addr, size_t num_pages, const KPageGroup &page_group, const KPageProperties properties, OperationType operation, bool reuse_ll) {
        /* Check validity of parameters. */
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());
        MESOSPHERE_ASSERT(util::IsAligned(GetInteger(virt_addr), PageSize));
        MESOSPHERE_ASSERT(num_pages > 0);
        MESOSPHERE_ASSERT(num_pages == page_group.GetNumPages());

        /* Map the page group. */
        auto entry_template = this->GetEntryTemplate(properties);
        switch (operation) {
            case OperationType_MapGroup:
                return this->MapGroup(virt_addr, page_group, num_pages, entry_template, properties.disable_merge_attributes == DisableMergeAttribute_DisableHead, page_list, reuse_ll);
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

        return ResultSuccess();
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
                    PteDataSynchronizationBarrier();
                    *l1_entry = L1PageTableEntry(PageTableEntry::TableTag{}, l2_phys, this->IsKernel(), true);
                    PteDataSynchronizationBarrier();
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

        return ResultSuccess();
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
                        PteDataSynchronizationBarrier();
                        *l1_entry = L1PageTableEntry(PageTableEntry::TableTag{}, l2_phys, this->IsKernel(), true);
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
                        *l2_entry = L2PageTableEntry(PageTableEntry::TableTag{}, l3_phys, this->IsKernel(), true);
                        PteDataSynchronizationBarrier();
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
        ON_SCOPE_EXIT { pages_to_close.Close(); };

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
                MESOSPHERE_UNREACHABLE_DEFAULT_CASE();
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

    Result KPageTable::MapContiguous(KProcessAddress virt_addr, KPhysicalAddress phys_addr, size_t num_pages, PageTableEntry entry_template, bool disable_head_merge, PageLinkedList *page_list, bool reuse_ll) {
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

        /* Cache initial addresses for use on cleanup. */
        const KProcessAddress  orig_virt_addr = virt_addr;
        const KPhysicalAddress orig_phys_addr = phys_addr;

        size_t remaining_pages = num_pages;

        /* Map the pages, using a guard to ensure we don't leak. */
        {
            auto map_guard = SCOPE_GUARD { MESOSPHERE_R_ABORT_UNLESS(this->Unmap(orig_virt_addr, num_pages, page_list, true, true)); };

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

            /* We successfully mapped, so cancel our guard. */
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

    Result KPageTable::MapGroup(KProcessAddress virt_addr, const KPageGroup &pg, size_t num_pages, PageTableEntry entry_template, bool disable_head_merge, PageLinkedList *page_list, bool reuse_ll) {
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

        /* We want to maintain a new reference to every page in the group. */
        KScopedPageGroup spg(pg);

        /* Cache initial address for use on cleanup. */
        const KProcessAddress  orig_virt_addr = virt_addr;

        size_t mapped_pages = 0;

        /* Map the pages, using a guard to ensure we don't leak. */
        {
            auto map_guard = SCOPE_GUARD { MESOSPHERE_R_ABORT_UNLESS(this->Unmap(orig_virt_addr, num_pages, page_list, true, true)); };

            if (num_pages < ContiguousPageSize / PageSize) {
                for (const auto &block : pg) {
                    const KPhysicalAddress block_phys_addr = GetLinearMappedPhysicalAddress(block.GetAddress());
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
                    const KPhysicalAddress block_phys_addr = GetLinearMappedPhysicalAddress(block.GetAddress());
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

            /* We successfully mapped, so cancel our guard. */
            map_guard.Cancel();
        }
        MESOSPHERE_ASSERT(mapped_pages == num_pages);

        /* Perform what coalescing we can. */
        this->MergePages(orig_virt_addr, page_list);
        if (num_pages > 1) {
            this->MergePages(orig_virt_addr + (num_pages - 1) * PageSize, page_list);
        }

        /* We succeeded! We want to persist the reference to the pages. */
        spg.CancelClose();
        return ResultSuccess();
    }

    bool KPageTable::MergePages(KProcessAddress virt_addr, PageLinkedList *page_list) {
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

        auto &impl = this->GetImpl();
        bool merged = false;

        /* If there's no L1 table, don't bother. */
        L1PageTableEntry *l1_entry = impl.GetL1Entry(virt_addr);
        if (!l1_entry->IsTable()) {
            /* Ensure the table is not corrupted. */
            MESOSPHERE_ABORT_UNLESS(l1_entry->IsBlock() || l1_entry->IsEmpty());
            return merged;
        }

        /* Examine and try to merge the L2 table. */
        L2PageTableEntry *l2_entry = impl.GetL2Entry(l1_entry, virt_addr);
        if (l2_entry->IsTable()) {
            /* We have an L3 entry. */
            L3PageTableEntry *l3_entry = impl.GetL3Entry(l2_entry, virt_addr);
            if (!l3_entry->IsBlock()) {
                return merged;
            }

            /* If it's not contiguous, try to make it so. */
            if (!l3_entry->IsContiguous()) {
                virt_addr = util::AlignDown(GetInteger(virt_addr), L3ContiguousBlockSize);
                const KPhysicalAddress phys_addr = util::AlignDown(GetInteger(l3_entry->GetBlock()), L3ContiguousBlockSize);
                const u64 entry_template = l3_entry->GetEntryTemplateForMerge();

                /* Validate that we can merge. */
                for (size_t i = 0; i < L3ContiguousBlockSize / L3BlockSize; i++) {
                    const L3PageTableEntry *check_entry = impl.GetL3Entry(l2_entry, virt_addr + L3BlockSize * i);
                    if (!check_entry->IsForMerge(entry_template | GetInteger(phys_addr + L3BlockSize * i) | PageTableEntry::Type_L3Block)) {
                        return merged;
                    }
                    if (i > 0 && (check_entry->IsHeadOrHeadAndBodyMergeDisabled())) {
                        return merged;
                    }
                    if ((i < (L3ContiguousBlockSize / L3BlockSize) - 1) && check_entry->IsTailMergeDisabled()) {
                        return merged;
                    }
                }

                /* Determine the new software reserved bits. */
                const L3PageTableEntry *head_entry = impl.GetL3Entry(l2_entry, virt_addr + L3BlockSize * 0);
                const L3PageTableEntry *tail_entry = impl.GetL3Entry(l2_entry, virt_addr + L3BlockSize * ((L3ContiguousBlockSize / L3BlockSize) - 1));
                auto sw_reserved_bits = PageTableEntry::EncodeSoftwareReservedBits(head_entry->IsHeadMergeDisabled(), head_entry->IsHeadAndBodyMergeDisabled(), tail_entry->IsTailMergeDisabled());

                /* Merge! */
                for (size_t i = 0; i < L3ContiguousBlockSize / L3BlockSize; i++) {
                    *impl.GetL3Entry(l2_entry, virt_addr + L3BlockSize * i) = L3PageTableEntry(PageTableEntry::BlockTag{}, phys_addr + L3BlockSize * i, PageTableEntry(entry_template), sw_reserved_bits, true);
                    sw_reserved_bits &= ~(PageTableEntry::SoftwareReservedBit_DisableMergeHead);
                }

                /* Note that we updated. */
                this->NoteUpdated();
                merged = true;
            }

            /* We might be able to upgrade a contiguous set of L3 entries into an L2 block. */
            virt_addr = util::AlignDown(GetInteger(virt_addr), L2BlockSize);
            KPhysicalAddress phys_addr = util::AlignDown(GetInteger(l3_entry->GetBlock()), L2BlockSize);
            const u64 entry_template = l3_entry->GetEntryTemplateForMerge();

            /* Validate that we can merge. */
            for (size_t i = 0; i < L2BlockSize / L3ContiguousBlockSize; i++) {
                const L3PageTableEntry *check_entry = impl.GetL3Entry(l2_entry, virt_addr + L3ContiguousBlockSize * i);
                if (!check_entry->IsForMerge(entry_template | GetInteger(phys_addr + L3ContiguousBlockSize * i) | PageTableEntry::ContigType_Contiguous | PageTableEntry::Type_L3Block)) {
                    return merged;
                }
                if (i > 0 && (check_entry->IsHeadOrHeadAndBodyMergeDisabled())) {
                    return merged;
                }
                if ((i < (L2BlockSize / L3ContiguousBlockSize) - 1) && check_entry->IsTailMergeDisabled()) {
                    return merged;
                }
            }

            /* Determine the new software reserved bits. */
            const L3PageTableEntry *head_entry = impl.GetL3Entry(l2_entry, virt_addr + L3ContiguousBlockSize * 0);
            const L3PageTableEntry *tail_entry = impl.GetL3Entry(l2_entry, virt_addr + L3ContiguousBlockSize * ((L2BlockSize / L3ContiguousBlockSize) - 1));
            auto sw_reserved_bits = PageTableEntry::EncodeSoftwareReservedBits(head_entry->IsHeadMergeDisabled(), head_entry->IsHeadAndBodyMergeDisabled(), tail_entry->IsTailMergeDisabled());

            /* Merge! */
            PteDataSynchronizationBarrier();
            *l2_entry = L2PageTableEntry(PageTableEntry::BlockTag{}, phys_addr, PageTableEntry(entry_template), sw_reserved_bits, false);

            /* Note that we updated. */
            this->NoteUpdated();
            merged = true;

            /* Free the L3 table. */
            KVirtualAddress l3_table = util::AlignDown(reinterpret_cast<uintptr_t>(l3_entry), PageSize);
            if (this->GetPageTableManager().IsInPageTableHeap(l3_table)) {
                this->GetPageTableManager().Close(l3_table, L2BlockSize / L3BlockSize);
                ClearPageTable(l3_table);
                this->FreePageTable(page_list, l3_table);
            }
        }

        /* If the l2 entry is not a block or we can't make it contiguous, we're done. */
        if (!l2_entry->IsBlock()) {
            return merged;
        }

        /* If it's not contiguous, try to make it so. */
        if (!l2_entry->IsContiguous()) {
            virt_addr = util::AlignDown(GetInteger(virt_addr), L2ContiguousBlockSize);
            KPhysicalAddress phys_addr = util::AlignDown(GetInteger(l2_entry->GetBlock()), L2ContiguousBlockSize);
            const u64 entry_template = l2_entry->GetEntryTemplateForMerge();

            /* Validate that we can merge. */
            for (size_t i = 0; i < L2ContiguousBlockSize / L2BlockSize; i++) {
                const L2PageTableEntry *check_entry = impl.GetL2Entry(l1_entry, virt_addr + L2BlockSize * i);
                if (!check_entry->IsForMerge(entry_template | GetInteger(phys_addr + L2BlockSize * i) | PageTableEntry::Type_L2Block)) {
                    return merged;
                }
                if (i > 0 && (check_entry->IsHeadOrHeadAndBodyMergeDisabled())) {
                    return merged;
                }
                if ((i < (L2ContiguousBlockSize / L2BlockSize) - 1) && check_entry->IsTailMergeDisabled()) {
                    return merged;
                }
            }

            /* Determine the new software reserved bits. */
            const L2PageTableEntry *head_entry = impl.GetL2Entry(l1_entry, virt_addr + L2BlockSize * 0);
            const L2PageTableEntry *tail_entry = impl.GetL2Entry(l1_entry, virt_addr + L2BlockSize * ((L2ContiguousBlockSize / L2BlockSize) - 1));
            auto sw_reserved_bits = PageTableEntry::EncodeSoftwareReservedBits(head_entry->IsHeadMergeDisabled(), head_entry->IsHeadAndBodyMergeDisabled(), tail_entry->IsTailMergeDisabled());

            /* Merge! */
            for (size_t i = 0; i < L2ContiguousBlockSize / L2BlockSize; i++) {
                *impl.GetL2Entry(l1_entry, virt_addr + L2BlockSize * i) = L2PageTableEntry(PageTableEntry::BlockTag{}, phys_addr + L2BlockSize * i, PageTableEntry(entry_template), sw_reserved_bits, true);
                sw_reserved_bits &= ~(PageTableEntry::SoftwareReservedBit_DisableMergeHead);
            }

            /* Note that we updated. */
            this->NoteUpdated();
            merged = true;
        }

        /* We might be able to upgrade a contiguous set of L2 entries into an L1 block. */
        virt_addr = util::AlignDown(GetInteger(virt_addr), L1BlockSize);
        KPhysicalAddress phys_addr = util::AlignDown(GetInteger(l2_entry->GetBlock()), L1BlockSize);
        const u64 entry_template = l2_entry->GetEntryTemplateForMerge();

        /* Validate that we can merge. */
        for (size_t i = 0; i < L1BlockSize / L2ContiguousBlockSize; i++) {
            const L2PageTableEntry *check_entry = impl.GetL2Entry(l1_entry, virt_addr + L2ContiguousBlockSize * i);
            if (!check_entry->IsForMerge(entry_template | GetInteger(phys_addr + L2ContiguousBlockSize * i) | PageTableEntry::ContigType_Contiguous | PageTableEntry::Type_L2Block)) {
                return merged;
            }
            if (i > 0 && (check_entry->IsHeadOrHeadAndBodyMergeDisabled())) {
                return merged;
            }
            if ((i < (L1ContiguousBlockSize / L2ContiguousBlockSize) - 1) && check_entry->IsTailMergeDisabled()) {
                return merged;
            }
        }

        /* Determine the new software reserved bits. */
        const L2PageTableEntry *head_entry = impl.GetL2Entry(l1_entry, virt_addr + L2ContiguousBlockSize * 0);
        const L2PageTableEntry *tail_entry = impl.GetL2Entry(l1_entry, virt_addr + L2ContiguousBlockSize * ((L1BlockSize / L2ContiguousBlockSize) - 1));
        auto sw_reserved_bits = PageTableEntry::EncodeSoftwareReservedBits(head_entry->IsHeadMergeDisabled(), head_entry->IsHeadAndBodyMergeDisabled(), tail_entry->IsTailMergeDisabled());

        /* Merge! */
        PteDataSynchronizationBarrier();
        *l1_entry = L1PageTableEntry(PageTableEntry::BlockTag{}, phys_addr, PageTableEntry(entry_template), sw_reserved_bits, false);

        /* Note that we updated. */
        this->NoteUpdated();
        merged = true;

        /* Free the L2 table. */
        KVirtualAddress l2_table = util::AlignDown(reinterpret_cast<uintptr_t>(l2_entry), PageSize);
        if (this->GetPageTableManager().IsInPageTableHeap(l2_table)) {
            this->GetPageTableManager().Close(l2_table, L1BlockSize / L2BlockSize);
            ClearPageTable(l2_table);
            this->FreePageTable(page_list, l2_table);
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
            R_SUCCEED_IF(block_size >= L1BlockSize);

            /* Get the addresses we're working with. */
            const KProcessAddress block_virt_addr  = util::AlignDown(GetInteger(virt_addr), L1BlockSize);
            const KPhysicalAddress block_phys_addr = l1_entry->GetBlock();

            /* Allocate a new page for the L2 table. */
            const KVirtualAddress l2_table = this->AllocatePageTable(page_list, reuse_ll);
            R_UNLESS(l2_table != Null<KVirtualAddress>, svc::ResultOutOfResource());
            const KPhysicalAddress l2_phys = GetPageTablePhysicalAddress(l2_table);

            /* Set the entries in the L2 table. */
            for (size_t i = 0; i < L1BlockSize / L2BlockSize; i++) {
                const u64 entry_template = l1_entry->GetEntryTemplateForL2Block(i);
                *(impl.GetL2EntryFromTable(l2_table, block_virt_addr + L2BlockSize * i)) = L2PageTableEntry(PageTableEntry::BlockTag{}, block_phys_addr + L2BlockSize * i, PageTableEntry(entry_template), PageTableEntry::SoftwareReservedBit_None, true);
            }

            /* Open references to the L2 table. */
            this->GetPageTableManager().Open(l2_table, L1BlockSize / L2BlockSize);

            /* Replace the L1 entry with one to the new table. */
            PteDataSynchronizationBarrier();
            *l1_entry = L1PageTableEntry(PageTableEntry::TableTag{}, l2_phys, this->IsKernel(), true);
            this->NoteUpdated();
        }

        /* If we don't have an l1 table, we're done. */
        MESOSPHERE_ABORT_UNLESS(l1_entry->IsTable() || l1_entry->IsEmpty());
        R_SUCCEED_IF(!l1_entry->IsTable());

        /* We want to separate L2 contiguous blocks into L2 blocks, so check that our size permits that. */
        R_SUCCEED_IF(block_size >= L2ContiguousBlockSize);

        L2PageTableEntry *l2_entry = impl.GetL2Entry(l1_entry, virt_addr);
        if (l2_entry->IsBlock()) {
            /* If we're contiguous, try to separate. */
            if (l2_entry->IsContiguous()) {
                const KProcessAddress block_virt_addr  = util::AlignDown(GetInteger(virt_addr), L2ContiguousBlockSize);
                const KPhysicalAddress block_phys_addr = util::AlignDown(GetInteger(l2_entry->GetBlock()), L2ContiguousBlockSize);

                /* Mark the entries as non-contiguous. */
                for (size_t i = 0; i < L2ContiguousBlockSize / L2BlockSize; i++) {
                    L2PageTableEntry *target = impl.GetL2Entry(l1_entry, block_virt_addr + L2BlockSize * i);
                    const u64 entry_template = target->GetEntryTemplateForL2Block(i);
                    *target = L2PageTableEntry(PageTableEntry::BlockTag{}, block_phys_addr + L2BlockSize * i, PageTableEntry(entry_template), PageTableEntry::SoftwareReservedBit_None, false);
                }
                this->NoteUpdated();
            }

            /* We want to separate L2 blocks into L3 contiguous blocks, so check that our size permits that. */
            R_SUCCEED_IF(block_size >= L2BlockSize);

            /* Get the addresses we're working with. */
            const KProcessAddress block_virt_addr  = util::AlignDown(GetInteger(virt_addr), L2BlockSize);
            const KPhysicalAddress block_phys_addr = l2_entry->GetBlock();

            /* Allocate a new page for the L3 table. */
            const KVirtualAddress l3_table = this->AllocatePageTable(page_list, reuse_ll);
            R_UNLESS(l3_table != Null<KVirtualAddress>, svc::ResultOutOfResource());
            const KPhysicalAddress l3_phys = GetPageTablePhysicalAddress(l3_table);

            /* Set the entries in the L3 table. */
            for (size_t i = 0; i < L2BlockSize / L3BlockSize; i++) {
                const u64 entry_template = l2_entry->GetEntryTemplateForL3Block(i);
                *(impl.GetL3EntryFromTable(l3_table, block_virt_addr + L3BlockSize * i)) = L3PageTableEntry(PageTableEntry::BlockTag{}, block_phys_addr + L3BlockSize * i, PageTableEntry(entry_template), PageTableEntry::SoftwareReservedBit_None, true);
            }

            /* Open references to the L3 table. */
            this->GetPageTableManager().Open(l3_table, L2BlockSize / L3BlockSize);

            /* Replace the L2 entry with one to the new table. */
            PteDataSynchronizationBarrier();
            *l2_entry = L2PageTableEntry(PageTableEntry::TableTag{}, l3_phys, this->IsKernel(), true);
            this->NoteUpdated();
        }

        /* If we don't have an L3 table, we're done. */
        MESOSPHERE_ABORT_UNLESS(l2_entry->IsTable() || l2_entry->IsEmpty());
        R_SUCCEED_IF(!l2_entry->IsTable());

        /* We want to separate L3 contiguous blocks into L2 blocks, so check that our size permits that. */
        R_SUCCEED_IF(block_size >= L3ContiguousBlockSize);

        /* If we're contiguous, try to separate. */
        L3PageTableEntry *l3_entry = impl.GetL3Entry(l2_entry, virt_addr);
        if (l3_entry->IsBlock() && l3_entry->IsContiguous()) {
            const KProcessAddress block_virt_addr  = util::AlignDown(GetInteger(virt_addr), L3ContiguousBlockSize);
            const KPhysicalAddress block_phys_addr = util::AlignDown(GetInteger(l3_entry->GetBlock()), L3ContiguousBlockSize);

            /* Mark the entries as non-contiguous. */
            for (size_t i = 0; i < L3ContiguousBlockSize / L3BlockSize; i++) {
                L3PageTableEntry *target = impl.GetL3Entry(l2_entry, block_virt_addr + L3BlockSize * i);
                const u64 entry_template = target->GetEntryTemplateForL3Block(i);
                *target = L3PageTableEntry(PageTableEntry::BlockTag{}, block_phys_addr + L3BlockSize * i, PageTableEntry(entry_template), PageTableEntry::SoftwareReservedBit_None, false);
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

    Result KPageTable::ChangePermissions(KProcessAddress virt_addr, size_t num_pages, PageTableEntry entry_template, DisableMergeAttribute disable_merge_attr, bool refresh_mapping, PageLinkedList *page_list, bool reuse_ll) {
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

        /* Separate pages before we change permissions. */
        const size_t size = num_pages * PageSize;
        R_TRY(this->SeparatePages(virt_addr, std::min(GetInteger(virt_addr) & -GetInteger(virt_addr), size), page_list, reuse_ll));
        if (num_pages > 1) {
            const auto end_page  = virt_addr + size;
            const auto last_page = end_page - PageSize;

            auto merge_guard = SCOPE_GUARD { this->MergePages(virt_addr, page_list); };
            R_TRY(this->SeparatePages(last_page, std::min(GetInteger(end_page) & -GetInteger(end_page), size), page_list, reuse_ll));
            merge_guard.Cancel();
        }

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
                            merge = this->MergePages(cur_virt_addr, page_list);
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

            /* Finally, apply the changes as directed, flushing the mappings before they're applied. */
            ApplyEntryTemplate(entry_template, ApplyOption_FlushDataCache);
        }

        /* We've succeeded, now perform what coalescing we can. */
        this->MergePages(virt_addr, page_list);
        if (num_pages > 1) {
            this->MergePages(virt_addr + (num_pages - 1) * PageSize, page_list);
        }

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
