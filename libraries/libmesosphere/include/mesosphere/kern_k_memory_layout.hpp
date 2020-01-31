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
        KMemoryRegionType_VirtualDramApplicationPool     = 0x271A,
        KMemoryRegionType_VirtualDramAppletPool          = 0x1B1A,
        KMemoryRegionType_VirtualDramSystemNonSecurePool = 0x331A,
        KMemoryRegionType_VirtualDramSystemPool          = 0x2B1A,

        KMemoryRegionType_Uart                      = 0x1D,
        KMemoryRegionType_InterruptDistributor      = 0x4D,
        KMemoryRegionType_InterruptController       = 0x2D,

        KMemoryRegionType_MemoryController          = 0x55,
        KMemoryRegionType_MemoryController0         = 0x95,
        KMemoryRegionType_MemoryController1         = 0x65,
        KMemoryRegionType_PowerManagementController = 0x1A5,

        KMemoryRegionType_KernelAutoMap = KMemoryRegionType_Kernel | KMemoryRegionAttr_ShouldKernelMap,

        KMemoryRegionType_KernelTemp  = 0x31,

        KMemoryRegionType_KernelCode  = 0x19,
        KMemoryRegionType_KernelStack = 0x29,
        KMemoryRegionType_KernelMisc  = 0x49,
        KMemoryRegionType_KernelSlab  = 0x89,

        KMemoryRegionType_KernelMiscMainStack      = 0xB49,
        KMemoryRegionType_KernelMiscMappedDevice   = 0xD49,
        KMemoryRegionType_KernelMiscIdleStack      = 0x1349,
        KMemoryRegionType_KernelMiscUnknownDebug   = 0x1549,
        KMemoryRegionType_KernelMiscExceptionStack = 0x2349,

        KMemoryRegionType_DramLinearMapped  = KMemoryRegionType_Dram  | KMemoryRegionAttr_LinearMapped,

        KMemoryRegionType_DramReservedEarly       = 0x16  | KMemoryRegionAttr_NoUserMap,
        KMemoryRegionType_DramPoolPartition       = 0x26  | KMemoryRegionAttr_NoUserMap | KMemoryRegionAttr_LinearMapped,
        KMemoryRegionType_DramMetadataPool        = 0x166  | KMemoryRegionAttr_NoUserMap | KMemoryRegionAttr_LinearMapped | KMemoryRegionAttr_CarveoutProtected,
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

    class KMemoryBlock : public util::IntrusiveRedBlackTreeBaseNode<KMemoryBlock> {
        NON_COPYABLE(KMemoryBlock);
        NON_MOVEABLE(KMemoryBlock);
        private:
            uintptr_t address;
            uintptr_t pair_address;
            size_t block_size;
            u32 attributes;
            u32 type_id;
        public:
            static constexpr ALWAYS_INLINE int Compare(const KMemoryBlock &lhs, const KMemoryBlock &rhs) {
                if (lhs.GetAddress() < rhs.GetAddress()) {
                    return -1;
                } else if (lhs.GetLastAddress() > rhs.GetLastAddress()) {
                    return 1;
                } else {
                    return 0;
                }
            }
        public:
            constexpr ALWAYS_INLINE KMemoryBlock() : address(0), pair_address(0), block_size(0), attributes(0), type_id(0) { /* ... */ }
            constexpr ALWAYS_INLINE KMemoryBlock(uintptr_t a, size_t bl, uintptr_t p, u32 r, u32 t) :
                address(a), pair_address(p), block_size(bl), attributes(r), type_id(t)
            {
                /* ... */
            }
            constexpr ALWAYS_INLINE KMemoryBlock(uintptr_t a, size_t bl, u32 r, u32 t) : KMemoryBlock(a, bl, std::numeric_limits<uintptr_t>::max(), r, t) { /* ... */ }

            constexpr ALWAYS_INLINE uintptr_t GetAddress() const {
                return this->address;
            }

            constexpr ALWAYS_INLINE uintptr_t GetPairAddress() const {
                return this->pair_address;
            }

            constexpr ALWAYS_INLINE size_t GetSize() const {
                return this->block_size;
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
                return this->GetAddress() <= address && address < this->GetLastAddress();
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
    static_assert(std::is_trivially_destructible<KMemoryBlock>::value);

    class KMemoryBlockTree {
        public:
            struct DerivedRegionExtents {
                const KMemoryBlock *first_block;
                const KMemoryBlock *last_block;
            };
        private:
            using TreeType = util::IntrusiveRedBlackTreeBaseTraits<KMemoryBlock>::TreeType<KMemoryBlock>;
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
            constexpr ALWAYS_INLINE KMemoryBlockTree() : tree() { /* ... */ }
        public:
            iterator FindContainingBlock(uintptr_t address) {
                auto it = this->find(KMemoryBlock(address, 1, 0, 0));
                MESOSPHERE_INIT_ABORT_UNLESS(it != this->end());
                MESOSPHERE_INIT_ABORT_UNLESS(it->Contains(address));

                return it;
            }

            iterator FindFirstBlockByTypeAttr(u32 type_id, u32 attr = 0) {
                for (auto it = this->begin(); it != this->end(); it++) {
                    if (it->GetType() == type_id && it->GetAttributes() == attr) {
                        return it;
                    }
                }
                MESOSPHERE_INIT_ABORT();
            }

            iterator FindFirstBlockByType(u32 type_id) {
                for (auto it = this->begin(); it != this->end(); it++) {
                    if (it->GetType() == type_id) {
                        return it;
                    }
                }
                MESOSPHERE_INIT_ABORT();
            }

            iterator FindFirstDerivedBlock(u32 type_id) {
                for (auto it = this->begin(); it != this->end(); it++) {
                    if (it->IsDerivedFrom(type_id)) {
                        return it;
                    }
                }
                MESOSPHERE_INIT_ABORT();
            }


            DerivedRegionExtents GetDerivedRegionExtents(u32 type_id) {
                DerivedRegionExtents extents = { .first_block = nullptr, .last_block = nullptr };

                for (auto it = this->cbegin(); it != this->cend(); it++) {
                    if (it->IsDerivedFrom(type_id)) {
                        if (extents.first_block == nullptr) {
                            extents.first_block = std::addressof(*it);
                        }
                        extents.last_block = std::addressof(*it);
                    }
                }

                MESOSPHERE_INIT_ABORT_UNLESS(extents.first_block != nullptr);
                MESOSPHERE_INIT_ABORT_UNLESS(extents.last_block  != nullptr);

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

    class KMemoryBlockAllocator {
        NON_COPYABLE(KMemoryBlockAllocator);
        NON_MOVEABLE(KMemoryBlockAllocator);
        public:
            static constexpr size_t MaxMemoryBlocks = 1000;
            friend class KMemoryLayout;
        private:
            KMemoryBlock block_heap[MaxMemoryBlocks];
            size_t num_blocks;
        private:
            constexpr ALWAYS_INLINE KMemoryBlockAllocator() : block_heap(), num_blocks() { /* ... */ }
        public:
            ALWAYS_INLINE KMemoryBlock *Allocate() {
                /* Ensure we stay within the bounds of our heap. */
                MESOSPHERE_INIT_ABORT_UNLESS(this->num_blocks < MaxMemoryBlocks);

                return &this->block_heap[this->num_blocks++];
            }

            template<typename... Args>
            ALWAYS_INLINE KMemoryBlock *Create(Args&&... args) {
                KMemoryBlock *block = this->Allocate();
                new (block) KMemoryBlock(std::forward<Args>(args)...);
                return block;
            }
    };

    class KMemoryLayout {
        private:
            static /* constinit */ inline uintptr_t s_linear_phys_to_virt_diff;
            static /* constinit */ inline uintptr_t s_linear_virt_to_phys_diff;
            static /* constinit */ inline KMemoryBlockAllocator s_block_allocator;
            static /* constinit */ inline KMemoryBlockTree s_virtual_tree;
            static /* constinit */ inline KMemoryBlockTree s_physical_tree;
            static /* constinit */ inline KMemoryBlockTree s_virtual_linear_tree;
            static /* constinit */ inline KMemoryBlockTree s_physical_linear_tree;
        public:
            static ALWAYS_INLINE KMemoryBlockAllocator &GetMemoryBlockAllocator()        { return s_block_allocator; }
            static ALWAYS_INLINE KMemoryBlockTree      &GetVirtualMemoryBlockTree()      { return s_virtual_tree; }
            static ALWAYS_INLINE KMemoryBlockTree      &GetPhysicalMemoryBlockTree()     { return s_physical_tree; }
            static ALWAYS_INLINE KMemoryBlockTree      &GetVirtualLinearMemoryBlockTree()  { return s_virtual_linear_tree; }
            static ALWAYS_INLINE KMemoryBlockTree      &GetPhysicalLinearMemoryBlockTree() { return s_physical_linear_tree; }

            static ALWAYS_INLINE KVirtualAddress GetLinearVirtualAddress(KPhysicalAddress address) {
                return GetInteger(address) + s_linear_phys_to_virt_diff;
            }

            static ALWAYS_INLINE KPhysicalAddress GetLinearPhysicalAddress(KVirtualAddress address) {
                return GetInteger(address) + s_linear_virt_to_phys_diff;
            }

            static NOINLINE KVirtualAddress GetMainStackTopAddress(s32 core_id) {
                return GetVirtualMemoryBlockTree().FindFirstBlockByTypeAttr(KMemoryRegionType_KernelMiscMainStack, static_cast<u32>(core_id))->GetEndAddress();
            }

            static NOINLINE KVirtualAddress GetIdleStackTopAddress(s32 core_id) {
                return GetVirtualMemoryBlockTree().FindFirstBlockByTypeAttr(KMemoryRegionType_KernelMiscIdleStack, static_cast<u32>(core_id))->GetEndAddress();
            }

            static NOINLINE KVirtualAddress GetExceptionStackBottomAddress(s32 core_id) {
                return GetVirtualMemoryBlockTree().FindFirstBlockByTypeAttr(KMemoryRegionType_KernelMiscExceptionStack, static_cast<u32>(core_id))->GetAddress();
            }

            static NOINLINE KVirtualAddress GetSlabRegionAddress() {
                return GetVirtualMemoryBlockTree().FindFirstBlockByType(KMemoryRegionType_KernelSlab)->GetAddress();
            }

            static NOINLINE KVirtualAddress GetCoreLocalRegionAddress() {
                return GetVirtualMemoryBlockTree().FindFirstBlockByType(KMemoryRegionType_CoreLocal)->GetAddress();
            }

            static void InitializeLinearMemoryBlockTrees(KPhysicalAddress aligned_linear_phys_start, KVirtualAddress linear_virtual_start);
    };


    namespace init {

        /* These should be generic, regardless of board. */
        void SetupCoreLocalRegionMemoryBlocks(KInitialPageTable &page_table, KInitialPageAllocator &page_allocator);
        void SetupPoolPartitionMemoryBlocks();

        /* These may be implemented in a board-specific manner. */
        void SetupDevicePhysicalMemoryBlocks();
        void SetupDramPhysicalMemoryBlocks();

    }

}
