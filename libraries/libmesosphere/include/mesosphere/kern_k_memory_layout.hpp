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
#include <mesosphere/init/kern_init_page_table_select.hpp>

#if defined(ATMOSPHERE_BOARD_NINTENDO_NX)
    #include <mesosphere/board/nintendo/nx/kern_k_memory_layout.board.nintendo_nx.hpp>
#else
    #error "Unknown board for KMemoryLayout"
#endif

namespace ams::kern {

    constexpr size_t KernelAslrAlignment = 2_MB;
    constexpr size_t KernelVirtualAddressSpaceWidth  = size_t(1ul) << 39ul;
    constexpr size_t KernelPhysicalAddressSpaceWidth = size_t(1ul) << 48ul;

    constexpr size_t KernelVirtualAddressSpaceBase  = 0ul - KernelVirtualAddressSpaceWidth;
    constexpr size_t KernelVirtualAddressSpaceEnd   = KernelVirtualAddressSpaceBase + (KernelVirtualAddressSpaceWidth - KernelAslrAlignment);
    constexpr size_t KernelVirtualAddressSpaceLast  = KernelVirtualAddressSpaceEnd - 1ul;
    constexpr size_t KernelVirtualAddressSpaceSize  = KernelVirtualAddressSpaceEnd - KernelVirtualAddressSpaceBase;

    constexpr size_t KernelPhysicalAddressSpaceBase = 0ul;
    constexpr size_t KernelPhysicalAddressSpaceEnd  = KernelPhysicalAddressSpaceBase + KernelPhysicalAddressSpaceWidth;
    constexpr size_t KernelPhysicalAddressSpaceLast = KernelPhysicalAddressSpaceEnd - 1ul;
    constexpr size_t KernelPhysicalAddressSpaceSize = KernelPhysicalAddressSpaceEnd - KernelPhysicalAddressSpaceBase;

    constexpr size_t KernelPageTableHeapSize    = init::KInitialPageTable::GetMaximumOverheadSize(8_GB);
    constexpr size_t KernelInitialPageHeapSize  = 128_KB;

    constexpr size_t KernelSlabHeapDataSize           = 5_MB;
    constexpr size_t KernelSlabHeapGapsSize           = 2_MB - 64_KB;
    constexpr size_t KernelSlabHeapGapsSizeDeprecated = 2_MB;
    constexpr size_t KernelSlabHeapSize               = KernelSlabHeapDataSize + KernelSlabHeapGapsSize;

    /* NOTE: This is calculated from KThread slab counts, assuming KThread size <= 0x860. */
    constexpr size_t KernelSlabHeapAdditionalSize     = 0x68000;

    constexpr size_t KernelResourceSize               = KernelPageTableHeapSize + KernelInitialPageHeapSize + KernelSlabHeapSize;

    enum KMemoryRegionType : u32 {
        KMemoryRegionAttr_CarveoutProtected = 0x04000000,
        KMemoryRegionAttr_DidKernelMap      = 0x08000000,
        KMemoryRegionAttr_ShouldKernelMap   = 0x10000000,
        KMemoryRegionAttr_UserReadOnly      = 0x20000000,
        KMemoryRegionAttr_NoUserMap         = 0x40000000,
        KMemoryRegionAttr_LinearMapped      = 0x80000000,

        KMemoryRegionType_None        = 0,
        KMemoryRegionType_Kernel      = 1,
        KMemoryRegionType_Dram        = 2,
        KMemoryRegionType_CoreLocal   = 4,

        KMemoryRegionType_VirtualKernelPtHeap       = 0x2A,
        KMemoryRegionType_VirtualKernelTraceBuffer  = 0x4A,
        KMemoryRegionType_VirtualKernelInitPt       = 0x19A,

        KMemoryRegionType_VirtualDramMetadataPool        = 0x29A,
        KMemoryRegionType_VirtualDramManagedPool         = 0x31A,
        KMemoryRegionType_VirtualDramApplicationPool     = 0x271A,
        KMemoryRegionType_VirtualDramAppletPool          = 0x1B1A,
        KMemoryRegionType_VirtualDramSystemPool          = 0x2B1A,
        KMemoryRegionType_VirtualDramSystemNonSecurePool = 0x331A,

        KMemoryRegionType_Uart                      = 0x1D,
        KMemoryRegionType_InterruptDistributor      = 0x4D | KMemoryRegionAttr_NoUserMap,
        KMemoryRegionType_InterruptCpuInterface     = 0x2D | KMemoryRegionAttr_NoUserMap,

        KMemoryRegionType_MemoryController          = 0x55,
        KMemoryRegionType_MemoryController0         = 0x95,
        KMemoryRegionType_MemoryController1         = 0x65,
        KMemoryRegionType_PowerManagementController = 0x1A5,

        KMemoryRegionType_KernelAutoMap = KMemoryRegionType_Kernel | KMemoryRegionAttr_ShouldKernelMap,

        KMemoryRegionType_KernelTemp   = 0x31,

        KMemoryRegionType_KernelCode   = 0x19,
        KMemoryRegionType_KernelStack  = 0x29,
        KMemoryRegionType_KernelMisc   = 0x49,
        KMemoryRegionType_KernelSlab   = 0x89,

        KMemoryRegionType_KernelMiscMainStack      = 0xB49,
        KMemoryRegionType_KernelMiscMappedDevice   = 0xD49,
        KMemoryRegionType_KernelMiscIdleStack      = 0x1349,
        KMemoryRegionType_KernelMiscUnknownDebug   = 0x1549,
        KMemoryRegionType_KernelMiscExceptionStack = 0x2349,

        KMemoryRegionType_DramLinearMapped  = KMemoryRegionType_Dram  | KMemoryRegionAttr_LinearMapped,

        KMemoryRegionType_DramReservedEarly       = 0x16   | KMemoryRegionAttr_NoUserMap,
        KMemoryRegionType_DramPoolPartition       = 0x26   | KMemoryRegionAttr_NoUserMap | KMemoryRegionAttr_LinearMapped,
        KMemoryRegionType_DramMetadataPool        = 0x166  | KMemoryRegionAttr_NoUserMap | KMemoryRegionAttr_LinearMapped | KMemoryRegionAttr_CarveoutProtected,

        KMemoryRegionType_DramNonKernel           = 0x1A6  | KMemoryRegionAttr_NoUserMap | KMemoryRegionAttr_LinearMapped,

        KMemoryRegionType_DramApplicationPool     = 0x7A6  | KMemoryRegionAttr_NoUserMap | KMemoryRegionAttr_LinearMapped,
        KMemoryRegionType_DramAppletPool          = 0xBA6  | KMemoryRegionAttr_NoUserMap | KMemoryRegionAttr_LinearMapped,
        KMemoryRegionType_DramSystemNonSecurePool = 0xDA6  | KMemoryRegionAttr_NoUserMap | KMemoryRegionAttr_LinearMapped,
        KMemoryRegionType_DramSystemPool          = 0x13A6 | KMemoryRegionAttr_NoUserMap | KMemoryRegionAttr_LinearMapped | KMemoryRegionAttr_CarveoutProtected,

        KMemoryRegionType_DramKernel        = 0xE   | KMemoryRegionAttr_NoUserMap | KMemoryRegionAttr_CarveoutProtected,
        KMemoryRegionType_DramKernelCode    = 0xCE  | KMemoryRegionAttr_NoUserMap | KMemoryRegionAttr_CarveoutProtected,
        KMemoryRegionType_DramKernelSlab    = 0x14E | KMemoryRegionAttr_NoUserMap | KMemoryRegionAttr_CarveoutProtected,
        KMemoryRegionType_DramKernelPtHeap  = 0x24E | KMemoryRegionAttr_NoUserMap | KMemoryRegionAttr_CarveoutProtected | KMemoryRegionAttr_LinearMapped,
        KMemoryRegionType_DramKernelInitPt  = 0x44E | KMemoryRegionAttr_NoUserMap | KMemoryRegionAttr_CarveoutProtected | KMemoryRegionAttr_LinearMapped,

        /* These regions aren't normally mapped in retail kernel. */
        KMemoryRegionType_KernelTraceBuffer = 0xA6  | KMemoryRegionAttr_UserReadOnly | KMemoryRegionAttr_LinearMapped,
        KMemoryRegionType_OnMemoryBootImage = 0x156,
        KMemoryRegionType_DTB               = 0x256,
    };

    constexpr ALWAYS_INLINE KMemoryRegionType GetTypeForVirtualLinearMapping(u32 type_id) {
        if (type_id == (type_id | KMemoryRegionType_KernelTraceBuffer)) {
            return KMemoryRegionType_VirtualKernelTraceBuffer;
        } else if (type_id == (type_id | KMemoryRegionType_DramKernelPtHeap)) {
            return KMemoryRegionType_VirtualKernelPtHeap;
        } else {
            return KMemoryRegionType_Dram;
        }
    }

    class KMemoryRegionTree;

    class KMemoryRegion : public util::IntrusiveRedBlackTreeBaseNode<KMemoryRegion> {
        NON_COPYABLE(KMemoryRegion);
        NON_MOVEABLE(KMemoryRegion);
        private:
            friend class KMemoryRegionTree;
        private:
            uintptr_t address;
            uintptr_t pair_address;
            size_t region_size;
            u32 attributes;
            u32 type_id;
        public:
            static constexpr ALWAYS_INLINE int Compare(const KMemoryRegion &lhs, const KMemoryRegion &rhs) {
                if (lhs.GetAddress() < rhs.GetAddress()) {
                    return -1;
                } else if (lhs.GetAddress() <= rhs.GetLastAddress()) {
                    return 0;
                } else {
                    return 1;
                }
            }
        public:
            constexpr ALWAYS_INLINE KMemoryRegion() : address(0), pair_address(0), region_size(0), attributes(0), type_id(0) { /* ... */ }
            constexpr ALWAYS_INLINE KMemoryRegion(uintptr_t a, size_t rs, uintptr_t p, u32 r, u32 t) :
                address(a), pair_address(p), region_size(rs), attributes(r), type_id(t)
            {
                /* ... */
            }
            constexpr ALWAYS_INLINE KMemoryRegion(uintptr_t a, size_t rs, u32 r, u32 t) : KMemoryRegion(a, rs, std::numeric_limits<uintptr_t>::max(), r, t) { /* ... */ }
        private:
            constexpr ALWAYS_INLINE void Reset(uintptr_t a, uintptr_t rs, uintptr_t p, u32 r, u32 t) {
                this->address      = a;
                this->pair_address = p;
                this->region_size  = rs;
                this->attributes   = r;
                this->type_id      = t;
            }
        public:
            constexpr ALWAYS_INLINE uintptr_t GetAddress() const {
                return this->address;
            }

            constexpr ALWAYS_INLINE uintptr_t GetPairAddress() const {
                return this->pair_address;
            }

            constexpr ALWAYS_INLINE size_t GetSize() const {
                return this->region_size;
            }

            constexpr ALWAYS_INLINE uintptr_t GetEndAddress() const {
                return this->GetAddress() + this->GetSize();
            }

            constexpr ALWAYS_INLINE uintptr_t GetLastAddress() const {
                return this->GetEndAddress() - 1;
            }

            constexpr ALWAYS_INLINE u32 GetAttributes() const {
                return this->attributes;
            }

            constexpr ALWAYS_INLINE u32 GetType() const {
                return this->type_id;
            }

            constexpr ALWAYS_INLINE void SetType(u32 type) {
                MESOSPHERE_INIT_ABORT_UNLESS(this->CanDerive(type));
                this->type_id = type;
            }

            constexpr ALWAYS_INLINE bool Contains(uintptr_t address) const {
                return this->GetAddress() <= address && address <= this->GetLastAddress();
            }

            constexpr ALWAYS_INLINE bool IsDerivedFrom(u32 type) const {
                return (this->GetType() | type) == this->GetType();
            }

            constexpr ALWAYS_INLINE bool HasTypeAttribute(KMemoryRegionType attr) const {
                return (this->GetType() | attr) == this->GetType();
            }

            constexpr ALWAYS_INLINE bool CanDerive(u32 type) const {
                return (this->GetType() | type) == type;
            }

            constexpr ALWAYS_INLINE void SetPairAddress(uintptr_t a) {
                this->pair_address = a;
            }

            constexpr ALWAYS_INLINE void SetTypeAttribute(KMemoryRegionType attr) {
                this->type_id |= attr;
            }
    };
    static_assert(std::is_trivially_destructible<KMemoryRegion>::value);

    class KMemoryRegionTree {
        public:
            struct DerivedRegionExtents {
                const KMemoryRegion *first_region;
                const KMemoryRegion *last_region;

                constexpr DerivedRegionExtents() : first_region(nullptr), last_region(nullptr) { /* ... */ }

                constexpr ALWAYS_INLINE uintptr_t GetAddress() const {
                    return this->first_region->GetAddress();
                }

                constexpr ALWAYS_INLINE uintptr_t GetEndAddress() const {
                    return this->last_region->GetEndAddress();
                }

                constexpr ALWAYS_INLINE size_t GetSize() const {
                    return this->GetEndAddress() - this->GetAddress();
                }

                constexpr ALWAYS_INLINE uintptr_t GetLastAddress() const {
                    return this->GetEndAddress() - 1;
                }
            };
        private:
            using TreeType = util::IntrusiveRedBlackTreeBaseTraits<KMemoryRegion>::TreeType<KMemoryRegion>;
        public:
            using value_type        = TreeType::value_type;
            using size_type         = TreeType::size_type;
            using difference_type   = TreeType::difference_type;
            using pointer           = TreeType::pointer;
            using const_pointer     = TreeType::const_pointer;
            using reference         = TreeType::reference;
            using const_reference   = TreeType::const_reference;
            using iterator          = TreeType::iterator;
            using const_iterator    = TreeType::const_iterator;
        private:
            TreeType tree;
        public:
            constexpr ALWAYS_INLINE KMemoryRegionTree() : tree() { /* ... */ }
        public:
            KMemoryRegion *FindModifiable(uintptr_t address) {
                if (auto it = this->find(KMemoryRegion(address, 1, 0, 0)); it != this->end()) {
                    return std::addressof(*it);
                } else {
                    return nullptr;
                }
            }

            const KMemoryRegion *Find(uintptr_t address) const {
                if (auto it = this->find(KMemoryRegion(address, 1, 0, 0)); it != this->cend()) {
                    return std::addressof(*it);
                } else {
                    return nullptr;
                }
            }

            const KMemoryRegion *FindByType(u32 type_id) const {
                for (auto it = this->cbegin(); it != this->cend(); ++it) {
                    if (it->GetType() == type_id) {
                        return std::addressof(*it);
                    }
                }
                return nullptr;
            }

            const KMemoryRegion *FindByTypeAndAttribute(u32 type_id, u32 attr) const {
                for (auto it = this->cbegin(); it != this->cend(); ++it) {
                    if (it->GetType() == type_id && it->GetAttributes() == attr) {
                        return std::addressof(*it);
                    }
                }
                return nullptr;
            }

            const KMemoryRegion *FindFirstDerived(u32 type_id) const {
                for (auto it = this->cbegin(); it != this->cend(); it++) {
                    if (it->IsDerivedFrom(type_id)) {
                        return std::addressof(*it);
                    }
                }
                return nullptr;
            }

            const KMemoryRegion *FindLastDerived(u32 type_id) const {
                const KMemoryRegion *region = nullptr;
                for (auto it = this->begin(); it != this->end(); it++) {
                    if (it->IsDerivedFrom(type_id)) {
                        region = std::addressof(*it);
                    }
                }
                return region;
            }


            DerivedRegionExtents GetDerivedRegionExtents(u32 type_id) const {
                DerivedRegionExtents extents;

                MESOSPHERE_INIT_ABORT_UNLESS(extents.first_region == nullptr);
                MESOSPHERE_INIT_ABORT_UNLESS(extents.last_region  == nullptr);

                for (auto it = this->cbegin(); it != this->cend(); it++) {
                    if (it->IsDerivedFrom(type_id)) {
                        if (extents.first_region == nullptr) {
                            extents.first_region = std::addressof(*it);
                        }
                        extents.last_region = std::addressof(*it);
                    }
                }

                MESOSPHERE_INIT_ABORT_UNLESS(extents.first_region != nullptr);
                MESOSPHERE_INIT_ABORT_UNLESS(extents.last_region  != nullptr);

                return extents;
            }
        public:
            NOINLINE void InsertDirectly(uintptr_t address, size_t size, u32 attr = 0, u32 type_id = 0);
            NOINLINE bool Insert(uintptr_t address, size_t size, u32 type_id, u32 new_attr = 0, u32 old_attr = 0);

            NOINLINE KVirtualAddress GetRandomAlignedRegion(size_t size, size_t alignment, u32 type_id);

            ALWAYS_INLINE KVirtualAddress GetRandomAlignedRegionWithGuard(size_t size, size_t alignment, u32 type_id, size_t guard_size) {
                return this->GetRandomAlignedRegion(size + 2 * guard_size, alignment, type_id) + guard_size;
            }
        public:
            /* Iterator accessors. */
            iterator begin() {
                return this->tree.begin();
            }

            const_iterator begin() const {
                return this->tree.begin();
            }

            iterator end() {
                return this->tree.end();
            }

            const_iterator end() const {
                return this->tree.end();
            }

            const_iterator cbegin() const {
                return this->begin();
            }

            const_iterator cend() const {
                return this->end();
            }

            iterator iterator_to(reference ref) {
                return this->tree.iterator_to(ref);
            }

            const_iterator iterator_to(const_reference ref) const {
                return this->tree.iterator_to(ref);
            }

            /* Content management. */
            bool empty() const {
                return this->tree.empty();
            }

            reference back() {
                return this->tree.back();
            }

            const_reference back() const {
                return this->tree.back();
            }

            reference front() {
                return this->tree.front();
            }

            const_reference front() const {
                return this->tree.front();
            }

            /* GCC over-eagerly inlines this operation. */
            NOINLINE iterator insert(reference ref) {
                return this->tree.insert(ref);
            }

            NOINLINE iterator erase(iterator it) {
                return this->tree.erase(it);
            }

            iterator find(const_reference ref) const {
                return this->tree.find(ref);
            }

            iterator nfind(const_reference ref) const {
                return this->tree.nfind(ref);
            }
    };

    class KMemoryLayout {
        private:
            static /* constinit */ inline uintptr_t s_linear_phys_to_virt_diff;
            static /* constinit */ inline uintptr_t s_linear_virt_to_phys_diff;
            static /* constinit */ inline KMemoryRegionTree s_virtual_tree;
            static /* constinit */ inline KMemoryRegionTree s_physical_tree;
            static /* constinit */ inline KMemoryRegionTree s_virtual_linear_tree;
            static /* constinit */ inline KMemoryRegionTree s_physical_linear_tree;
        private:
            template<typename AddressType> requires IsKTypedAddress<AddressType>
            static ALWAYS_INLINE bool IsTypedAddress(const KMemoryRegion *&region, AddressType address, KMemoryRegionTree &tree, KMemoryRegionType type) {
                /* Check if the cached region already contains the address. */
                if (region != nullptr && region->Contains(GetInteger(address))) {
                    return true;
                }

                /* Find the containing region, and update the cache. */
                if (const KMemoryRegion *found = tree.Find(GetInteger(address)); found != nullptr && found->IsDerivedFrom(type)) {
                    region = found;
                    return true;
                } else {
                    return false;
                }
            }

            template<typename AddressType> requires IsKTypedAddress<AddressType>
            static ALWAYS_INLINE bool IsTypedAddress(const KMemoryRegion *&region, AddressType address, size_t size, KMemoryRegionTree &tree, KMemoryRegionType type) {
                /* Get the end of the checked region. */
                const uintptr_t last_address = GetInteger(address) + size - 1;

                /* Walk the tree to verify the region is correct. */
                const KMemoryRegion *cur = (region != nullptr && region->Contains(GetInteger(address))) ? region : tree.Find(GetInteger(address));
                while (cur != nullptr && cur->IsDerivedFrom(type)) {
                    if (last_address <= cur->GetLastAddress()) {
                        region = cur;
                        return true;
                    }

                    cur = cur->GetNext();
                }
                return false;
            }

            template<typename AddressType> requires IsKTypedAddress<AddressType>
            static ALWAYS_INLINE const KMemoryRegion *Find(AddressType address, const KMemoryRegionTree &tree) {
                return tree.Find(GetInteger(address));
            }

            static ALWAYS_INLINE KMemoryRegion &Dereference(KMemoryRegion *region) {
                MESOSPHERE_INIT_ABORT_UNLESS(region != nullptr);
                return *region;
            }

            static ALWAYS_INLINE const KMemoryRegion &Dereference(const KMemoryRegion *region) {
                MESOSPHERE_INIT_ABORT_UNLESS(region != nullptr);
                return *region;
            }

            static ALWAYS_INLINE KVirtualAddress GetStackTopAddress(s32 core_id, KMemoryRegionType type) {
                return Dereference(GetVirtualMemoryRegionTree().FindByTypeAndAttribute(type, static_cast<u32>(core_id))).GetEndAddress();
            }
        public:
            static ALWAYS_INLINE KMemoryRegionTree &GetVirtualMemoryRegionTree()        { return s_virtual_tree; }
            static ALWAYS_INLINE KMemoryRegionTree &GetPhysicalMemoryRegionTree()       { return s_physical_tree; }
            static ALWAYS_INLINE KMemoryRegionTree &GetVirtualLinearMemoryRegionTree()  { return s_virtual_linear_tree; }
            static ALWAYS_INLINE KMemoryRegionTree &GetPhysicalLinearMemoryRegionTree() { return s_physical_linear_tree; }

            static ALWAYS_INLINE KVirtualAddress GetLinearVirtualAddress(KPhysicalAddress address)  { return GetInteger(address) + s_linear_phys_to_virt_diff; }
            static ALWAYS_INLINE KPhysicalAddress GetLinearPhysicalAddress(KVirtualAddress address) { return GetInteger(address) + s_linear_virt_to_phys_diff; }

            static NOINLINE const KMemoryRegion *Find(KVirtualAddress address)  { return Find(address, GetVirtualMemoryRegionTree()); }
            static NOINLINE const KMemoryRegion *Find(KPhysicalAddress address) { return Find(address, GetPhysicalMemoryRegionTree()); }

            static NOINLINE const KMemoryRegion *FindLinear(KVirtualAddress address)  { return Find(address, GetVirtualLinearMemoryRegionTree());  }
            static NOINLINE const KMemoryRegion *FindLinear(KPhysicalAddress address) { return Find(address, GetPhysicalLinearMemoryRegionTree()); }

            static NOINLINE KVirtualAddress GetMainStackTopAddress(s32 core_id)      { return GetStackTopAddress(core_id, KMemoryRegionType_KernelMiscMainStack); }
            static NOINLINE KVirtualAddress GetIdleStackTopAddress(s32 core_id)      { return GetStackTopAddress(core_id, KMemoryRegionType_KernelMiscIdleStack); }
            static NOINLINE KVirtualAddress GetExceptionStackTopAddress(s32 core_id) { return GetStackTopAddress(core_id, KMemoryRegionType_KernelMiscExceptionStack); }

            static NOINLINE KVirtualAddress GetSlabRegionAddress()      { return Dereference(GetVirtualMemoryRegionTree().FindByType(KMemoryRegionType_KernelSlab)).GetAddress(); }
            static NOINLINE KVirtualAddress GetCoreLocalRegionAddress() { return Dereference(GetVirtualMemoryRegionTree().FindByType(KMemoryRegionType_CoreLocal)).GetAddress(); }

            static NOINLINE KVirtualAddress GetInterruptDistributorAddress()  { return Dereference(GetPhysicalMemoryRegionTree().FindFirstDerived(KMemoryRegionType_InterruptDistributor)).GetPairAddress(); }
            static NOINLINE KVirtualAddress GetInterruptCpuInterfaceAddress() { return Dereference(GetPhysicalMemoryRegionTree().FindFirstDerived(KMemoryRegionType_InterruptCpuInterface)).GetPairAddress(); }
            static NOINLINE KVirtualAddress GetUartAddress()                  { return Dereference(GetPhysicalMemoryRegionTree().FindFirstDerived(KMemoryRegionType_Uart)).GetPairAddress(); }

            static NOINLINE const KMemoryRegion &GetMemoryControllerRegion() { return Dereference(GetPhysicalMemoryRegionTree().FindFirstDerived(KMemoryRegionType_MemoryController)); }

            static NOINLINE const KMemoryRegion &GetMetadataPoolRegion()  { return Dereference(GetVirtualMemoryRegionTree().FindByType(KMemoryRegionType_VirtualDramMetadataPool)); }
            static NOINLINE const KMemoryRegion &GetPageTableHeapRegion() { return Dereference(GetVirtualMemoryRegionTree().FindByType(KMemoryRegionType_VirtualKernelPtHeap)); }
            static NOINLINE const KMemoryRegion &GetKernelStackRegion()   { return Dereference(GetVirtualMemoryRegionTree().FindByType(KMemoryRegionType_KernelStack)); }
            static NOINLINE const KMemoryRegion &GetTempRegion()          { return Dereference(GetVirtualMemoryRegionTree().FindByType(KMemoryRegionType_KernelTemp)); }
            static NOINLINE const KMemoryRegion &GetCoreLocalRegion()     { return Dereference(GetVirtualMemoryRegionTree().FindByType(KMemoryRegionType_CoreLocal)); }

            static NOINLINE const KMemoryRegion &GetKernelTraceBufferRegion() { return Dereference(GetVirtualLinearMemoryRegionTree().FindByType(KMemoryRegionType_VirtualKernelTraceBuffer)); }

            static NOINLINE const KMemoryRegion &GetVirtualLinearRegion(KVirtualAddress address) { return Dereference(FindLinear(address)); }

            static NOINLINE const KMemoryRegion *GetPhysicalKernelTraceBufferRegion() { return GetPhysicalMemoryRegionTree().FindFirstDerived(KMemoryRegionType_KernelTraceBuffer); }
            static NOINLINE const KMemoryRegion *GetPhysicalOnMemoryBootImageRegion() { return GetPhysicalMemoryRegionTree().FindFirstDerived(KMemoryRegionType_OnMemoryBootImage); }
            static NOINLINE const KMemoryRegion *GetPhysicalDTBRegion()               { return GetPhysicalMemoryRegionTree().FindFirstDerived(KMemoryRegionType_DTB); }

            static NOINLINE bool IsHeapPhysicalAddress(const KMemoryRegion *&region, KPhysicalAddress address) { return IsTypedAddress(region, address, GetPhysicalLinearMemoryRegionTree(), KMemoryRegionType_DramNonKernel); }
            static NOINLINE bool IsHeapVirtualAddress(const KMemoryRegion *&region, KVirtualAddress address)   { return IsTypedAddress(region, address, GetVirtualLinearMemoryRegionTree(), KMemoryRegionType_VirtualDramManagedPool); }

            static NOINLINE bool IsHeapPhysicalAddress(const KMemoryRegion *&region, KPhysicalAddress address, size_t size) { return IsTypedAddress(region, address, size, GetPhysicalLinearMemoryRegionTree(), KMemoryRegionType_DramNonKernel); }
            static NOINLINE bool IsHeapVirtualAddress(const KMemoryRegion *&region, KVirtualAddress address, size_t size)   { return IsTypedAddress(region, address, size, GetVirtualLinearMemoryRegionTree(), KMemoryRegionType_VirtualDramManagedPool); }

            static NOINLINE bool IsLinearMappedPhysicalAddress(const KMemoryRegion *&region, KPhysicalAddress address)  { return IsTypedAddress(region, address, GetPhysicalLinearMemoryRegionTree(), KMemoryRegionAttr_LinearMapped); }
            static NOINLINE bool IsLinearMappedPhysicalAddress(const KMemoryRegion *&region, KPhysicalAddress address, size_t size) { return IsTypedAddress(region, address, size, GetPhysicalLinearMemoryRegionTree(), KMemoryRegionAttr_LinearMapped); }

            static NOINLINE std::tuple<size_t, size_t> GetTotalAndKernelMemorySizes() {
                size_t total_size = 0, kernel_size = 0;
                for (const auto &region : GetPhysicalMemoryRegionTree()) {
                    if (region.IsDerivedFrom(KMemoryRegionType_Dram)) {
                        total_size += region.GetSize();
                        if (!region.IsDerivedFrom(KMemoryRegionType_DramNonKernel)) {
                            kernel_size += region.GetSize();
                        }
                    }
                }
                return std::make_tuple(total_size, kernel_size);
            }

            static void InitializeLinearMemoryRegionTrees(KPhysicalAddress aligned_linear_phys_start, KVirtualAddress linear_virtual_start);
            static size_t GetResourceRegionSizeForInit();

            static NOINLINE auto GetKernelRegionExtents()      { return GetVirtualMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_Kernel); }
            static NOINLINE auto GetKernelCodeRegionExtents()  { return GetVirtualMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_KernelCode); }
            static NOINLINE auto GetKernelStackRegionExtents() { return GetVirtualMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_KernelStack); }
            static NOINLINE auto GetKernelMiscRegionExtents()  { return GetVirtualMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_KernelMisc); }
            static NOINLINE auto GetKernelSlabRegionExtents()  { return GetVirtualMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_KernelSlab); }


            static NOINLINE auto GetLinearRegionPhysicalExtents() { return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionAttr_LinearMapped); }

            static NOINLINE auto GetLinearRegionVirtualExtents()  {
                auto physical = GetLinearRegionPhysicalExtents();
                return KMemoryRegion(GetInteger(GetLinearVirtualAddress(physical.GetAddress())), physical.GetSize(), 0, KMemoryRegionType_None);
            }

            static NOINLINE auto GetMainMemoryPhysicalExtents() { return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_Dram); }
            static NOINLINE auto GetCarveoutRegionExtents() { return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionAttr_CarveoutProtected); }

            static NOINLINE auto GetKernelRegionPhysicalExtents() { return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_DramKernel); }
            static NOINLINE auto GetKernelCodeRegionPhysicalExtents() { return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_DramKernelCode); }
            static NOINLINE auto GetKernelSlabRegionPhysicalExtents() { return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_DramKernelSlab); }
            static NOINLINE auto GetKernelPageTableHeapRegionPhysicalExtents() { return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_DramKernelPtHeap); }
            static NOINLINE auto GetKernelInitPageTableRegionPhysicalExtents() { return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_DramKernelInitPt); }

            static NOINLINE auto GetKernelPoolPartitionRegionPhysicalExtents() { return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_DramPoolPartition); }
            static NOINLINE auto GetKernelMetadataPoolRegionPhysicalExtents() { return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_DramMetadataPool); }
            static NOINLINE auto GetKernelSystemPoolRegionPhysicalExtents() { return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_DramSystemPool); }
            static NOINLINE auto GetKernelSystemNonSecurePoolRegionPhysicalExtents() { return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_DramSystemNonSecurePool); }
            static NOINLINE auto GetKernelAppletPoolRegionPhysicalExtents() { return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_DramAppletPool); }
            static NOINLINE auto GetKernelApplicationPoolRegionPhysicalExtents() { return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_DramApplicationPool); }

            static NOINLINE auto GetKernelTraceBufferRegionPhysicalExtents() { return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_KernelTraceBuffer); }
    };


    namespace init {

        /* These should be generic, regardless of board. */
        void SetupCoreLocalRegionMemoryRegions(KInitialPageTable &page_table, KInitialPageAllocator &page_allocator);
        void SetupPoolPartitionMemoryRegions();

        /* These may be implemented in a board-specific manner. */
        void SetupDevicePhysicalMemoryRegions();
        void SetupDramPhysicalMemoryRegions();

    }

}
