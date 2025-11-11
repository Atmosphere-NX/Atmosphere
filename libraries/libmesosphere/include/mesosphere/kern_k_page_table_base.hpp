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

    class KResourceLimit;
    class KSystemResource;

    class KPageTableBase {
        NON_COPYABLE(KPageTableBase);
        NON_MOVEABLE(KPageTableBase);
        public:
            using TraversalEntry   = KPageTableImpl::TraversalEntry;
            using TraversalContext = KPageTableImpl::TraversalContext;

            class MemoryRange {
                private:
                    KPhysicalAddress m_address;
                    size_t m_size;
                    bool m_heap;
                    u8 m_attr;
                public:
                    constexpr MemoryRange() : m_address(Null<KPhysicalAddress>), m_size(0), m_heap(false), m_attr(0) { /* ... */ }

                    void Set(KPhysicalAddress address, size_t size, bool heap, u8 attr) {
                        m_address = address;
                        m_size    = size;
                        m_heap    = heap;
                        m_attr    = attr;
                    }

                    constexpr KPhysicalAddress GetAddress() const { return m_address; }
                    constexpr size_t GetSize() const { return m_size; }
                    constexpr bool IsHeap() const { return m_heap; }
                    constexpr u8 GetAttribute() const { return m_attr; }

                    void Open();
                    void Close();
            };
        protected:
            enum MemoryFillValue {
                MemoryFillValue_Zero  = 0,
                MemoryFillValue_Stack = 'X',
                MemoryFillValue_Ipc   = 'Y',
                MemoryFillValue_Heap  = 'Z',
            };

            enum RegionType {
                RegionType_KernelMap = 0,
                RegionType_Stack     = 1,
                RegionType_Alias     = 2,
                RegionType_Heap      = 3,

                RegionType_Count,
            };

            enum OperationType {
                OperationType_Map                                 = 0,
                OperationType_MapGroup                            = 1,
                OperationType_MapFirstGroup                       = 2,
                OperationType_Unmap                               = 3,
                OperationType_ChangePermissions                   = 4,
                OperationType_ChangePermissionsAndRefresh         = 5,
                OperationType_ChangePermissionsAndRefreshAndFlush = 6,
                OperationType_Separate                            = 7,
            };

            static constexpr size_t MaxPhysicalMapAlignment = 1_GB;
            static constexpr size_t RegionAlignment         = 2_MB;
            static_assert(RegionAlignment == KernelAslrAlignment);

            struct PageLinkedList {
                private:
                    struct Node {
                        Node *m_next;
                        u8 m_buffer[PageSize - sizeof(Node *)];
                    };
                    static_assert(util::is_pod<Node>::value);
                private:
                    Node *m_root;
                public:
                    constexpr PageLinkedList() : m_root(nullptr) { /* ... */ }

                    void Push(Node *n) {
                        MESOSPHERE_ASSERT(util::IsAligned(reinterpret_cast<uintptr_t>(n), PageSize));
                        n->m_next = m_root;
                        m_root    = n;
                    }

                    void Push(KVirtualAddress addr) {
                        this->Push(GetPointer<Node>(addr));
                    }

                    Node *Peek() const { return m_root; }

                    Node *Pop() {
                        Node * const r = m_root;

                        m_root    = r->m_next;
                        r->m_next = nullptr;

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
                    KPageTableBase *m_pt;
                    PageLinkedList m_ll;
                public:
                    ALWAYS_INLINE explicit KScopedPageTableUpdater(KPageTableBase *pt) : m_pt(pt), m_ll() { /* ... */ }
                    ALWAYS_INLINE explicit KScopedPageTableUpdater(KPageTableBase &pt) : KScopedPageTableUpdater(std::addressof(pt)) { /* ... */ }
                    ALWAYS_INLINE ~KScopedPageTableUpdater() { m_pt->FinalizeUpdate(this->GetPageList()); }

                    PageLinkedList *GetPageList() { return std::addressof(m_ll); }
            };
        private:
            KProcessAddress m_address_space_start;
            KProcessAddress m_address_space_end;
            KProcessAddress m_region_starts[RegionType_Count];
            KProcessAddress m_region_ends[RegionType_Count];
            KProcessAddress m_current_heap_end;
            KProcessAddress m_alias_code_region_start;
            KProcessAddress m_alias_code_region_end;
            KProcessAddress m_code_region_start;
            KProcessAddress m_code_region_end;
            size_t m_max_heap_size;
            size_t m_mapped_physical_memory_size;
            size_t m_mapped_unsafe_physical_memory;
            size_t m_mapped_insecure_memory;
            size_t m_mapped_ipc_server_memory;
            size_t m_alias_region_extra_size;
            mutable KLightLock m_general_lock;
            mutable KLightLock m_map_physical_memory_lock;
            KLightLock m_device_map_lock;
            KPageTableImpl m_impl;
            KMemoryBlockManager m_memory_block_manager;
            u32 m_allocate_option;
            u32 m_address_space_width;
            bool m_is_kernel;
            bool m_enable_aslr;
            bool m_enable_device_address_space_merge;
            KMemoryBlockSlabManager *m_memory_block_slab_manager;
            KBlockInfoManager *m_block_info_manager;
            KResourceLimit *m_resource_limit;
            const KMemoryRegion *m_cached_physical_linear_region;
            const KMemoryRegion *m_cached_physical_heap_region;
            MemoryFillValue m_heap_fill_value;
            MemoryFillValue m_ipc_fill_value;
            MemoryFillValue m_stack_fill_value;
        public:
            constexpr explicit KPageTableBase(util::ConstantInitializeTag)
                : m_address_space_start(Null<KProcessAddress>), m_address_space_end(Null<KProcessAddress>),
                  m_region_starts{Null<KProcessAddress>, Null<KProcessAddress>, Null<KProcessAddress>, Null<KProcessAddress>},
                  m_region_ends{Null<KProcessAddress>, Null<KProcessAddress>, Null<KProcessAddress>, Null<KProcessAddress>},
                  m_current_heap_end(Null<KProcessAddress>), m_alias_code_region_start(Null<KProcessAddress>),
                  m_alias_code_region_end(Null<KProcessAddress>), m_code_region_start(Null<KProcessAddress>), m_code_region_end(Null<KProcessAddress>),
                  m_max_heap_size(), m_mapped_physical_memory_size(), m_mapped_unsafe_physical_memory(), m_mapped_insecure_memory(), m_mapped_ipc_server_memory(), m_alias_region_extra_size(),
                  m_general_lock(), m_map_physical_memory_lock(), m_device_map_lock(), m_impl(util::ConstantInitialize), m_memory_block_manager(util::ConstantInitialize),
                  m_allocate_option(), m_address_space_width(), m_is_kernel(), m_enable_aslr(), m_enable_device_address_space_merge(),
                  m_memory_block_slab_manager(), m_block_info_manager(), m_resource_limit(), m_cached_physical_linear_region(), m_cached_physical_heap_region(),
                  m_heap_fill_value(), m_ipc_fill_value(), m_stack_fill_value()
            {
                /* ... */
            }

            explicit KPageTableBase() { /* ... */ }

            NOINLINE void InitializeForKernel(bool is_64_bit, void *table, KVirtualAddress start, KVirtualAddress end);
            NOINLINE Result InitializeForProcess(ams::svc::CreateProcessFlag flags, bool from_back, KMemoryManager::Pool pool, void *table, KProcessAddress start, KProcessAddress end, KProcessAddress code_address, size_t code_size, KSystemResource *system_resource, KResourceLimit *resource_limit);

            void Finalize();

            constexpr bool IsKernel() const { return m_is_kernel; }
            constexpr bool IsAslrEnabled() const { return m_enable_aslr; }

            constexpr bool Contains(KProcessAddress addr) const {
                return m_address_space_start <= addr && addr <= m_address_space_end - 1;
            }

            constexpr bool Contains(KProcessAddress addr, size_t size) const {
                return m_address_space_start <= addr && addr < addr + size && addr + size - 1 <= m_address_space_end - 1;
            }

            constexpr bool IsInAliasRegion(KProcessAddress addr, size_t size) const {
                return this->Contains(addr, size) && m_region_starts[RegionType_Alias] <= addr && addr + size - 1 <= m_region_ends[RegionType_Alias] - 1;
            }

            bool IsInUnsafeAliasRegion(KProcessAddress addr, size_t size) const {
                /* Even though Unsafe physical memory is KMemoryState_Normal, it must be mapped inside the alias code region. */
                return this->CanContain(addr, size, ams::svc::MemoryState_AliasCode);
            }

            ALWAYS_INLINE KScopedLightLock AcquireDeviceMapLock() {
                return KScopedLightLock(m_device_map_lock);
            }

            KProcessAddress GetRegionAddress(ams::svc::MemoryState state) const;
            size_t GetRegionSize(ams::svc::MemoryState state) const;
            bool CanContain(KProcessAddress addr, size_t size, ams::svc::MemoryState state) const;

            ALWAYS_INLINE KProcessAddress GetRegionAddress(KMemoryState state) const { return this->GetRegionAddress(static_cast<ams::svc::MemoryState>(state & KMemoryState_Mask)); }
            ALWAYS_INLINE size_t GetRegionSize(KMemoryState state) const { return this->GetRegionSize(static_cast<ams::svc::MemoryState>(state & KMemoryState_Mask)); }
            ALWAYS_INLINE bool CanContain(KProcessAddress addr, size_t size, KMemoryState state) const { return this->CanContain(addr, size, static_cast<ams::svc::MemoryState>(state & KMemoryState_Mask)); }
        protected:
            /* NOTE: These three functions (Operate, Operate, FinalizeUpdate) are virtual functions */
            /* in Nintendo's kernel. We devirtualize them, since KPageTable is the only derived */
            /* class, and this avoids unnecessary virtual function calls. See "kern_select_page_table.hpp" */
            /* for definition of these functions. */
            Result Operate(PageLinkedList *page_list, KProcessAddress virt_addr, size_t num_pages, KPhysicalAddress phys_addr, bool is_pa_valid, const KPageProperties properties, OperationType operation, bool reuse_ll);
            Result Operate(PageLinkedList *page_list, KProcessAddress virt_addr, size_t num_pages, const KPageGroup &page_group, const KPageProperties properties, OperationType operation, bool reuse_ll);
            void FinalizeUpdate(PageLinkedList *page_list);

            ALWAYS_INLINE KPageTableImpl &GetImpl() { return m_impl; }
            ALWAYS_INLINE const KPageTableImpl &GetImpl() const { return m_impl; }

            ALWAYS_INLINE bool IsLockedByCurrentThread() const { return m_general_lock.IsLockedByCurrentThread(); }

            ALWAYS_INLINE bool IsLinearMappedPhysicalAddress(KPhysicalAddress phys_addr) {
                MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

                return KMemoryLayout::IsLinearMappedPhysicalAddress(m_cached_physical_linear_region, phys_addr);
            }

            ALWAYS_INLINE bool IsLinearMappedPhysicalAddress(KPhysicalAddress phys_addr, size_t size) {
                MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

                return KMemoryLayout::IsLinearMappedPhysicalAddress(m_cached_physical_linear_region, phys_addr, size);
            }

            ALWAYS_INLINE bool IsHeapPhysicalAddress(KPhysicalAddress phys_addr) {
                MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

                return KMemoryLayout::IsHeapPhysicalAddress(m_cached_physical_heap_region, phys_addr);
            }

            ALWAYS_INLINE bool IsHeapPhysicalAddress(KPhysicalAddress phys_addr, size_t size) {
                MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

                return KMemoryLayout::IsHeapPhysicalAddress(m_cached_physical_heap_region, phys_addr, size);
            }

            ALWAYS_INLINE bool IsHeapPhysicalAddressForFinalize(KPhysicalAddress phys_addr) {
                MESOSPHERE_ASSERT(!this->IsLockedByCurrentThread());

                return KMemoryLayout::IsHeapPhysicalAddress(m_cached_physical_heap_region, phys_addr);
            }

            ALWAYS_INLINE bool ContainsPages(KProcessAddress addr, size_t num_pages) const {
                return (m_address_space_start <= addr) && (num_pages <= (m_address_space_end - m_address_space_start) / PageSize) && (addr + num_pages * PageSize - 1 <= m_address_space_end - 1);
            }
        private:
            constexpr size_t GetNumGuardPages() const { return this->IsKernel() ? 1 : 4; }
            ALWAYS_INLINE KProcessAddress FindFreeArea(KProcessAddress region_start, size_t region_num_pages, size_t num_pages, size_t alignment, size_t offset, size_t guard_pages) const;

            Result CheckMemoryStateContiguous(size_t *out_blocks_needed, KProcessAddress addr, size_t size, u32 state_mask, u32 state, u32 perm_mask, u32 perm, u32 attr_mask, u32 attr) const;
            Result CheckMemoryStateContiguous(KProcessAddress addr, size_t size, u32 state_mask, u32 state, u32 perm_mask, u32 perm, u32 attr_mask, u32 attr) const {
                R_RETURN(this->CheckMemoryStateContiguous(nullptr, addr, size, state_mask, state, perm_mask, perm, attr_mask, attr));
            }

            Result CheckMemoryState(KMemoryBlockManager::const_iterator it, u32 state_mask, u32 state, u32 perm_mask, u32 perm, u32 attr_mask, u32 attr) const;
            Result CheckMemoryState(KMemoryState *out_state, KMemoryPermission *out_perm, KMemoryAttribute *out_attr, size_t *out_blocks_needed, KMemoryBlockManager::const_iterator it, KProcessAddress last_addr, u32 state_mask, u32 state, u32 perm_mask, u32 perm, u32 attr_mask, u32 attr, u32 ignore_attr = DefaultMemoryIgnoreAttr) const;
            Result CheckMemoryState(KMemoryState *out_state, KMemoryPermission *out_perm, KMemoryAttribute *out_attr, size_t *out_blocks_needed, KProcessAddress addr, size_t size, u32 state_mask, u32 state, u32 perm_mask, u32 perm, u32 attr_mask, u32 attr, u32 ignore_attr = DefaultMemoryIgnoreAttr) const;
            Result CheckMemoryState(size_t *out_blocks_needed, KProcessAddress addr, size_t size, u32 state_mask, u32 state, u32 perm_mask, u32 perm, u32 attr_mask, u32 attr, u32 ignore_attr = DefaultMemoryIgnoreAttr) const {
                R_RETURN(this->CheckMemoryState(nullptr, nullptr, nullptr, out_blocks_needed, addr, size, state_mask, state, perm_mask, perm, attr_mask, attr, ignore_attr));
            }
            Result CheckMemoryState(KProcessAddress addr, size_t size, u32 state_mask, u32 state, u32 perm_mask, u32 perm, u32 attr_mask, u32 attr, u32 ignore_attr = DefaultMemoryIgnoreAttr) const {
                R_RETURN(this->CheckMemoryState(nullptr, addr, size, state_mask, state, perm_mask, perm, attr_mask, attr, ignore_attr));
            }

            bool CanReadWriteDebugMemory(KProcessAddress addr, size_t size, bool force_debug_prod);

            Result LockMemoryAndOpen(KPageGroup *out_pg, KPhysicalAddress *out_paddr, KProcessAddress addr, size_t size, u32 state_mask, u32 state, u32 perm_mask, u32 perm, u32 attr_mask, u32 attr, KMemoryPermission new_perm, u32 lock_attr);
            Result UnlockMemory(KProcessAddress addr, size_t size, u32 state_mask, u32 state, u32 perm_mask, u32 perm, u32 attr_mask, u32 attr, KMemoryPermission new_perm, u32 lock_attr, const KPageGroup *pg);

            Result QueryInfoImpl(KMemoryInfo *out_info, ams::svc::PageInfo *out_page, KProcessAddress address) const;

            Result QueryMappingImpl(KProcessAddress *out, KPhysicalAddress address, size_t size, ams::svc::MemoryState state) const;

            Result AllocateAndMapPagesImpl(PageLinkedList *page_list, KProcessAddress address, size_t num_pages, const KPageProperties &properties);
            Result MapPageGroupImpl(PageLinkedList *page_list, KProcessAddress address, const KPageGroup &pg, const KPageProperties properties, bool reuse_ll);

            void RemapPageGroup(PageLinkedList *page_list, KProcessAddress address, size_t size, const KPageGroup &pg);

            Result MakePageGroup(KPageGroup &pg, KProcessAddress addr, size_t num_pages);
            bool IsValidPageGroup(const KPageGroup &pg, KProcessAddress addr, size_t num_pages);

            Result GetContiguousMemoryRangeWithState(MemoryRange *out, KProcessAddress address, size_t size, u32 state_mask, u32 state, u32 perm_mask, u32 perm, u32 attr_mask, u32 attr);

            NOINLINE Result MapPages(KProcessAddress *out_addr, size_t num_pages, size_t alignment, KPhysicalAddress phys_addr, bool is_pa_valid, KProcessAddress region_start, size_t region_num_pages, KMemoryState state, KMemoryPermission perm);

            Result MapIoImpl(KProcessAddress *out, PageLinkedList *page_list, KPhysicalAddress phys_addr, size_t size, KMemoryState state, KMemoryPermission perm);
            Result ReadIoMemoryImpl(void *buffer, KPhysicalAddress phys_addr, size_t size, KMemoryState state);
            Result WriteIoMemoryImpl(KPhysicalAddress phys_addr, const void *buffer, size_t size, KMemoryState state);

            Result SetupForIpcClient(PageLinkedList *page_list, size_t *out_blocks_needed, KProcessAddress address, size_t size, KMemoryPermission test_perm, KMemoryState dst_state);
            Result SetupForIpcServer(KProcessAddress *out_addr, size_t size, KProcessAddress src_addr, KMemoryPermission test_perm, KMemoryState dst_state, KPageTableBase &src_page_table, bool send);
            void CleanupForIpcClientOnServerSetupFailure(PageLinkedList *page_list, KProcessAddress address, size_t size, KMemoryPermission prot_perm);

            size_t GetSize(KMemoryState state) const;

            ALWAYS_INLINE bool GetPhysicalAddressLocked(KPhysicalAddress *out, KProcessAddress virt_addr) const {
                /* Validate pre-conditions. */
                MESOSPHERE_AUDIT(this->IsLockedByCurrentThread());

                return this->GetImpl().GetPhysicalAddress(out, virt_addr);
            }
        public:
            bool GetPhysicalAddress(KPhysicalAddress *out, KProcessAddress virt_addr) const {
                /* Validate pre-conditions. */
                MESOSPHERE_AUDIT(!this->IsLockedByCurrentThread());

                /* Acquire exclusive access to the table while doing address translation. */
                KScopedLightLock lk(m_general_lock);

                return this->GetPhysicalAddressLocked(out, virt_addr);
            }

            KBlockInfoManager *GetBlockInfoManager() const { return m_block_info_manager; }

            Result SetMemoryPermission(KProcessAddress addr, size_t size, ams::svc::MemoryPermission perm);
            Result SetProcessMemoryPermission(KProcessAddress addr, size_t size, ams::svc::MemoryPermission perm);
            Result SetMemoryAttribute(KProcessAddress addr, size_t size, u32 mask, u32 attr);
            Result SetHeapSize(KProcessAddress *out, size_t size);
            Result SetMaxHeapSize(size_t size);
            Result QueryInfo(KMemoryInfo *out_info, ams::svc::PageInfo *out_page_info, KProcessAddress addr) const;
            Result QueryPhysicalAddress(ams::svc::PhysicalMemoryInfo *out, KProcessAddress address) const;
            Result QueryStaticMapping(KProcessAddress *out, KPhysicalAddress address, size_t size) const { R_RETURN(this->QueryMappingImpl(out, address, size, ams::svc::MemoryState_Static)); }
            Result QueryIoMapping(KProcessAddress *out, KPhysicalAddress address, size_t size) const { R_RETURN(this->QueryMappingImpl(out, address, size, ams::svc::MemoryState_Io)); }
            Result MapMemory(KProcessAddress dst_address, KProcessAddress src_address, size_t size);
            Result UnmapMemory(KProcessAddress dst_address, KProcessAddress src_address, size_t size);
            Result MapCodeMemory(KProcessAddress dst_address, KProcessAddress src_address, size_t size);
            Result UnmapCodeMemory(KProcessAddress dst_address, KProcessAddress src_address, size_t size);
            Result MapIo(KPhysicalAddress phys_addr, size_t size, KMemoryPermission perm);
            Result MapIoRegion(KProcessAddress dst_address, KPhysicalAddress phys_addr, size_t size, ams::svc::MemoryMapping mapping, ams::svc::MemoryPermission perm);
            Result UnmapIoRegion(KProcessAddress dst_address, KPhysicalAddress phys_addr, size_t size, ams::svc::MemoryMapping mapping);
            Result MapStatic(KPhysicalAddress phys_addr, size_t size, KMemoryPermission perm);
            Result MapRegion(KMemoryRegionType region_type, KMemoryPermission perm);
            Result MapInsecurePhysicalMemory(KProcessAddress address, size_t size);
            Result UnmapInsecurePhysicalMemory(KProcessAddress address, size_t size);

            Result MapPages(KProcessAddress *out_addr, size_t num_pages, size_t alignment, KPhysicalAddress phys_addr, KProcessAddress region_start, size_t region_num_pages, KMemoryState state, KMemoryPermission perm) {
                R_RETURN(this->MapPages(out_addr, num_pages, alignment, phys_addr, true, region_start, region_num_pages, state, perm));
            }

            Result MapPages(KProcessAddress *out_addr, size_t num_pages, size_t alignment, KPhysicalAddress phys_addr, KMemoryState state, KMemoryPermission perm) {
                R_RETURN(this->MapPages(out_addr, num_pages, alignment, phys_addr, true, this->GetRegionAddress(state), this->GetRegionSize(state) / PageSize, state, perm));
            }

            Result MapPages(KProcessAddress *out_addr, size_t num_pages, KMemoryState state, KMemoryPermission perm) {
                R_RETURN(this->MapPages(out_addr, num_pages, PageSize, Null<KPhysicalAddress>, false, this->GetRegionAddress(state), this->GetRegionSize(state) / PageSize, state, perm));
            }

            Result MapPages(KProcessAddress address, size_t num_pages, KMemoryState state, KMemoryPermission perm);
            Result UnmapPages(KProcessAddress address, size_t num_pages, KMemoryState state);

            Result MapPageGroup(KProcessAddress *out_addr, const KPageGroup &pg, KProcessAddress region_start, size_t region_num_pages, KMemoryState state, KMemoryPermission perm);
            Result MapPageGroup(KProcessAddress address, const KPageGroup &pg, KMemoryState state, KMemoryPermission perm);
            Result UnmapPageGroup(KProcessAddress address, const KPageGroup &pg, KMemoryState state);

            Result MakeAndOpenPageGroup(KPageGroup *out, KProcessAddress address, size_t num_pages, u32 state_mask, u32 state, u32 perm_mask, u32 perm, u32 attr_mask, u32 attr);

            Result InvalidateProcessDataCache(KProcessAddress address, size_t size);
            Result InvalidateCurrentProcessDataCache(KProcessAddress address, size_t size);

            Result ReadDebugMemory(void *buffer, KProcessAddress address, size_t size, bool force_debug_prod);
            Result ReadDebugIoMemory(void *buffer, KProcessAddress address, size_t size, KMemoryState state);

            Result WriteDebugMemory(KProcessAddress address, const void *buffer, size_t size);
            Result WriteDebugIoMemory(KProcessAddress address, const void *buffer, size_t size, KMemoryState state);

            Result LockForMapDeviceAddressSpace(bool *out_is_io, KProcessAddress address, size_t size, KMemoryPermission perm, bool is_aligned, bool check_heap);
            Result LockForUnmapDeviceAddressSpace(KProcessAddress address, size_t size, bool check_heap);

            Result UnlockForDeviceAddressSpace(KProcessAddress address, size_t size);
            Result UnlockForDeviceAddressSpacePartialMap(KProcessAddress address, size_t size);

            Result OpenMemoryRangeForMapDeviceAddressSpace(KPageTableBase::MemoryRange *out, KProcessAddress address, size_t size, KMemoryPermission perm, bool is_aligned);
            Result OpenMemoryRangeForUnmapDeviceAddressSpace(MemoryRange *out, KProcessAddress address, size_t size);

            Result LockForIpcUserBuffer(KPhysicalAddress *out, KProcessAddress address, size_t size);
            Result UnlockForIpcUserBuffer(KProcessAddress address, size_t size);

            Result LockForTransferMemory(KPageGroup *out, KProcessAddress address, size_t size, KMemoryPermission perm);
            Result UnlockForTransferMemory(KProcessAddress address, size_t size, const KPageGroup &pg);
            Result LockForCodeMemory(KPageGroup *out, KProcessAddress address, size_t size);
            Result UnlockForCodeMemory(KProcessAddress address, size_t size, const KPageGroup &pg);

            Result OpenMemoryRangeForProcessCacheOperation(MemoryRange *out, KProcessAddress address, size_t size);

            Result CopyMemoryFromLinearToUser(KProcessAddress dst_addr, size_t size, KProcessAddress src_addr, u32 src_state_mask, u32 src_state, KMemoryPermission src_test_perm, u32 src_attr_mask, u32 src_attr);
            Result CopyMemoryFromLinearToKernel(KProcessAddress dst_addr, size_t size, KProcessAddress src_addr, u32 src_state_mask, u32 src_state, KMemoryPermission src_test_perm, u32 src_attr_mask, u32 src_attr);
            Result CopyMemoryFromUserToLinear(KProcessAddress dst_addr, size_t size, u32 dst_state_mask, u32 dst_state, KMemoryPermission dst_test_perm, u32 dst_attr_mask, u32 dst_attr, KProcessAddress src_addr);
            Result CopyMemoryFromKernelToLinear(KProcessAddress dst_addr, size_t size, u32 dst_state_mask, u32 dst_state, KMemoryPermission dst_test_perm, u32 dst_attr_mask, u32 dst_attr, KProcessAddress src_addr);
            Result CopyMemoryFromHeapToHeap(KPageTableBase &dst_page_table, KProcessAddress dst_addr, size_t size, u32 dst_state_mask, u32 dst_state, KMemoryPermission dst_test_perm, u32 dst_attr_mask, u32 dst_attr, KProcessAddress src_addr, u32 src_state_mask, u32 src_state, KMemoryPermission src_test_perm, u32 src_attr_mask, u32 src_attr);
            Result CopyMemoryFromHeapToHeapWithoutCheckDestination(KPageTableBase &dst_page_table, KProcessAddress dst_addr, size_t size, u32 dst_state_mask, u32 dst_state, KMemoryPermission dst_test_perm, u32 dst_attr_mask, u32 dst_attr, KProcessAddress src_addr, u32 src_state_mask, u32 src_state, KMemoryPermission src_test_perm, u32 src_attr_mask, u32 src_attr);

            Result SetupForIpc(KProcessAddress *out_dst_addr, size_t size, KProcessAddress src_addr, KPageTableBase &src_page_table, KMemoryPermission test_perm, KMemoryState dst_state, bool send);
            Result CleanupForIpcServer(KProcessAddress address, size_t size, KMemoryState dst_state);
            Result CleanupForIpcClient(KProcessAddress address, size_t size, KMemoryState dst_state);

            Result MapPhysicalMemory(KProcessAddress address, size_t size);
            Result UnmapPhysicalMemory(KProcessAddress address, size_t size);

            Result MapPhysicalMemoryUnsafe(KProcessAddress address, size_t size);
            Result UnmapPhysicalMemoryUnsafe(KProcessAddress address, size_t size);

            Result UnmapProcessMemory(KProcessAddress dst_address, size_t size, KPageTableBase &src_pt, KProcessAddress src_address);

            void DumpMemoryBlocksLocked() const {
                MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());
                m_memory_block_manager.DumpBlocks();
            }

            void DumpMemoryBlocks() const {
                KScopedLightLock lk(m_general_lock);
                this->DumpMemoryBlocksLocked();
            }

            void DumpPageTable() const {
                KScopedLightLock lk(m_general_lock);
                this->GetImpl().Dump(GetInteger(m_address_space_start), m_address_space_end - m_address_space_start);
            }

            size_t CountPageTables() const {
                KScopedLightLock lk(m_general_lock);
                return this->GetImpl().CountPageTables();
            }
        public:
            KProcessAddress GetAddressSpaceStart()    const { return m_address_space_start; }

            KProcessAddress GetHeapRegionStart()      const { return m_region_starts[RegionType_Heap]; }
            KProcessAddress GetAliasRegionStart()     const { return m_region_starts[RegionType_Alias]; }
            KProcessAddress GetStackRegionStart()     const { return m_region_starts[RegionType_Stack]; }
            KProcessAddress GetKernelMapRegionStart() const { return m_region_starts[RegionType_KernelMap]; }

            KProcessAddress GetAliasCodeRegionStart() const { return m_alias_code_region_start; }

            size_t GetAddressSpaceSize()    const { return m_address_space_end - m_address_space_start; }

            size_t GetHeapRegionSize()      const { return m_region_ends[RegionType_Heap]      - m_region_starts[RegionType_Heap]; }
            size_t GetAliasRegionSize()     const { return m_region_ends[RegionType_Alias]     - m_region_starts[RegionType_Alias]; }
            size_t GetStackRegionSize()     const { return m_region_ends[RegionType_Stack]     - m_region_starts[RegionType_Stack]; }
            size_t GetKernelMapRegionSize() const { return m_region_ends[RegionType_KernelMap] - m_region_starts[RegionType_KernelMap]; }

            size_t GetAliasCodeRegionSize() const { return m_alias_code_region_end - m_alias_code_region_start; }

            size_t GetAliasRegionExtraSize() const { return m_alias_region_extra_size; }

            size_t GetNormalMemorySize() const {
                /* Lock the table. */
                KScopedLightLock lk(m_general_lock);

                return (m_current_heap_end - m_region_starts[RegionType_Heap]) + m_mapped_physical_memory_size;
            }

            size_t GetCodeSize() const;
            size_t GetCodeDataSize() const;
            size_t GetAliasCodeSize() const;
            size_t GetAliasCodeDataSize() const;

            u32 GetAllocateOption() const { return m_allocate_option; }
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
