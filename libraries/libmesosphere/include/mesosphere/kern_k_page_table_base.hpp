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
#include <mesosphere/kern_select_page_table_impl.hpp>
#include <mesosphere/kern_k_light_lock.hpp>
#include <mesosphere/kern_k_page_group.hpp>
#include <mesosphere/kern_k_memory_manager.hpp>
#include <mesosphere/kern_k_memory_layout.hpp>
#include <mesosphere/kern_k_memory_block_manager.hpp>

namespace ams::kern {

    struct KPageProperties {
        KMemoryPermission perm;
        bool io;
        bool uncached;
        bool non_contiguous;
    };
    static_assert(std::is_trivial<KPageProperties>::value);

    class KPageTableBase {
        NON_COPYABLE(KPageTableBase);
        NON_MOVEABLE(KPageTableBase);
        protected:
            enum MemoryFillValue {
                MemoryFillValue_Zero  = 0,
                MemoryFillValue_Stack = 'X',
                MemoryFillValue_Ipc   = 'Y',
                MemoryFillValue_Heap  = 'Z',
            };

            enum OperationType {
                OperationType_Map      = 0,
                OperationType_MapGroup = 1,
                OperationType_Unmap    = 2,
                /* TODO: perm/attr operations */
            };


            struct PageLinkedList {
                private:
                    struct Node {
                        Node *next;
                        u8 buffer[PageSize - sizeof(Node *)];
                    };
                    static_assert(std::is_pod<Node>::value);
                private:
                    Node *root;
                public:
                    constexpr PageLinkedList() : root(nullptr) { /* ... */ }

                    void Push(Node *n) {
                        MESOSPHERE_ASSERT(util::IsAligned(reinterpret_cast<uintptr_t>(n), PageSize));
                        n->next = this->root;
                        this->root = n;
                    }

                    void Push(KVirtualAddress addr) {
                        this->Push(GetPointer<Node>(addr));
                    }

                    Node *Peek() const { return this->root; }

                    Node *Pop() {
                        Node *r = this->root;
                        this->root = this->root->next;
                        return r;
                    }
            };
            static_assert(std::is_trivially_destructible<PageLinkedList>::value);

            static constexpr u32 DefaultMemoryIgnoreAttr = KMemoryAttribute_DontCareMask | KMemoryAttribute_IpcLocked | KMemoryAttribute_DeviceShared;
        private:
            class KScopedPageTableUpdater {
                private:
                    KPageTableBase *page_table;
                    PageLinkedList ll;
                public:
                    ALWAYS_INLINE explicit KScopedPageTableUpdater(KPageTableBase *pt) : page_table(pt), ll() { /* ... */ }
                    ALWAYS_INLINE explicit KScopedPageTableUpdater(KPageTableBase &pt) : KScopedPageTableUpdater(std::addressof(pt)) { /* ... */ }
                    ALWAYS_INLINE ~KScopedPageTableUpdater() { this->page_table->FinalizeUpdate(this->GetPageList()); }

                    PageLinkedList *GetPageList() { return std::addressof(this->ll); }
            };
        private:
            KProcessAddress address_space_start;
            KProcessAddress address_space_end;
            KProcessAddress heap_region_start;
            KProcessAddress heap_region_end;
            KProcessAddress current_heap_end;
            KProcessAddress alias_region_start;
            KProcessAddress alias_region_end;
            KProcessAddress stack_region_start;
            KProcessAddress stack_region_end;
            KProcessAddress kernel_map_region_start;
            KProcessAddress kernel_map_region_end;
            KProcessAddress alias_code_region_start;
            KProcessAddress alias_code_region_end;
            KProcessAddress code_region_start;
            KProcessAddress code_region_end;
            size_t max_heap_size;
            size_t max_physical_memory_size;
            mutable KLightLock general_lock;
            mutable KLightLock map_physical_memory_lock;
            KPageTableImpl impl;
            KMemoryBlockManager memory_block_manager;
            u32 allocate_option;
            u32 address_space_size;
            bool is_kernel;
            bool enable_aslr;
            KMemoryBlockSlabManager *memory_block_slab_manager;
            KBlockInfoManager *block_info_manager;
            KMemoryRegion *cached_physical_linear_region;
            KMemoryRegion *cached_physical_heap_region;
            KMemoryRegion *cached_virtual_managed_pool_dram_region;
            MemoryFillValue heap_fill_value;
            MemoryFillValue ipc_fill_value;
            MemoryFillValue stack_fill_value;
        public:
            constexpr KPageTableBase() :
                address_space_start(), address_space_end(), heap_region_start(), heap_region_end(), current_heap_end(),
                alias_region_start(), alias_region_end(), stack_region_start(), stack_region_end(), kernel_map_region_start(),
                kernel_map_region_end(), alias_code_region_start(), alias_code_region_end(), code_region_start(), code_region_end(),
                max_heap_size(), max_physical_memory_size(), general_lock(), map_physical_memory_lock(), impl(), memory_block_manager(),
                allocate_option(), address_space_size(), is_kernel(), enable_aslr(), memory_block_slab_manager(), block_info_manager(),
                cached_physical_linear_region(), cached_physical_heap_region(), cached_virtual_managed_pool_dram_region(),
                heap_fill_value(), ipc_fill_value(), stack_fill_value()
            {
                /* ... */
            }

            NOINLINE Result InitializeForKernel(bool is_64_bit, void *table, KVirtualAddress start, KVirtualAddress end);

            void Finalize();

            constexpr bool IsKernel() const { return this->is_kernel; }
            constexpr bool IsAslrEnabled() const { return this->enable_aslr; }

            constexpr bool Contains(KProcessAddress addr) const {
                return this->address_space_start <= addr && addr <= this->address_space_end - 1;
            }

            constexpr bool Contains(KProcessAddress addr, size_t size) const {
                return this->address_space_start <= addr && addr < addr + size && addr + size - 1 <= this->address_space_end - 1;
            }

            KProcessAddress GetRegionAddress(KMemoryState state) const;
            size_t GetRegionSize(KMemoryState state) const;
            bool Contains(KProcessAddress addr, size_t size, KMemoryState state) const;
        protected:
            virtual Result Operate(PageLinkedList *page_list, KProcessAddress virt_addr, size_t num_pages, KPhysicalAddress phys_addr, bool is_pa_valid, const KPageProperties properties, OperationType operation, bool reuse_ll) = 0;
            virtual Result Operate(PageLinkedList *page_list, KProcessAddress virt_addr, size_t num_pages, const KPageGroup *page_group, const KPageProperties properties, OperationType operation, bool reuse_ll) = 0;
            virtual void   FinalizeUpdate(PageLinkedList *page_list) = 0;

            KPageTableImpl &GetImpl() { return this->impl; }
            const KPageTableImpl &GetImpl() const { return this->impl; }

            bool IsLockedByCurrentThread() const { return this->general_lock.IsLockedByCurrentThread(); }

            bool IsHeapPhysicalAddress(KPhysicalAddress phys_addr) {
                if (this->cached_physical_heap_region && this->cached_physical_heap_region->Contains(GetInteger(phys_addr))) {
                    return true;
                }
                return KMemoryLayout::IsHeapPhysicalAddress(&this->cached_physical_heap_region, phys_addr);
            }

            bool ContainsPages(KProcessAddress addr, size_t num_pages) const {
                return (this->address_space_start <= addr) && (num_pages <= (this->address_space_end - this->address_space_start) / PageSize) && (addr + num_pages * PageSize - 1 <= this->address_space_end - 1);
            }
        private:
            constexpr size_t GetNumGuardPages() const { return this->IsKernel() ? 1 : 4; }
            ALWAYS_INLINE KProcessAddress FindFreeArea(KProcessAddress region_start, size_t region_num_pages, size_t num_pages, size_t alignment, size_t offset, size_t guard_pages) const;

            Result CheckMemoryState(const KMemoryInfo &info, u32 state_mask, u32 state, u32 perm_mask, u32 perm, u32 attr_mask, u32 attr) const;
            Result CheckMemoryState(KMemoryState *out_state, KMemoryPermission *out_perm, KMemoryAttribute *out_attr, KProcessAddress addr, size_t size, u32 state_mask, u32 state, u32 perm_mask, u32 perm, u32 attr_mask, u32 attr, u32 ignore_attr = DefaultMemoryIgnoreAttr) const;
            Result CheckMemoryState(KProcessAddress addr, size_t size, u32 state_mask, u32 state, u32 perm_mask, u32 perm, u32 attr_mask, u32 attr, u32 ignore_attr = DefaultMemoryIgnoreAttr) const {
                return this->CheckMemoryState(nullptr, nullptr, nullptr, addr, size, state_mask, state, perm_mask, perm, attr_mask, attr, ignore_attr);
            }

            Result QueryInfoImpl(KMemoryInfo *out_info, ams::svc::PageInfo *out_page, KProcessAddress address) const;
            Result AllocateAndMapPagesImpl(PageLinkedList *page_list, KProcessAddress address, size_t num_pages, const KPageProperties properties);

            NOINLINE Result MapPages(KProcessAddress *out_addr, size_t num_pages, size_t alignment, KPhysicalAddress phys_addr, bool is_pa_valid, KProcessAddress region_start, size_t region_num_pages, KMemoryState state, KMemoryPermission perm);
        public:
            Result MapPages(KProcessAddress *out_addr, size_t num_pages, size_t alignment, KPhysicalAddress phys_addr, KProcessAddress region_start, size_t region_num_pages, KMemoryState state, KMemoryPermission perm) {
                return this->MapPages(out_addr, num_pages, alignment, phys_addr, true, region_start, region_num_pages, state, perm);
            }
        public:
            static ALWAYS_INLINE KVirtualAddress GetLinearVirtualAddress(KPhysicalAddress addr) {
                return KMemoryLayout::GetLinearVirtualAddress(addr);
            }

            static ALWAYS_INLINE KPhysicalAddress GetLinearPhysicalAddress(KVirtualAddress addr) {
                return KMemoryLayout::GetLinearPhysicalAddress(addr);
            }

            static ALWAYS_INLINE KVirtualAddress GetHeapVirtualAddress(KPhysicalAddress addr) {
                return GetLinearVirtualAddress(addr);
            }

            static ALWAYS_INLINE KVirtualAddress GetPageTableVirtualAddress(KPhysicalAddress addr) {
                return GetLinearVirtualAddress(addr);
            }

            static ALWAYS_INLINE KPhysicalAddress GetPageTablePhysicalAddress(KVirtualAddress addr) {
                return GetLinearPhysicalAddress(addr);
            }
    };

}
