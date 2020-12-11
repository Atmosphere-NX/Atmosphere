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

    enum DisableMergeAttribute : u8 {
        DisableMergeAttribute_None                       = (0u << 0),

        DisableMergeAttribute_DisableHead                = (1u << 0),
        DisableMergeAttribute_DisableHeadAndBody         = (1u << 1),
        DisableMergeAttribute_EnableHeadAndBody          = (1u << 2),
        DisableMergeAttribute_DisableTail                = (1u << 3),
        DisableMergeAttribute_EnableTail                 = (1u << 4),
        DisableMergeAttribute_EnableAndMergeHeadBodyTail = (1u << 5),

        DisableMergeAttribute_EnableHeadBodyTail         = DisableMergeAttribute_EnableHeadAndBody  | DisableMergeAttribute_EnableTail,
        DisableMergeAttribute_DisableHeadBodyTail        = DisableMergeAttribute_DisableHeadAndBody | DisableMergeAttribute_DisableTail,
    };

    struct KPageProperties {
        KMemoryPermission perm;
        bool io;
        bool uncached;
        DisableMergeAttribute disable_merge_attributes;
    };
    static_assert(std::is_trivial<KPageProperties>::value);
    static_assert(sizeof(KPageProperties) == sizeof(u32));

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
                    static_assert(util::is_pod<Node>::value);
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
            size_t mapped_physical_memory_size;
            size_t mapped_unsafe_physical_memory;
            mutable KLightLock general_lock;
            mutable KLightLock map_physical_memory_lock;
            KPageTableImpl impl;
            KMemoryBlockManager memory_block_manager;
            u32 allocate_option;
            u32 address_space_width;
            bool is_kernel;
            bool enable_aslr;
            bool enable_device_address_space_merge;
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
                max_heap_size(), mapped_physical_memory_size(), mapped_unsafe_physical_memory(), general_lock(), map_physical_memory_lock(),
                impl(), memory_block_manager(), allocate_option(), address_space_width(), is_kernel(), enable_aslr(), enable_device_address_space_merge(),
                memory_block_slab_manager(), block_info_manager(), cached_physical_linear_region(), cached_physical_heap_region(), cached_virtual_heap_region(),
                heap_fill_value(), ipc_fill_value(), stack_fill_value()
            {
                /* ... */
            }

            NOINLINE Result InitializeForKernel(bool is_64_bit, void *table, KVirtualAddress start, KVirtualAddress end);
            NOINLINE Result InitializeForProcess(ams::svc::CreateProcessFlag as_type, bool enable_aslr, bool enable_device_address_space_merge, bool from_back, KMemoryManager::Pool pool, void *table, KProcessAddress start, KProcessAddress end, KProcessAddress code_address, size_t code_size, KMemoryBlockSlabManager *mem_block_slab_manager, KBlockInfoManager *block_info_manager);

            void Finalize();

            constexpr bool IsKernel() const { return this->is_kernel; }
            constexpr bool IsAslrEnabled() const { return this->enable_aslr; }

            constexpr bool Contains(KProcessAddress addr) const {
                return this->address_space_start <= addr && addr <= this->address_space_end - 1;
            }

            constexpr bool Contains(KProcessAddress addr, size_t size) const {
                return this->address_space_start <= addr && addr < addr + size && addr + size - 1 <= this->address_space_end - 1;
            }

            constexpr bool IsInAliasRegion(KProcessAddress addr, size_t size) const {
                return this->Contains(addr, size) && this->alias_region_start <= addr && addr + size - 1 <= this->alias_region_end - 1;
            }

            bool IsInUnsafeAliasRegion(KProcessAddress addr, size_t size) const {
                /* Even though Unsafe physical memory is KMemoryState_Normal, it must be mapped inside the alias code region. */
                return this->CanContain(addr, size, KMemoryState_AliasCode);
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

            bool IsLinearMappedPhysicalAddress(KPhysicalAddress phys_addr) {
                MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

                return KMemoryLayout::IsLinearMappedPhysicalAddress(this->cached_physical_linear_region, phys_addr);
            }

            bool IsLinearMappedPhysicalAddress(KPhysicalAddress phys_addr, size_t size) {
                MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

                return KMemoryLayout::IsLinearMappedPhysicalAddress(this->cached_physical_linear_region, phys_addr, size);
            }

            bool IsHeapPhysicalAddress(KPhysicalAddress phys_addr) {
                MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

                return KMemoryLayout::IsHeapPhysicalAddress(this->cached_physical_heap_region, phys_addr);
            }

            bool IsHeapPhysicalAddress(KPhysicalAddress phys_addr, size_t size) {
                MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

                return KMemoryLayout::IsHeapPhysicalAddress(this->cached_physical_heap_region, phys_addr, size);
            }

            bool IsHeapPhysicalAddressForFinalize(KPhysicalAddress phys_addr) {
                MESOSPHERE_ASSERT(!this->IsLockedByCurrentThread());

                return KMemoryLayout::IsHeapPhysicalAddress(this->cached_physical_heap_region, phys_addr);
            }

            bool IsHeapVirtualAddress(KVirtualAddress virt_addr) {
                MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

                return KMemoryLayout::IsHeapVirtualAddress(this->cached_virtual_heap_region, virt_addr);
            }

            bool IsHeapVirtualAddress(KVirtualAddress virt_addr, size_t size) {
                MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

                return KMemoryLayout::IsHeapVirtualAddress(this->cached_virtual_heap_region, virt_addr, size);
            }

            bool ContainsPages(KProcessAddress addr, size_t num_pages) const {
                return (this->address_space_start <= addr) && (num_pages <= (this->address_space_end - this->address_space_start) / PageSize) && (addr + num_pages * PageSize - 1 <= this->address_space_end - 1);
            }
        private:
            constexpr size_t GetNumGuardPages() const { return this->IsKernel() ? 1 : 4; }
            ALWAYS_INLINE KProcessAddress FindFreeArea(KProcessAddress region_start, size_t region_num_pages, size_t num_pages, size_t alignment, size_t offset, size_t guard_pages) const;

            Result CheckMemoryStateContiguous(size_t *out_blocks_needed, KProcessAddress addr, size_t size, u32 state_mask, u32 state, u32 perm_mask, u32 perm, u32 attr_mask, u32 attr) const;
            Result CheckMemoryStateContiguous(KProcessAddress addr, size_t size, u32 state_mask, u32 state, u32 perm_mask, u32 perm, u32 attr_mask, u32 attr) const {
                return this->CheckMemoryStateContiguous(nullptr, addr, size, state_mask, state, perm_mask, perm, attr_mask, attr);
            }

            Result CheckMemoryState(const KMemoryInfo &info, u32 state_mask, u32 state, u32 perm_mask, u32 perm, u32 attr_mask, u32 attr) const;
            Result CheckMemoryState(KMemoryState *out_state, KMemoryPermission *out_perm, KMemoryAttribute *out_attr, size_t *out_blocks_needed, KProcessAddress addr, size_t size, u32 state_mask, u32 state, u32 perm_mask, u32 perm, u32 attr_mask, u32 attr, u32 ignore_attr = DefaultMemoryIgnoreAttr) const;
            Result CheckMemoryState(size_t *out_blocks_needed, KProcessAddress addr, size_t size, u32 state_mask, u32 state, u32 perm_mask, u32 perm, u32 attr_mask, u32 attr, u32 ignore_attr = DefaultMemoryIgnoreAttr) const {
                return this->CheckMemoryState(nullptr, nullptr, nullptr, out_blocks_needed, addr, size, state_mask, state, perm_mask, perm, attr_mask, attr, ignore_attr);
            }
            Result CheckMemoryState(KProcessAddress addr, size_t size, u32 state_mask, u32 state, u32 perm_mask, u32 perm, u32 attr_mask, u32 attr, u32 ignore_attr = DefaultMemoryIgnoreAttr) const {
                return this->CheckMemoryState(nullptr, addr, size, state_mask, state, perm_mask, perm, attr_mask, attr, ignore_attr);
            }

            Result LockMemoryAndOpen(KPageGroup *out_pg, KPhysicalAddress *out_paddr, KProcessAddress addr, size_t size, u32 state_mask, u32 state, u32 perm_mask, u32 perm, u32 attr_mask, u32 attr, KMemoryPermission new_perm, u32 lock_attr);
            Result UnlockMemory(KProcessAddress addr, size_t size, u32 state_mask, u32 state, u32 perm_mask, u32 perm, u32 attr_mask, u32 attr, KMemoryPermission new_perm, u32 lock_attr, const KPageGroup *pg);

            Result QueryInfoImpl(KMemoryInfo *out_info, ams::svc::PageInfo *out_page, KProcessAddress address) const;

            Result QueryMappingImpl(KProcessAddress *out, KPhysicalAddress address, size_t size, KMemoryState state) const;

            Result AllocateAndMapPagesImpl(PageLinkedList *page_list, KProcessAddress address, size_t num_pages, KMemoryPermission perm);
            Result MapPageGroupImpl(PageLinkedList *page_list, KProcessAddress address, const KPageGroup &pg, const KPageProperties properties, bool reuse_ll);

            void RemapPageGroup(PageLinkedList *page_list, KProcessAddress address, size_t size, const KPageGroup &pg);

            Result MakePageGroup(KPageGroup &pg, KProcessAddress addr, size_t num_pages);
            bool IsValidPageGroup(const KPageGroup &pg, KProcessAddress addr, size_t num_pages);

            NOINLINE Result MapPages(KProcessAddress *out_addr, size_t num_pages, size_t alignment, KPhysicalAddress phys_addr, bool is_pa_valid, KProcessAddress region_start, size_t region_num_pages, KMemoryState state, KMemoryPermission perm);

            Result SetupForIpcClient(PageLinkedList *page_list, size_t *out_blocks_needed, KProcessAddress address, size_t size, KMemoryPermission test_perm, KMemoryState dst_state);
            Result SetupForIpcServer(KProcessAddress *out_addr, size_t size, KProcessAddress src_addr, KMemoryPermission test_perm, KMemoryState dst_state, KPageTableBase &src_page_table, bool send);
            void CleanupForIpcClientOnServerSetupFailure(PageLinkedList *page_list, KProcessAddress address, size_t size, KMemoryPermission prot_perm);

            size_t GetSize(KMemoryState state) const;
        public:
            bool GetPhysicalAddress(KPhysicalAddress *out, KProcessAddress virt_addr) const {
                return this->GetImpl().GetPhysicalAddress(out, virt_addr);
            }

            KBlockInfoManager *GetBlockInfoManager() const { return this->block_info_manager; }

            Result SetMemoryPermission(KProcessAddress addr, size_t size, ams::svc::MemoryPermission perm);
            Result SetProcessMemoryPermission(KProcessAddress addr, size_t size, ams::svc::MemoryPermission perm);
            Result SetMemoryAttribute(KProcessAddress addr, size_t size, u32 mask, u32 attr);
            Result SetHeapSize(KProcessAddress *out, size_t size);
            Result SetMaxHeapSize(size_t size);
            Result QueryInfo(KMemoryInfo *out_info, ams::svc::PageInfo *out_page_info, KProcessAddress addr) const;
            Result QueryPhysicalAddress(ams::svc::PhysicalMemoryInfo *out, KProcessAddress address) const;
            Result QueryStaticMapping(KProcessAddress *out, KPhysicalAddress address, size_t size) const { return this->QueryMappingImpl(out, address, size, KMemoryState_Static); }
            Result QueryIoMapping(KProcessAddress *out, KPhysicalAddress address, size_t size) const { return this->QueryMappingImpl(out, address, size, KMemoryState_Io); }
            Result MapMemory(KProcessAddress dst_address, KProcessAddress src_address, size_t size);
            Result UnmapMemory(KProcessAddress dst_address, KProcessAddress src_address, size_t size);
            Result MapCodeMemory(KProcessAddress dst_address, KProcessAddress src_address, size_t size);
            Result UnmapCodeMemory(KProcessAddress dst_address, KProcessAddress src_address, size_t size);
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

            Result MapPages(KProcessAddress address, size_t num_pages, KMemoryState state, KMemoryPermission perm);
            Result UnmapPages(KProcessAddress address, size_t num_pages, KMemoryState state);

            Result MapPageGroup(KProcessAddress *out_addr, const KPageGroup &pg, KProcessAddress region_start, size_t region_num_pages, KMemoryState state, KMemoryPermission perm);
            Result MapPageGroup(KProcessAddress address, const KPageGroup &pg, KMemoryState state, KMemoryPermission perm);
            Result UnmapPageGroup(KProcessAddress address, const KPageGroup &pg, KMemoryState state);

            Result MakeAndOpenPageGroup(KPageGroup *out, KProcessAddress address, size_t num_pages, u32 state_mask, u32 state, u32 perm_mask, u32 perm, u32 attr_mask, u32 attr);

            Result InvalidateProcessDataCache(KProcessAddress address, size_t size);

            Result ReadDebugMemory(void *buffer, KProcessAddress address, size_t size);
            Result WriteDebugMemory(KProcessAddress address, const void *buffer, size_t size);

            Result LockForDeviceAddressSpace(KPageGroup *out, KProcessAddress address, size_t size, KMemoryPermission perm, bool is_aligned);
            Result UnlockForDeviceAddressSpace(KProcessAddress address, size_t size);

            Result MakePageGroupForUnmapDeviceAddressSpace(KPageGroup *out, KProcessAddress address, size_t size);
            Result UnlockForDeviceAddressSpacePartialMap(KProcessAddress address, size_t size, size_t mapped_size);

            Result LockForIpcUserBuffer(KPhysicalAddress *out, KProcessAddress address, size_t size);
            Result UnlockForIpcUserBuffer(KProcessAddress address, size_t size);

            Result LockForTransferMemory(KPageGroup *out, KProcessAddress address, size_t size, KMemoryPermission perm);
            Result UnlockForTransferMemory(KProcessAddress address, size_t size, const KPageGroup &pg);
            Result LockForCodeMemory(KPageGroup *out, KProcessAddress address, size_t size);
            Result UnlockForCodeMemory(KProcessAddress address, size_t size, const KPageGroup &pg);

            Result CopyMemoryFromLinearToUser(KProcessAddress dst_addr, size_t size, KProcessAddress src_addr, u32 src_state_mask, u32 src_state, KMemoryPermission src_test_perm, u32 src_attr_mask, u32 src_attr);
            Result CopyMemoryFromLinearToKernel(KProcessAddress dst_addr, size_t size, KProcessAddress src_addr, u32 src_state_mask, u32 src_state, KMemoryPermission src_test_perm, u32 src_attr_mask, u32 src_attr);
            Result CopyMemoryFromUserToLinear(KProcessAddress dst_addr, size_t size, u32 dst_state_mask, u32 dst_state, KMemoryPermission dst_test_perm, u32 dst_attr_mask, u32 dst_attr, KProcessAddress src_addr);
            Result CopyMemoryFromKernelToLinear(KProcessAddress dst_addr, size_t size, u32 dst_state_mask, u32 dst_state, KMemoryPermission dst_test_perm, u32 dst_attr_mask, u32 dst_attr, KProcessAddress src_addr);
            Result CopyMemoryFromHeapToHeap(KPageTableBase &dst_page_table, KProcessAddress dst_addr, size_t size, u32 dst_state_mask, u32 dst_state, KMemoryPermission dst_test_perm, u32 dst_attr_mask, u32 dst_attr, KProcessAddress src_addr, u32 src_state_mask, u32 src_state, KMemoryPermission src_test_perm, u32 src_attr_mask, u32 src_attr);
            Result CopyMemoryFromHeapToHeapWithoutCheckDestination(KPageTableBase &dst_page_table, KProcessAddress dst_addr, size_t size, u32 dst_state_mask, u32 dst_state, KMemoryPermission dst_test_perm, u32 dst_attr_mask, u32 dst_attr, KProcessAddress src_addr, u32 src_state_mask, u32 src_state, KMemoryPermission src_test_perm, u32 src_attr_mask, u32 src_attr);

            Result SetupForIpc(KProcessAddress *out_dst_addr, size_t size, KProcessAddress src_addr, KPageTableBase &src_page_table, KMemoryPermission test_perm, KMemoryState dst_state, bool send);
            Result CleanupForIpcServer(KProcessAddress address, size_t size, KMemoryState dst_state, KProcess *server_process);
            Result CleanupForIpcClient(KProcessAddress address, size_t size, KMemoryState dst_state);

            Result MapPhysicalMemory(KProcessAddress address, size_t size);
            Result UnmapPhysicalMemory(KProcessAddress address, size_t size);

            Result MapPhysicalMemoryUnsafe(KProcessAddress address, size_t size);
            Result UnmapPhysicalMemoryUnsafe(KProcessAddress address, size_t size);

            void DumpMemoryBlocksLocked() const {
                MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());
                this->memory_block_manager.DumpBlocks();
            }

            void DumpMemoryBlocks() const {
                KScopedLightLock lk(this->general_lock);
                this->DumpMemoryBlocksLocked();
            }

            void DumpPageTable() const {
                KScopedLightLock lk(this->general_lock);
                this->GetImpl().Dump(GetInteger(this->address_space_start), this->address_space_end - this->address_space_start);
            }

            size_t CountPageTables() const {
                KScopedLightLock lk(this->general_lock);
                return this->GetImpl().CountPageTables();
            }
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

            size_t GetNormalMemorySize() const {
                /* Lock the table. */
                KScopedLightLock lk(this->general_lock);

                return (this->current_heap_end - this->heap_region_start) + this->mapped_physical_memory_size;
            }

            size_t GetCodeSize() const;
            size_t GetCodeDataSize() const;
            size_t GetAliasCodeSize() const;
            size_t GetAliasCodeDataSize() const;

            u32 GetAllocateOption() const { return this->allocate_option; }
        public:
            static ALWAYS_INLINE KVirtualAddress GetLinearMappedVirtualAddress(KPhysicalAddress addr) {
                return KMemoryLayout::GetLinearVirtualAddress(addr);
            }

            static ALWAYS_INLINE KPhysicalAddress GetLinearMappedPhysicalAddress(KVirtualAddress addr) {
                return KMemoryLayout::GetLinearPhysicalAddress(addr);
            }

            static ALWAYS_INLINE KVirtualAddress GetHeapVirtualAddress(KPhysicalAddress addr) {
                return GetLinearMappedVirtualAddress(addr);
            }

            static ALWAYS_INLINE KPhysicalAddress GetHeapPhysicalAddress(KVirtualAddress addr) {
                return GetLinearMappedPhysicalAddress(addr);
            }

            static ALWAYS_INLINE KVirtualAddress GetPageTableVirtualAddress(KPhysicalAddress addr) {
                return GetLinearMappedVirtualAddress(addr);
            }

            static ALWAYS_INLINE KPhysicalAddress GetPageTablePhysicalAddress(KVirtualAddress addr) {
                return GetLinearMappedPhysicalAddress(addr);
            }
    };

}
