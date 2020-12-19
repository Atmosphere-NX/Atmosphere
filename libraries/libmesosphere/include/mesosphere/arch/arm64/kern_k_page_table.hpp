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
#include <mesosphere/kern_k_page_table_base.hpp>
#include <mesosphere/kern_k_page_group.hpp>
#include <mesosphere/kern_k_page_table_manager.hpp>

namespace ams::kern::arch::arm64 {

    class KPageTable : public KPageTableBase {
        NON_COPYABLE(KPageTable);
        NON_MOVEABLE(KPageTable);
        public:
            using TraversalEntry   = KPageTableImpl::TraversalEntry;
            using TraversalContext = KPageTableImpl::TraversalContext;

            enum BlockType {
                BlockType_L3Block,
                BlockType_L3ContiguousBlock,
                BlockType_L2Block,

#ifdef ATMOSPHERE_BOARD_NINTENDO_NX
                BlockType_L2TegraSmmuBlock,
#endif

                BlockType_L2ContiguousBlock,
                BlockType_L1Block,

                BlockType_Count,
            };
            static_assert(L3BlockSize == PageSize);
            static constexpr size_t ContiguousPageSize = L3ContiguousBlockSize;

#ifdef ATMOSPHERE_BOARD_NINTENDO_NX
            static constexpr size_t L2TegraSmmuBlockSize = 2 * L2BlockSize;
#endif
            static constexpr size_t BlockSizes[BlockType_Count] = {
                [BlockType_L3Block]           = L3BlockSize,
                [BlockType_L3ContiguousBlock] = L3ContiguousBlockSize,
                [BlockType_L2Block]           = L2BlockSize,
#ifdef ATMOSPHERE_BOARD_NINTENDO_NX
                [BlockType_L2TegraSmmuBlock]  = L2TegraSmmuBlockSize,
#endif
                [BlockType_L2ContiguousBlock] = L2ContiguousBlockSize,
                [BlockType_L1Block]           = L1BlockSize,
            };

            static constexpr BlockType GetMaxBlockType() {
                return BlockType_L1Block;
            }

            static constexpr size_t GetBlockSize(BlockType type) {
                return BlockSizes[type];
            }

            static constexpr BlockType GetBlockType(size_t size) {
                switch (size) {
                    case L3BlockSize:           return BlockType_L3Block;
                    case L3ContiguousBlockSize: return BlockType_L3ContiguousBlock;
                    case L2BlockSize:           return BlockType_L2Block;
#ifdef ATMOSPHERE_BOARD_NINTENDO_NX
                    case L2TegraSmmuBlockSize:  return BlockType_L2TegraSmmuBlock;
#endif
                    case L2ContiguousBlockSize: return BlockType_L2ContiguousBlock;
                    case L1BlockSize:           return BlockType_L1Block;
                    MESOSPHERE_UNREACHABLE_DEFAULT_CASE();
                }
            }

            static constexpr size_t GetSmallerAlignment(size_t alignment) {
                MESOSPHERE_ASSERT(alignment > L3BlockSize);
                return KPageTable::GetBlockSize(static_cast<KPageTable::BlockType>(KPageTable::GetBlockType(alignment) - 1));
            }

            static constexpr size_t GetLargerAlignment(size_t alignment) {
                MESOSPHERE_ASSERT(alignment < L1BlockSize);
                return KPageTable::GetBlockSize(static_cast<KPageTable::BlockType>(KPageTable::GetBlockType(alignment) + 1));
            }
        private:
            KPageTableManager *m_manager;
            u64 m_ttbr;
            u8 m_asid;
        protected:
            virtual Result Operate(PageLinkedList *page_list, KProcessAddress virt_addr, size_t num_pages, KPhysicalAddress phys_addr, bool is_pa_valid, const KPageProperties properties, OperationType operation, bool reuse_ll) override;
            virtual Result Operate(PageLinkedList *page_list, KProcessAddress virt_addr, size_t num_pages, const KPageGroup &page_group, const KPageProperties properties, OperationType operation, bool reuse_ll) override;
            virtual void   FinalizeUpdate(PageLinkedList *page_list) override;

            KPageTableManager &GetPageTableManager() const { return *m_manager; }
        private:
            constexpr PageTableEntry GetEntryTemplate(const KPageProperties properties) const {
                /* Set basic attributes. */
                PageTableEntry entry{PageTableEntry::ExtensionFlag_Valid};
                entry.SetPrivilegedExecuteNever(true);
                entry.SetAccessFlag(PageTableEntry::AccessFlag_Accessed);
                entry.SetShareable(PageTableEntry::Shareable_InnerShareable);

                if (!this->IsKernel()) {
                    entry.SetGlobal(false);
                }

                /* Set page attribute. */
                if (properties.io) {
                    MESOSPHERE_ABORT_UNLESS(!properties.uncached);
                    MESOSPHERE_ABORT_UNLESS((properties.perm & (KMemoryPermission_KernelExecute | KMemoryPermission_UserExecute)) == 0);

                    entry.SetPageAttribute(PageTableEntry::PageAttribute_Device_nGnRnE)
                         .SetUserExecuteNever(true);
                } else if (properties.uncached) {
                    MESOSPHERE_ABORT_UNLESS((properties.perm & (KMemoryPermission_KernelExecute | KMemoryPermission_UserExecute)) == 0);

                    entry.SetPageAttribute(PageTableEntry::PageAttribute_NormalMemoryNotCacheable);
                } else {
                    entry.SetPageAttribute(PageTableEntry::PageAttribute_NormalMemory);
                }

                /* Set user execute never bit. */
                if (properties.perm != KMemoryPermission_UserReadExecute) {
                    MESOSPHERE_ABORT_UNLESS((properties.perm & (KMemoryPermission_KernelExecute | KMemoryPermission_UserExecute)) == 0);
                    entry.SetUserExecuteNever(true);
                }

                /* Set AP[1] based on perm. */
                switch (properties.perm & KMemoryPermission_UserReadWrite) {
                    case KMemoryPermission_UserReadWrite:
                    case KMemoryPermission_UserRead:
                        entry.SetUserAccessible(true);
                        break;
                    case KMemoryPermission_KernelReadWrite:
                    case KMemoryPermission_KernelRead:
                        entry.SetUserAccessible(false);
                        break;
                    MESOSPHERE_UNREACHABLE_DEFAULT_CASE();
                }

                /* Set AP[2] based on perm. */
                switch (properties.perm & KMemoryPermission_UserReadWrite) {
                    case KMemoryPermission_UserReadWrite:
                    case KMemoryPermission_KernelReadWrite:
                        entry.SetReadOnly(false);
                        break;
                    case KMemoryPermission_KernelRead:
                    case KMemoryPermission_UserRead:
                        entry.SetReadOnly(true);
                        break;
                    MESOSPHERE_UNREACHABLE_DEFAULT_CASE();
                }

                /* Set the fault bit based on whether the page is mapped. */
                entry.SetMapped((properties.perm & KMemoryPermission_NotMapped) == 0);

                return entry;
            }
        public:
            constexpr KPageTable() : KPageTableBase(), m_manager(), m_ttbr(), m_asid() { /* ... */ }

            static NOINLINE void Initialize(s32 core_id);

            ALWAYS_INLINE void Activate(u32 proc_id) {
                cpu::DataSynchronizationBarrier();
                cpu::SwitchProcess(m_ttbr, proc_id);
            }

            NOINLINE Result InitializeForKernel(void *table, KVirtualAddress start, KVirtualAddress end);
            NOINLINE Result InitializeForProcess(u32 id, ams::svc::CreateProcessFlag as_type, bool enable_aslr, bool enable_das_merge, bool from_back, KMemoryManager::Pool pool, KProcessAddress code_address, size_t code_size, KMemoryBlockSlabManager *mem_block_slab_manager, KBlockInfoManager *block_info_manager, KPageTableManager *pt_manager);
            Result Finalize();
        private:
            Result MapL1Blocks(KProcessAddress virt_addr, KPhysicalAddress phys_addr, size_t num_pages, PageTableEntry entry_template, bool disable_head_merge, PageLinkedList *page_list, bool reuse_ll);
            Result MapL2Blocks(KProcessAddress virt_addr, KPhysicalAddress phys_addr, size_t num_pages, PageTableEntry entry_template, bool disable_head_merge, PageLinkedList *page_list, bool reuse_ll);
            Result MapL3Blocks(KProcessAddress virt_addr, KPhysicalAddress phys_addr, size_t num_pages, PageTableEntry entry_template, bool disable_head_merge, PageLinkedList *page_list, bool reuse_ll);

            Result Unmap(KProcessAddress virt_addr, size_t num_pages, PageLinkedList *page_list, bool force, bool reuse_ll);

            Result Map(KProcessAddress virt_addr, KPhysicalAddress phys_addr, size_t num_pages, PageTableEntry entry_template, bool disable_head_merge, size_t page_size, PageLinkedList *page_list, bool reuse_ll) {
                switch (page_size) {
                    case L1BlockSize:
                        return this->MapL1Blocks(virt_addr, phys_addr, num_pages, entry_template, disable_head_merge, page_list, reuse_ll);
                    case L2ContiguousBlockSize:
                        entry_template.SetContiguous(true);
                        [[fallthrough]];
#ifdef ATMOSPHERE_BOARD_NINTENDO_NX
                    case L2TegraSmmuBlockSize:
#endif
                    case L2BlockSize:
                        return this->MapL2Blocks(virt_addr, phys_addr, num_pages, entry_template, disable_head_merge, page_list, reuse_ll);
                    case L3ContiguousBlockSize:
                        entry_template.SetContiguous(true);
                        [[fallthrough]];
                    case L3BlockSize:
                        return this->MapL3Blocks(virt_addr, phys_addr, num_pages, entry_template, disable_head_merge, page_list, reuse_ll);
                    MESOSPHERE_UNREACHABLE_DEFAULT_CASE();
                }
            }

            Result MapContiguous(KProcessAddress virt_addr, KPhysicalAddress phys_addr, size_t num_pages, PageTableEntry entry_template, bool disable_head_merge, PageLinkedList *page_list, bool reuse_ll);
            Result MapGroup(KProcessAddress virt_addr, const KPageGroup &pg, size_t num_pages, PageTableEntry entry_template, bool disable_head_merge, PageLinkedList *page_list, bool reuse_ll);

            bool MergePages(KProcessAddress virt_addr, PageLinkedList *page_list);

            ALWAYS_INLINE Result SeparatePagesImpl(KProcessAddress virt_addr, size_t block_size, PageLinkedList *page_list, bool reuse_ll);
            Result SeparatePages(KProcessAddress virt_addr, size_t block_size, PageLinkedList *page_list, bool reuse_ll);

            Result ChangePermissions(KProcessAddress virt_addr, size_t num_pages, PageTableEntry entry_template, DisableMergeAttribute disable_merge_attr, bool refresh_mapping, PageLinkedList *page_list, bool reuse_ll);

            static void PteDataSynchronizationBarrier() {
                cpu::DataSynchronizationBarrierInnerShareable();
            }

            static void ClearPageTable(KVirtualAddress table) {
                cpu::ClearPageToZero(GetVoidPointer(table));
            }

            void OnTableUpdated() const {
                cpu::InvalidateTlbByAsid(m_asid);
            }

            void OnKernelTableUpdated() const {
                cpu::InvalidateEntireTlbDataOnly();
            }

            void OnKernelTableSinglePageUpdated(KProcessAddress virt_addr) const {
                cpu::InvalidateTlbByVaDataOnly(virt_addr);
            }

            void NoteUpdated() const {
                cpu::DataSynchronizationBarrier();

                if (this->IsKernel()) {
                    this->OnKernelTableUpdated();
                } else {
                    this->OnTableUpdated();
                }
            }

            void NoteSingleKernelPageUpdated(KProcessAddress virt_addr) const {
                MESOSPHERE_ASSERT(this->IsKernel());

                cpu::DataSynchronizationBarrier();
                this->OnKernelTableSinglePageUpdated(virt_addr);
            }

            KVirtualAddress AllocatePageTable(PageLinkedList *page_list, bool reuse_ll) const {
                KVirtualAddress table = this->GetPageTableManager().Allocate();

                if (table == Null<KVirtualAddress>) {
                    if (reuse_ll && page_list->Peek()) {
                        table = KVirtualAddress(reinterpret_cast<uintptr_t>(page_list->Pop()));
                    } else {
                        return Null<KVirtualAddress>;
                    }
                }

                MESOSPHERE_ASSERT(this->GetPageTableManager().GetRefCount(table) == 0);

                return table;
            }

            void FreePageTable(PageLinkedList *page_list, KVirtualAddress table) const {
                MESOSPHERE_ASSERT(this->GetPageTableManager().IsInPageTableHeap(table));
                MESOSPHERE_ASSERT(this->GetPageTableManager().GetRefCount(table) == 0);
                page_list->Push(table);
            }
    };

}
