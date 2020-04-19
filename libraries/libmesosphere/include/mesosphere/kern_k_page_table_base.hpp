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
        public:
            using TraversalEntry   = KPageTableImpl::TraversalEntry;
            using TraversalContext = KPageTableImpl::TraversalContext;
        protected:
            enum MemoryFillValue {
                MemoryFillValue_Zero  = 0,
                MemoryFillValue_Stack = 'X',
                MemoryFillValue_Ipc   = 'Y',
                MemoryFillValue_Heap  = 'Z',
            };

            enum OperationType {
                OperationType_Map                         = 0,
                OperationType_MapGroup                    = 1,
                OperationType_Unmap                       = 2,
                OperationType_ChangePermissions           = 3,
                OperationType_ChangePermissionsAndRefresh = 4,
                /* TODO: perm/attr operations */
            };

            static constexpr size_t MaxPhysicalMapAlignment = 1_GB;
            static constexpr size_t RegionAlignment         = 2_MB;
            static_assert(RegionAlignment == KernelAslrAlignment);

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

            static constexpr u32 DefaultMemoryIgnoreAttr = KMemoryAttribute_IpcLocked | KMemoryAttribute_DeviceShared;

            static constexpr size_t GetAddressSpaceWidth(ams::svc::CreateProcessFlag as_type) {
                switch (static_cast<ams::svc::CreateProcessFlag>(as_type & ams::svc::CreateProcessFlag_AddressSpaceMask)) {
                    case ams::svc::CreateProcessFlag_AddressSpace64Bit:
                        return 39;
                    case ams::svc::CreateProcessFlag_AddressSpace64BitDeprecated:
                        return 36;
                    case ams::svc::CreateProcessFlag_AddressSpace32Bit:
                    case ams::svc::CreateProcessFlag_AddressSpace32BitWithoutAlias:
                        return 32;
                    MESOSPHERE_UNREACHABLE_DEFAULT_CASE();
                }
            }
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
            size_t mapped_unsafe_physical_memory;
            mutable KLightLock general_lock;
            mutable KLightLock map_physical_memory_lock;
            KPageTableImpl impl;
            KMemoryBlockManager memory_block_manager;
            u32 allocate_option;
            u32 address_space_width;
            bool is_kernel;
            bool enable_aslr;
            KMemoryBlockSlabManager *memory_block_slab_manager;
            KBlockInfoManager *block_info_manager;
            const KMemoryRegion *cached_physical_linear_region;
            const KMemoryRegion *cached_physical_heap_region;
            const KMemoryRegion *cached_virtual_heap_region;
            MemoryFillValue heap_fill_value;
            MemoryFillValue ipc_fill_value;
            MemoryFillValue stack_fill_value;
        public:
            constexpr KPageTableBase() :
                address_space_start(), address_space_end(), heap_region_start(), heap_region_end(), current_heap_end(),
                alias_region_start(), alias_region_end(), stack_region_start(), stack_region_end(), kernel_map_region_start(),
                kernel_map_region_end(), alias_code_region_start(), alias_code_region_end(), code_region_start(), code_region_end(),
                max_heap_size(), max_physical_memory_size(),mapped_unsafe_physical_memory(), general_lock(), map_physical_memory_lock(),
                impl(), memory_block_manager(), allocate_option(), address_space_width(), is_kernel(), enable_aslr(), memory_block_slab_manager(),
                block_info_manager(), cached_physical_linear_region(), cached_physical_heap_region(), cached_virtual_heap_region(),
                heap_fill_value(), ipc_fill_value(), stack_fill_value()
            {
                /* ... */
            }

            NOINLINE Result InitializeForKernel(bool is_64_bit, void *table, KVirtualAddress start, KVirtualAddress end);
            NOINLINE Result InitializeForProcess(ams::svc::CreateProcessFlag as_type, bool enable_aslr, bool from_back, KMemoryManager::Pool pool, void *table, KProcessAddress start, KProcessAddress end, KProcessAddress code_address, size_t code_size, KMemoryBlockSlabManager *mem_block_slab_manager, KBlockInfoManager *block_info_manager);

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
            bool CanContain(KProcessAddress addr, size_t size, KMemoryState state) const;
        protected:
            virtual Result Operate(PageLinkedList *page_list, KProcessAddress virt_addr, size_t num_pages, KPhysicalAddress phys_addr, bool is_pa_valid, const KPageProperties properties, OperationType operation, bool reuse_ll) = 0;
            virtual Result Operate(PageLinkedList *page_list, KProcessAddress virt_addr, size_t num_pages, const KPageGroup &page_group, const KPageProperties properties, OperationType operation, bool reuse_ll) = 0;
            virtual void   FinalizeUpdate(PageLinkedList *page_list) = 0;

            KPageTableImpl &GetImpl() { return this->impl; }
            const KPageTableImpl &GetImpl() const { return this->impl; }

            bool IsLockedByCurrentThread() const { return this->general_lock.IsLockedByCurrentThread(); }

            bool IsHeapPhysicalAddress(KPhysicalAddress phys_addr) {
                MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

                return KMemoryLayout::IsHeapPhysicalAddress(std::addressof(this->cached_physical_heap_region), phys_addr, this->cached_physical_heap_region);
            }

            bool IsHeapPhysicalAddress(KPhysicalAddress phys_addr, size_t size) {
                MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

                return KMemoryLayout::IsHeapPhysicalAddress(std::addressof(this->cached_physical_heap_region), phys_addr, size, this->cached_physical_heap_region);
            }

            bool IsHeapVirtualAddress(KVirtualAddress virt_addr) {
                MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

                return KMemoryLayout::IsHeapVirtualAddress(std::addressof(this->cached_virtual_heap_region), virt_addr, this->cached_virtual_heap_region);
            }

            bool IsHeapVirtualAddress(KVirtualAddress virt_addr, size_t size) {
                MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

                return KMemoryLayout::IsHeapVirtualAddress(std::addressof(this->cached_virtual_heap_region), virt_addr, size, this->cached_virtual_heap_region);
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
            Result MapPageGroupImpl(PageLinkedList *page_list, KProcessAddress address, const KPageGroup &pg, const KPageProperties properties, bool reuse_ll);

            Result MakePageGroup(KPageGroup &pg, KProcessAddress addr, size_t num_pages);
            bool IsValidPageGroup(const KPageGroup &pg, KProcessAddress addr, size_t num_pages);

            NOINLINE Result MapPages(KProcessAddress *out_addr, size_t num_pages, size_t alignment, KPhysicalAddress phys_addr, bool is_pa_valid, KProcessAddress region_start, size_t region_num_pages, KMemoryState state, KMemoryPermission perm);
        public:
            bool GetPhysicalAddress(KPhysicalAddress *out, KProcessAddress virt_addr) const {
                return this->GetImpl().GetPhysicalAddress(out, virt_addr);
            }

            KBlockInfoManager *GetBlockInfoManager() const { return this->block_info_manager; }

            Result SetMemoryPermission(KProcessAddress addr, size_t size, ams::svc::MemoryPermission perm);
            Result SetProcessMemoryPermission(KProcessAddress addr, size_t size, ams::svc::MemoryPermission perm);
            Result SetHeapSize(KProcessAddress *out, size_t size);
            Result SetMaxHeapSize(size_t size);
            Result QueryInfo(KMemoryInfo *out_info, ams::svc::PageInfo *out_page_info, KProcessAddress addr) const;
            Result MapIo(KPhysicalAddress phys_addr, size_t size, KMemoryPermission perm);
            Result MapStatic(KPhysicalAddress phys_addr, size_t size, KMemoryPermission perm);
            Result MapRegion(KMemoryRegionType region_type, KMemoryPermission perm);

            Result MapPages(KProcessAddress *out_addr, size_t num_pages, size_t alignment, KPhysicalAddress phys_addr, KProcessAddress region_start, size_t region_num_pages, KMemoryState state, KMemoryPermission perm) {
                return this->MapPages(out_addr, num_pages, alignment, phys_addr, true, region_start, region_num_pages, state, perm);
            }

            Result MapPages(KProcessAddress *out_addr, size_t num_pages, size_t alignment, KPhysicalAddress phys_addr, KMemoryState state, KMemoryPermission perm) {
                return this->MapPages(out_addr, num_pages, alignment, phys_addr, true, this->GetRegionAddress(state), this->GetRegionSize(state) / PageSize, state, perm);
            }

            Result MapPages(KProcessAddress *out_addr, size_t num_pages, KMemoryState state, KMemoryPermission perm) {
                return this->MapPages(out_addr, num_pages, PageSize, Null<KPhysicalAddress>, false, this->GetRegionAddress(state), this->GetRegionSize(state) / PageSize, state, perm);
            }

            Result UnmapPages(KProcessAddress address, size_t num_pages, KMemoryState state);
            Result MapPageGroup(KProcessAddress *out_addr, const KPageGroup &pg, KProcessAddress region_start, size_t region_num_pages, KMemoryState state, KMemoryPermission perm);
            Result MapPageGroup(KProcessAddress address, const KPageGroup &pg, KMemoryState state, KMemoryPermission perm);
            Result UnmapPageGroup(KProcessAddress address, const KPageGroup &pg, KMemoryState state);

            Result MakeAndOpenPageGroup(KPageGroup *out, KProcessAddress address, size_t num_pages, u32 state_mask, u32 state, u32 perm_mask, u32 perm, u32 attr_mask, u32 attr);
        public:
            KProcessAddress GetAddressSpaceStart()    const { return this->address_space_start; }
            KProcessAddress GetHeapRegionStart()      const { return this->heap_region_start; }
            KProcessAddress GetAliasRegionStart()     const { return this->alias_region_start; }
            KProcessAddress GetStackRegionStart()     const { return this->stack_region_start; }
            KProcessAddress GetKernelMapRegionStart() const { return this->kernel_map_region_start; }
            KProcessAddress GetAliasCodeRegionStart() const { return this->alias_code_region_start; }

            size_t GetAddressSpaceSize()    const { return this->address_space_end     - this->address_space_start; }
            size_t GetHeapRegionSize()      const { return this->heap_region_end       - this->heap_region_start; }
            size_t GetAliasRegionSize()     const { return this->alias_region_end      - this->alias_region_start; }
            size_t GetStackRegionSize()     const { return this->stack_region_end      - this->stack_region_start; }
            size_t GetKernelMapRegionSize() const { return this->kernel_map_region_end - this->kernel_map_region_start; }
            size_t GetAliasCodeRegionSize() const { return this->alias_code_region_end - this->alias_code_region_start; }
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

            static ALWAYS_INLINE KPhysicalAddress GetHeapPhysicalAddress(KVirtualAddress addr) {
                return GetLinearPhysicalAddress(addr);
            }

            static ALWAYS_INLINE KVirtualAddress GetPageTableVirtualAddress(KPhysicalAddress addr) {
                return GetLinearVirtualAddress(addr);
            }

            static ALWAYS_INLINE KPhysicalAddress GetPageTablePhysicalAddress(KVirtualAddress addr) {
                return GetLinearPhysicalAddress(addr);
            }
    };

}
