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

    class KMemoryRegion : public util::IntrusiveRedBlackTreeBaseNode<KMemoryRegion> {
        NON_COPYABLE(KMemoryRegion);
        NON_MOVEABLE(KMemoryRegion);
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
            iterator FindContainingRegion(uintptr_t address) {
                return this->find(KMemoryRegion(address, 1, 0, 0));
            }

            iterator FindFirstRegionByTypeAttr(u32 type_id, u32 attr = 0) {
                for (auto it = this->begin(); it != this->end(); it++) {
                    if (it->GetType() == type_id && it->GetAttributes() == attr) {
                        return it;
                    }
                }
                MESOSPHERE_INIT_ABORT();
            }

            iterator FindFirstRegionByType(u32 type_id) {
                for (auto it = this->begin(); it != this->end(); it++) {
                    if (it->GetType() == type_id) {
                        return it;
                    }
                }
                MESOSPHERE_INIT_ABORT();
            }

            iterator TryFindFirstRegionByType(u32 type_id) {
                for (auto it = this->begin(); it != this->end(); it++) {
                    if (it->GetType() == type_id) {
                        return it;
                    }
                }

                return this->end();
            }

            iterator FindFirstDerivedRegion(u32 type_id) {
                for (auto it = this->begin(); it != this->end(); it++) {
                    if (it->IsDerivedFrom(type_id)) {
                        return it;
                    }
                }
                MESOSPHERE_INIT_ABORT();
            }

            iterator TryFindFirstDerivedRegion(u32 type_id) {
                for (auto it = this->begin(); it != this->end(); it++) {
                    if (it->IsDerivedFrom(type_id)) {
                        return it;
                    }
                }

                return this->end();
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

    class KMemoryRegionAllocator {
        NON_COPYABLE(KMemoryRegionAllocator);
        NON_MOVEABLE(KMemoryRegionAllocator);
        public:
            static constexpr size_t MaxMemoryRegions = 1000;
            friend class KMemoryLayout;
        private:
            KMemoryRegion region_heap[MaxMemoryRegions];
            size_t num_regions;
        private:
            constexpr ALWAYS_INLINE KMemoryRegionAllocator() : region_heap(), num_regions() { /* ... */ }
        public:
            ALWAYS_INLINE KMemoryRegion *Allocate() {
                /* Ensure we stay within the bounds of our heap. */
                MESOSPHERE_INIT_ABORT_UNLESS(this->num_regions < MaxMemoryRegions);

                return &this->region_heap[this->num_regions++];
            }

            template<typename... Args>
            ALWAYS_INLINE KMemoryRegion *Create(Args&&... args) {
                KMemoryRegion *region = this->Allocate();
                new (region) KMemoryRegion(std::forward<Args>(args)...);
                return region;
            }
    };

    class KMemoryLayout {
        private:
            static /* constinit */ inline uintptr_t s_linear_phys_to_virt_diff;
            static /* constinit */ inline uintptr_t s_linear_virt_to_phys_diff;
            static /* constinit */ inline KMemoryRegionAllocator s_region_allocator;
            static /* constinit */ inline KMemoryRegionTree s_virtual_tree;
            static /* constinit */ inline KMemoryRegionTree s_physical_tree;
            static /* constinit */ inline KMemoryRegionTree s_virtual_linear_tree;
            static /* constinit */ inline KMemoryRegionTree s_physical_linear_tree;
        private:
            static ALWAYS_INLINE auto GetVirtualLinearExtents(const KMemoryRegionTree::DerivedRegionExtents physical) {
                return KMemoryRegion(GetInteger(GetLinearVirtualAddress(physical.GetAddress())), physical.GetSize(), 0, KMemoryRegionType_None);
            }
        public:
            static ALWAYS_INLINE KMemoryRegionAllocator &GetMemoryRegionAllocator()        { return s_region_allocator; }
            static ALWAYS_INLINE KMemoryRegionTree      &GetVirtualMemoryRegionTree()      { return s_virtual_tree; }
            static ALWAYS_INLINE KMemoryRegionTree      &GetPhysicalMemoryRegionTree()     { return s_physical_tree; }
            static ALWAYS_INLINE KMemoryRegionTree      &GetVirtualLinearMemoryRegionTree()  { return s_virtual_linear_tree; }
            static ALWAYS_INLINE KMemoryRegionTree      &GetPhysicalLinearMemoryRegionTree() { return s_physical_linear_tree; }

            static ALWAYS_INLINE KMemoryRegionTree::iterator GetEnd(KVirtualAddress) {
                return GetVirtualLinearMemoryRegionTree().end();
            }

            static ALWAYS_INLINE KMemoryRegionTree::iterator GetEnd(KPhysicalAddress) {
                return GetPhysicalMemoryRegionTree().end();
            }

            static NOINLINE KMemoryRegionTree::iterator FindContainingRegion(KVirtualAddress address) {
                return GetVirtualMemoryRegionTree().FindContainingRegion(GetInteger(address));
            }

            static NOINLINE KMemoryRegionTree::iterator FindContainingRegion(KPhysicalAddress address) {
                return GetPhysicalMemoryRegionTree().FindContainingRegion(GetInteger(address));
            }

            static ALWAYS_INLINE KVirtualAddress GetLinearVirtualAddress(KPhysicalAddress address) {
                return GetInteger(address) + s_linear_phys_to_virt_diff;
            }

            static ALWAYS_INLINE KPhysicalAddress GetLinearPhysicalAddress(KVirtualAddress address) {
                return GetInteger(address) + s_linear_virt_to_phys_diff;
            }

            static NOINLINE KVirtualAddress GetMainStackTopAddress(s32 core_id) {
                return GetVirtualMemoryRegionTree().FindFirstRegionByTypeAttr(KMemoryRegionType_KernelMiscMainStack, static_cast<u32>(core_id))->GetEndAddress();
            }

            static NOINLINE KVirtualAddress GetIdleStackTopAddress(s32 core_id) {
                return GetVirtualMemoryRegionTree().FindFirstRegionByTypeAttr(KMemoryRegionType_KernelMiscIdleStack, static_cast<u32>(core_id))->GetEndAddress();
            }

            static NOINLINE KVirtualAddress GetExceptionStackTopAddress(s32 core_id) {
                return GetVirtualMemoryRegionTree().FindFirstRegionByTypeAttr(KMemoryRegionType_KernelMiscExceptionStack, static_cast<u32>(core_id))->GetEndAddress();
            }

            static NOINLINE KVirtualAddress GetSlabRegionAddress() {
                return GetVirtualMemoryRegionTree().FindFirstRegionByType(KMemoryRegionType_KernelSlab)->GetAddress();
            }

            static NOINLINE KVirtualAddress GetCoreLocalRegionAddress() {
                return GetVirtualMemoryRegionTree().FindFirstRegionByType(KMemoryRegionType_CoreLocal)->GetAddress();
            }

            static NOINLINE KVirtualAddress GetInterruptDistributorAddress() {
                return GetPhysicalMemoryRegionTree().FindFirstDerivedRegion(KMemoryRegionType_InterruptDistributor)->GetPairAddress();
            }

            static NOINLINE KVirtualAddress GetInterruptCpuInterfaceAddress() {
                return GetPhysicalMemoryRegionTree().FindFirstDerivedRegion(KMemoryRegionType_InterruptCpuInterface)->GetPairAddress();
            }

            static NOINLINE KVirtualAddress GetUartAddress() {
                return GetPhysicalMemoryRegionTree().FindFirstDerivedRegion(KMemoryRegionType_Uart)->GetPairAddress();
            }

            static NOINLINE KMemoryRegion &GetMemoryControllerRegion() {
                return *GetPhysicalMemoryRegionTree().FindFirstDerivedRegion(KMemoryRegionType_MemoryController);
            }

            static NOINLINE KMemoryRegion &GetMetadataPoolRegion() {
                return *GetVirtualMemoryRegionTree().FindFirstRegionByType(KMemoryRegionType_VirtualDramMetadataPool);
            }

            static NOINLINE KMemoryRegion &GetPageTableHeapRegion() {
                return *GetVirtualMemoryRegionTree().FindFirstRegionByType(KMemoryRegionType_VirtualKernelPtHeap);
            }

            static NOINLINE KMemoryRegion &GetKernelStackRegion() {
                return *GetVirtualMemoryRegionTree().FindFirstRegionByType(KMemoryRegionType_KernelStack);
            }

            static NOINLINE KMemoryRegion &GetTempRegion() {
                return *GetVirtualMemoryRegionTree().FindFirstRegionByType(KMemoryRegionType_KernelTemp);
            }

            static NOINLINE KMemoryRegion &GetKernelTraceBufferRegion() {
                return *GetVirtualLinearMemoryRegionTree().FindFirstRegionByType(KMemoryRegionType_VirtualKernelTraceBuffer);
            }

            static NOINLINE KMemoryRegion &GetVirtualLinearRegion(KVirtualAddress address) {
                return *GetVirtualLinearMemoryRegionTree().FindContainingRegion(GetInteger(address));
            }

            static NOINLINE const KMemoryRegion *TryGetKernelTraceBufferRegion() {
                auto &tree = GetPhysicalMemoryRegionTree();
                if (KMemoryRegionTree::const_iterator it = tree.TryFindFirstDerivedRegion(KMemoryRegionType_KernelTraceBuffer); it != tree.end()) {
                    return std::addressof(*it);
                } else {
                    return nullptr;
                }
            }

            static NOINLINE const KMemoryRegion *TryGetOnMemoryBootImageRegion() {
                auto &tree = GetPhysicalMemoryRegionTree();
                if (KMemoryRegionTree::const_iterator it = tree.TryFindFirstDerivedRegion(KMemoryRegionType_OnMemoryBootImage); it != tree.end()) {
                    return std::addressof(*it);
                } else {
                    return nullptr;
                }
            }

            static NOINLINE const KMemoryRegion *TryGetDTBRegion() {
                auto &tree = GetPhysicalMemoryRegionTree();
                if (KMemoryRegionTree::const_iterator it = tree.TryFindFirstDerivedRegion(KMemoryRegionType_DTB); it != tree.end()) {
                    return std::addressof(*it);
                } else {
                    return nullptr;
                }
            }

            static NOINLINE bool IsHeapPhysicalAddress(const KMemoryRegion **out, KPhysicalAddress address, const KMemoryRegion *hint = nullptr) {
                auto &tree = GetPhysicalLinearMemoryRegionTree();
                KMemoryRegionTree::const_iterator it = tree.end();
                if (hint != nullptr) {
                    it = tree.iterator_to(*hint);
                }
                if (it == tree.end() || !it->Contains(GetInteger(address))) {
                    it = tree.FindContainingRegion(GetInteger(address));
                }
                if (it != tree.end() && it->IsDerivedFrom(KMemoryRegionType_DramNonKernel)) {
                    if (out) {
                        *out = std::addressof(*it);
                    }
                    return true;
                }
                return false;
            }

            static NOINLINE bool IsHeapPhysicalAddress(const KMemoryRegion **out, KPhysicalAddress address, size_t size, const KMemoryRegion *hint = nullptr) {
                auto &tree = GetPhysicalLinearMemoryRegionTree();
                KMemoryRegionTree::const_iterator it = tree.end();
                if (hint != nullptr) {
                    it = tree.iterator_to(*hint);
                }
                if (it == tree.end() || !it->Contains(GetInteger(address))) {
                    it = tree.FindContainingRegion(GetInteger(address));
                }
                if (it != tree.end() && it->IsDerivedFrom(KMemoryRegionType_DramNonKernel)) {
                    const uintptr_t last_address = GetInteger(address) + size - 1;
                    do {
                        if (last_address <= it->GetLastAddress()) {
                            if (out) {
                                *out = std::addressof(*it);
                            }
                            return true;
                        }
                        it++;
                    } while (it != tree.end() && it->IsDerivedFrom(KMemoryRegionType_DramNonKernel));
                }
                return false;
            }

            static NOINLINE bool IsLinearMappedPhysicalAddress(const KMemoryRegion **out, KPhysicalAddress address, const KMemoryRegion *hint = nullptr) {
                auto &tree = GetPhysicalLinearMemoryRegionTree();
                KMemoryRegionTree::const_iterator it = tree.end();
                if (hint != nullptr) {
                    it = tree.iterator_to(*hint);
                }
                if (it == tree.end() || !it->Contains(GetInteger(address))) {
                    it = tree.FindContainingRegion(GetInteger(address));
                }
                if (it != tree.end() && it->IsDerivedFrom(KMemoryRegionAttr_LinearMapped)) {
                    if (out) {
                        *out = std::addressof(*it);
                    }
                    return true;
                }
                return false;
            }

            static NOINLINE bool IsLinearMappedPhysicalAddress(const KMemoryRegion **out, KPhysicalAddress address, size_t size, const KMemoryRegion *hint = nullptr) {
                auto &tree = GetPhysicalLinearMemoryRegionTree();
                KMemoryRegionTree::const_iterator it = tree.end();
                if (hint != nullptr) {
                    it = tree.iterator_to(*hint);
                }
                if (it == tree.end() || !it->Contains(GetInteger(address))) {
                    it = tree.FindContainingRegion(GetInteger(address));
                }
                if (it != tree.end() && it->IsDerivedFrom(KMemoryRegionAttr_LinearMapped)) {
                    const uintptr_t last_address = GetInteger(address) + size - 1;
                    do {
                        if (last_address <= it->GetLastAddress()) {
                            if (out) {
                                *out = std::addressof(*it);
                            }
                            return true;
                        }
                        it++;
                    } while (it != tree.end() && it->IsDerivedFrom(KMemoryRegionAttr_LinearMapped));
                }
                return false;
            }

            static NOINLINE bool IsHeapVirtualAddress(const KMemoryRegion **out, KVirtualAddress address, const KMemoryRegion *hint = nullptr) {
                auto &tree = GetVirtualLinearMemoryRegionTree();
                KMemoryRegionTree::const_iterator it = tree.end();
                if (hint != nullptr) {
                    it = tree.iterator_to(*hint);
                }
                if (it == tree.end() || !it->Contains(GetInteger(address))) {
                    it = tree.FindContainingRegion(GetInteger(address));
                }
                if (it != tree.end() && it->IsDerivedFrom(KMemoryRegionType_VirtualDramManagedPool)) {
                    if (out) {
                        *out = std::addressof(*it);
                    }
                    return true;
                }
                return false;
            }

            static NOINLINE bool IsHeapVirtualAddress(const KMemoryRegion **out, KVirtualAddress address, size_t size, const KMemoryRegion *hint = nullptr) {
                auto &tree = GetVirtualLinearMemoryRegionTree();
                KMemoryRegionTree::const_iterator it = tree.end();
                if (hint != nullptr) {
                    it = tree.iterator_to(*hint);
                }
                if (it == tree.end() || !it->Contains(GetInteger(address))) {
                    it = tree.FindContainingRegion(GetInteger(address));
                }
                if (it != tree.end() && it->IsDerivedFrom(KMemoryRegionType_VirtualDramManagedPool)) {
                    const uintptr_t last_address = GetInteger(address) + size - 1;
                    do {
                        if (last_address <= it->GetLastAddress()) {
                            if (out) {
                                *out = std::addressof(*it);
                            }
                            return true;
                        }
                        it++;
                    } while (it != tree.end() && it->IsDerivedFrom(KMemoryRegionType_VirtualDramManagedPool));
                }
                return false;
            }

            static NOINLINE std::tuple<size_t, size_t> GetTotalAndKernelMemorySizes() {
                size_t total_size = 0, kernel_size = 0;
                for (auto it = GetPhysicalMemoryRegionTree().cbegin(); it != GetPhysicalMemoryRegionTree().cend(); it++) {
                    if (it->IsDerivedFrom(KMemoryRegionType_Dram)) {
                        total_size += it->GetSize();
                        if (!it->IsDerivedFrom(KMemoryRegionType_DramNonKernel)) {
                            kernel_size += it->GetSize();
                        }
                    }
                }
                return std::make_tuple(total_size, kernel_size);
            }

            static void InitializeLinearMemoryRegionTrees(KPhysicalAddress aligned_linear_phys_start, KVirtualAddress linear_virtual_start);

            static NOINLINE auto GetKernelRegionExtents() {
                return GetVirtualMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_Kernel);
            }

            static NOINLINE auto GetKernelCodeRegionExtents() {
                return GetVirtualMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_KernelCode);
            }

            static NOINLINE auto GetKernelStackRegionExtents() {
                return GetVirtualMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_KernelStack);
            }

            static NOINLINE auto GetKernelMiscRegionExtents() {
                return GetVirtualMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_KernelMisc);
            }

            static NOINLINE auto GetKernelSlabRegionExtents() {
                return GetVirtualMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_KernelSlab);
            }

            static NOINLINE const KMemoryRegion &GetCoreLocalRegion() {
                return *GetVirtualMemoryRegionTree().FindFirstRegionByType(KMemoryRegionType_CoreLocal);
            }

            static NOINLINE auto GetLinearRegionPhysicalExtents() {
                return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionAttr_LinearMapped);
            }

            static NOINLINE auto GetLinearRegionExtents() {
                return GetVirtualLinearExtents(GetLinearRegionPhysicalExtents());
            }

            static NOINLINE auto GetMainMemoryPhysicalExtents() {
                return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_Dram);
            }

            static NOINLINE auto GetCarveoutRegionExtents() {
                return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionAttr_CarveoutProtected);
            }

            static NOINLINE auto GetKernelRegionPhysicalExtents() {
                return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_DramKernel);
            }

            static NOINLINE auto GetKernelCodeRegionPhysicalExtents() {
                return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_DramKernelCode);
            }

            static NOINLINE auto GetKernelSlabRegionPhysicalExtents() {
                return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_DramKernelSlab);
            }

            static NOINLINE auto GetKernelPageTableHeapRegionPhysicalExtents() {
                return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_DramKernelPtHeap);
            }

            static NOINLINE auto GetKernelInitPageTableRegionPhysicalExtents() {
                return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_DramKernelInitPt);
            }

            static NOINLINE auto GetKernelPoolPartitionRegionPhysicalExtents() {
                return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_DramPoolPartition);
            }

            static NOINLINE auto GetKernelMetadataPoolRegionPhysicalExtents() {
                return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_DramMetadataPool);
            }

            static NOINLINE auto GetKernelSystemPoolRegionPhysicalExtents() {
                return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_DramSystemPool);
            }

            static NOINLINE auto GetKernelSystemNonSecurePoolRegionPhysicalExtents() {
                return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_DramSystemNonSecurePool);
            }

            static NOINLINE auto GetKernelAppletPoolRegionPhysicalExtents() {
                return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_DramAppletPool);
            }

            static NOINLINE auto GetKernelApplicationPoolRegionPhysicalExtents() {
                return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_DramApplicationPool);
            }

            static NOINLINE auto GetKernelTraceBufferRegionPhysicalExtents() {
                return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_KernelTraceBuffer);
            }
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
