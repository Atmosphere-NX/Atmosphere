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
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_select_cpu.hpp>
#include <mesosphere/kern_k_page_table_base.hpp>
#include <mesosphere/kern_k_page_group.hpp>
#include <mesosphere/kern_k_page_table_manager.hpp>

namespace ams::kern::arch::arm64 {

    class KPageTable final : public KPageTableBase {
        NON_COPYABLE(KPageTable);
        NON_MOVEABLE(KPageTable);
        private:
            friend class KPageTableBase;
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
        public:
            /* TODO: How should this size be determined. Does the KProcess slab count need to go in a header as a define? */
            static constexpr size_t NumTtbr0Entries = 81;
        private:
            static constinit inline const volatile u64 s_ttbr0_entries[NumTtbr0Entries] = {};
        private:
            KPageTableManager *m_manager;
            u8 m_asid;
        protected:
            Result OperateImpl(PageLinkedList *page_list, KProcessAddress virt_addr, size_t num_pages, KPhysicalAddress phys_addr, bool is_pa_valid, const KPageProperties properties, OperationType operation, bool reuse_ll);
            Result OperateImpl(PageLinkedList *page_list, KProcessAddress virt_addr, size_t num_pages, const KPageGroup &page_group, const KPageProperties properties, OperationType operation, bool reuse_ll);
            void FinalizeUpdateImpl(PageLinkedList *page_list);

            KPageTableManager &GetPageTableManager() const { return *m_manager; }
        private:
            constexpr PageTableEntry GetEntryTemplate(const KPageProperties properties) const {
                /* Check that the property is not kernel execute. */
                MESOSPHERE_ABORT_UNLESS((properties.perm & KMemoryPermission_KernelExecute) == 0);

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
                    MESOSPHERE_ABORT_UNLESS((properties.perm & KMemoryPermission_UserExecute) == 0);

                    entry.SetPageAttribute(PageTableEntry::PageAttribute_Device_nGnRnE)
                         .SetUserExecuteNever(true);
                } else if (properties.uncached) {
                    MESOSPHERE_ABORT_UNLESS((properties.perm & KMemoryPermission_UserExecute) == 0);

                    entry.SetPageAttribute(PageTableEntry::PageAttribute_NormalMemoryNotCacheable)
                         .SetUserExecuteNever(true);
                } else {
                    entry.SetPageAttribute(PageTableEntry::PageAttribute_NormalMemory);

                    if ((properties.perm & KMemoryPermission_UserExecute) != 0) {
                        /* Check that the permission is either r--/--x or r--/r-x. */
                        MESOSPHERE_ABORT_UNLESS((properties.perm & ~ams::svc::MemoryPermission_Read) == (KMemoryPermission_KernelRead | KMemoryPermission_UserExecute));
                    } else {
                        entry.SetUserExecuteNever(true);
                    }
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
            constexpr explicit KPageTable(util::ConstantInitializeTag) : KPageTableBase(util::ConstantInitialize), m_manager(), m_asid() { /* ... */ }
            explicit KPageTable() { /* ... */ }

            static NOINLINE void Initialize(s32 core_id);

            static const volatile u64 &GetTtbr0Entry(size_t index) { return s_ttbr0_entries[index]; }

            static ALWAYS_INLINE u64 GetKernelTtbr0() {
                return s_ttbr0_entries[0];
            }

            static ALWAYS_INLINE void ActivateKernel() {
                /* Activate, using asid 0 and process id = 0xFFFFFFFF */
                cpu::SwitchProcess(GetKernelTtbr0(), 0xFFFFFFFF);
            }

            static ALWAYS_INLINE void ActivateProcess(size_t proc_idx, u32 proc_id) {
                cpu::SwitchProcess(s_ttbr0_entries[proc_idx + 1], proc_id);
            }

            NOINLINE Result InitializeForKernel(void *table, KVirtualAddress start, KVirtualAddress end);
            NOINLINE Result InitializeForProcess(ams::svc::CreateProcessFlag flags, bool from_back, KMemoryManager::Pool pool, KProcessAddress code_address, size_t code_size, KSystemResource *system_resource, KResourceLimit *resource_limit, size_t process_index);
            Result Finalize();

            static void NoteUpdatedCallback(const void *pt) {
                /* Note the update. */
                static_cast<const KPageTable *>(pt)->NoteUpdated();
            }
        private:
            Result Unmap(KProcessAddress virt_addr, size_t num_pages, PageLinkedList *page_list, bool force, bool reuse_ll);

            Result Map(KProcessAddress virt_addr, KPhysicalAddress phys_addr, size_t num_pages, PageTableEntry entry_template, bool disable_head_merge, size_t page_size, PageLinkedList *page_list, bool reuse_ll);

            Result MapContiguous(KProcessAddress virt_addr, KPhysicalAddress phys_addr, size_t num_pages, PageTableEntry entry_template, bool disable_head_merge, PageLinkedList *page_list, bool reuse_ll);
            Result MapGroup(KProcessAddress virt_addr, const KPageGroup &pg, size_t num_pages, PageTableEntry entry_template, bool disable_head_merge, bool not_first, PageLinkedList *page_list, bool reuse_ll);

            bool MergePages(TraversalContext *context, PageLinkedList *page_list);
            void MergePages(KProcessAddress virt_addr, size_t num_pages, PageLinkedList *page_list);

            Result SeparatePagesImpl(TraversalEntry *entry, TraversalContext *context, KProcessAddress virt_addr, size_t block_size, PageLinkedList *page_list, bool reuse_ll);
            Result SeparatePages(KProcessAddress virt_addr, size_t num_pages, PageLinkedList *page_list, bool reuse_ll);

            Result ChangePermissions(KProcessAddress virt_addr, size_t num_pages, PageTableEntry entry_template, DisableMergeAttribute disable_merge_attr, bool refresh_mapping, bool flush_mapping, PageLinkedList *page_list, bool reuse_ll);

            static ALWAYS_INLINE void PteDataMemoryBarrier() {
                cpu::DataMemoryBarrierInnerShareableStore();
            }

            static ALWAYS_INLINE void ClearPageTable(KVirtualAddress table) {
                cpu::ClearPageToZero(GetVoidPointer(table));
            }

            ALWAYS_INLINE void OnTableUpdated() const {
                cpu::InvalidateTlbByAsid(m_asid);
            }

            ALWAYS_INLINE void OnKernelTableUpdated() const {
                cpu::InvalidateEntireTlbDataOnly();
            }

            ALWAYS_INLINE void OnKernelTableSinglePageUpdated(KProcessAddress virt_addr) const {
                cpu::InvalidateTlbByVaDataOnly(virt_addr);
            }

            void NoteUpdated() const;
            void NoteSingleKernelPageUpdated(KProcessAddress virt_addr) const;

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
