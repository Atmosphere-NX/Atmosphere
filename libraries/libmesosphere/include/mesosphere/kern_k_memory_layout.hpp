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
#include <mesosphere/init/kern_init_page_table_select.hpp>
#include <mesosphere/kern_k_memory_region.hpp>

#if defined(ATMOSPHERE_BOARD_NINTENDO_NX)
    #include <mesosphere/board/nintendo/nx/kern_k_memory_layout.hpp>
#elif defined(ATMOSPHERE_BOARD_QEMU_VIRT)
    #include <mesosphere/board/qemu/virt/kern_k_memory_layout.hpp>
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

    constexpr size_t KernelPageTableHeapSize    = init::KInitialPageTable::GetMaximumOverheadSize(kern::MainMemorySizeMax);
    constexpr size_t KernelInitialPageHeapSize  = 128_KB;

    constexpr size_t KernelSlabHeapDataSize           = 5_MB;
    constexpr size_t KernelSlabHeapGapsSizeMax        = 2_MB - 64_KB;
    constexpr size_t KernelSlabHeapSize               = KernelSlabHeapDataSize + KernelSlabHeapGapsSizeMax;

    /* NOTE: This is calculated from KThread slab counts, assuming KThread size <= 0x800. */
    constexpr size_t KernelPageBufferHeapSize         = 0x3E0000;
    constexpr size_t KernelSlabHeapAdditionalSize     = 0x148000;
    constexpr size_t KernelPageBufferAdditionalSize   = 0x33C000;

    constexpr size_t KernelResourceSize               = KernelPageTableHeapSize + KernelInitialPageHeapSize + KernelSlabHeapSize + KernelPageBufferHeapSize;

    class KMemoryLayout {
        private:
            static constinit inline uintptr_t s_linear_phys_to_virt_diff;
            static constinit inline uintptr_t s_linear_virt_to_phys_diff;
            static constinit inline KMemoryRegionTree s_virtual_tree;
            static constinit inline KMemoryRegionTree s_physical_tree;
            static constinit inline KMemoryRegionTree s_virtual_linear_tree;
            static constinit inline KMemoryRegionTree s_physical_linear_tree;
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
                const auto &region = Dereference(GetVirtualMemoryRegionTree().FindByTypeAndAttribute(type, static_cast<u32>(core_id)));
                MESOSPHERE_INIT_ABORT_UNLESS(region.GetEndAddress() != 0);
                return region.GetEndAddress();
            }
        public:
            static ALWAYS_INLINE KMemoryRegionTree &GetVirtualMemoryRegionTree()        { return s_virtual_tree; }
            static ALWAYS_INLINE KMemoryRegionTree &GetPhysicalMemoryRegionTree()       { return s_physical_tree; }
            static ALWAYS_INLINE KMemoryRegionTree &GetVirtualLinearMemoryRegionTree()  { return s_virtual_linear_tree; }
            static ALWAYS_INLINE KMemoryRegionTree &GetPhysicalLinearMemoryRegionTree() { return s_physical_linear_tree; }

            static ALWAYS_INLINE KVirtualAddress GetLinearVirtualAddress(KPhysicalAddress address)  { return GetInteger(address) + s_linear_phys_to_virt_diff; }
            static ALWAYS_INLINE KPhysicalAddress GetLinearPhysicalAddress(KVirtualAddress address) { return GetInteger(address) + s_linear_virt_to_phys_diff; }

            static MESOSPHERE_NOINLINE_IF_DEBUG const KMemoryRegion *Find(KVirtualAddress address)  { return Find(address, GetVirtualMemoryRegionTree()); }
            static MESOSPHERE_NOINLINE_IF_DEBUG const KMemoryRegion *Find(KPhysicalAddress address) { return Find(address, GetPhysicalMemoryRegionTree()); }

            static MESOSPHERE_NOINLINE_IF_DEBUG const KMemoryRegion *FindLinear(KVirtualAddress address)  { return Find(address, GetVirtualLinearMemoryRegionTree());  }
            static MESOSPHERE_NOINLINE_IF_DEBUG const KMemoryRegion *FindLinear(KPhysicalAddress address) { return Find(address, GetPhysicalLinearMemoryRegionTree()); }

            static MESOSPHERE_NOINLINE_IF_DEBUG KVirtualAddress GetMainStackTopAddress(s32 core_id)      { return GetStackTopAddress(core_id, KMemoryRegionType_KernelMiscMainStack); }
            static MESOSPHERE_NOINLINE_IF_DEBUG KVirtualAddress GetIdleStackTopAddress(s32 core_id)      { return GetStackTopAddress(core_id, KMemoryRegionType_KernelMiscIdleStack); }
            static MESOSPHERE_NOINLINE_IF_DEBUG KVirtualAddress GetExceptionStackTopAddress(s32 core_id) { return GetStackTopAddress(core_id, KMemoryRegionType_KernelMiscExceptionStack); }

            static MESOSPHERE_NOINLINE_IF_DEBUG KVirtualAddress GetSlabRegionAddress()      { return Dereference(GetVirtualMemoryRegionTree().FindByType(KMemoryRegionType_KernelSlab)).GetAddress(); }

            static MESOSPHERE_NOINLINE_IF_DEBUG const KMemoryRegion &GetDeviceRegion(KMemoryRegionType type) { return Dereference(GetPhysicalMemoryRegionTree().FindFirstDerived(type)); }
            static KPhysicalAddress GetDevicePhysicalAddress(KMemoryRegionType type) { return GetDeviceRegion(type).GetAddress(); }
            static KVirtualAddress  GetDeviceVirtualAddress(KMemoryRegionType type)  { return GetDeviceRegion(type).GetPairAddress(); }

            static MESOSPHERE_NOINLINE_IF_DEBUG const KMemoryRegion &GetPoolManagementRegion() { return Dereference(GetVirtualMemoryRegionTree().FindByType(KMemoryRegionType_VirtualDramPoolManagement)); }
            static MESOSPHERE_NOINLINE_IF_DEBUG const KMemoryRegion &GetPageTableHeapRegion()  { return Dereference(GetVirtualMemoryRegionTree().FindByType(KMemoryRegionType_VirtualDramKernelPtHeap)); }
            static MESOSPHERE_NOINLINE_IF_DEBUG const KMemoryRegion &GetKernelStackRegion()    { return Dereference(GetVirtualMemoryRegionTree().FindByType(KMemoryRegionType_KernelStack)); }
            static MESOSPHERE_NOINLINE_IF_DEBUG const KMemoryRegion &GetTempRegion()           { return Dereference(GetVirtualMemoryRegionTree().FindByType(KMemoryRegionType_KernelTemp)); }
            static MESOSPHERE_NOINLINE_IF_DEBUG const KMemoryRegion &GetSlabRegion()           { return Dereference(GetVirtualMemoryRegionTree().FindByType(KMemoryRegionType_KernelSlab)); }

            static MESOSPHERE_NOINLINE_IF_DEBUG const KMemoryRegion &GetKernelTraceBufferRegion() { return Dereference(GetVirtualLinearMemoryRegionTree().FindByType(KMemoryRegionType_VirtualDramKernelTraceBuffer)); }

            static MESOSPHERE_NOINLINE_IF_DEBUG const KMemoryRegion &GetSecureAppletMemoryRegion() { return Dereference(GetVirtualMemoryRegionTree().FindByType(KMemoryRegionType_VirtualDramKernelSecureAppletMemory)); }

            static MESOSPHERE_NOINLINE_IF_DEBUG const KMemoryRegion &GetVirtualLinearRegion(KVirtualAddress address) { return Dereference(FindLinear(address)); }
            static MESOSPHERE_NOINLINE_IF_DEBUG const KMemoryRegion &GetPhysicalLinearRegion(KPhysicalAddress address) { return Dereference(FindLinear(address)); }

            static MESOSPHERE_NOINLINE_IF_DEBUG const KMemoryRegion *GetPhysicalKernelTraceBufferRegion() { return GetPhysicalMemoryRegionTree().FindFirstDerived(KMemoryRegionType_KernelTraceBuffer); }
            static MESOSPHERE_NOINLINE_IF_DEBUG const KMemoryRegion *GetPhysicalOnMemoryBootImageRegion() { return GetPhysicalMemoryRegionTree().FindFirstDerived(KMemoryRegionType_OnMemoryBootImage); }
            static MESOSPHERE_NOINLINE_IF_DEBUG const KMemoryRegion *GetPhysicalDTBRegion()               { return GetPhysicalMemoryRegionTree().FindFirstDerived(KMemoryRegionType_DTB); }

            static MESOSPHERE_NOINLINE_IF_DEBUG bool IsHeapPhysicalAddress(const KMemoryRegion *&region, KPhysicalAddress address) { return IsTypedAddress(region, address, GetPhysicalLinearMemoryRegionTree(), KMemoryRegionType_DramUserPool); }
            static MESOSPHERE_NOINLINE_IF_DEBUG bool IsHeapVirtualAddress(const KMemoryRegion *&region, KVirtualAddress address)   { return IsTypedAddress(region, address, GetVirtualLinearMemoryRegionTree(),  KMemoryRegionType_VirtualDramUserPool); }

            static MESOSPHERE_NOINLINE_IF_DEBUG bool IsHeapPhysicalAddress(const KMemoryRegion *&region, KPhysicalAddress address, size_t size) { return IsTypedAddress(region, address, size, GetPhysicalLinearMemoryRegionTree(), KMemoryRegionType_DramUserPool); }
            static MESOSPHERE_NOINLINE_IF_DEBUG bool IsHeapVirtualAddress(const KMemoryRegion *&region, KVirtualAddress address, size_t size)   { return IsTypedAddress(region, address, size, GetVirtualLinearMemoryRegionTree(),  KMemoryRegionType_VirtualDramUserPool); }

            static MESOSPHERE_NOINLINE_IF_DEBUG bool IsLinearMappedPhysicalAddress(const KMemoryRegion *&region, KPhysicalAddress address)  { return IsTypedAddress(region, address, GetPhysicalLinearMemoryRegionTree(), static_cast<KMemoryRegionType>(KMemoryRegionAttr_LinearMapped)); }
            static MESOSPHERE_NOINLINE_IF_DEBUG bool IsLinearMappedPhysicalAddress(const KMemoryRegion *&region, KPhysicalAddress address, size_t size) { return IsTypedAddress(region, address, size, GetPhysicalLinearMemoryRegionTree(), static_cast<KMemoryRegionType>(KMemoryRegionAttr_LinearMapped)); }

            static  std::tuple<size_t, size_t> GetTotalAndKernelMemorySizes() {
                size_t total_size = 0, kernel_size = 0;
                for (const auto &region : GetPhysicalMemoryRegionTree()) {
                    if (region.IsDerivedFrom(KMemoryRegionType_Dram)) {
                        total_size += region.GetSize();
                        if (!region.IsDerivedFrom(KMemoryRegionType_DramUserPool)) {
                            kernel_size += region.GetSize();
                        }
                    }
                }
                return std::make_tuple(total_size, kernel_size);
            }

            static void InitializeLinearMemoryAddresses(u64 phys_to_virt_diff) {
                /* Set static differences. */
                s_linear_phys_to_virt_diff =  phys_to_virt_diff;
                s_linear_virt_to_phys_diff = -phys_to_virt_diff;
            }

            static void InitializeLinearMemoryRegionTrees();

            static size_t GetResourceRegionSizeForInit(bool use_extra_resource);

            static MESOSPHERE_NOINLINE_IF_DEBUG auto GetKernelRegionExtents()      { return GetVirtualMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_Kernel); }
            static MESOSPHERE_NOINLINE_IF_DEBUG auto GetKernelCodeRegionExtents()  { return GetVirtualMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_KernelCode); }
            static MESOSPHERE_NOINLINE_IF_DEBUG auto GetKernelStackRegionExtents() { return GetVirtualMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_KernelStack); }
            static MESOSPHERE_NOINLINE_IF_DEBUG auto GetKernelMiscRegionExtents()  { return GetVirtualMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_KernelMisc); }
            static MESOSPHERE_NOINLINE_IF_DEBUG auto GetKernelSlabRegionExtents()  { return GetVirtualMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_KernelSlab); }


            static MESOSPHERE_NOINLINE_IF_DEBUG auto GetLinearRegionPhysicalExtents() { return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionAttr_LinearMapped); }

            static MESOSPHERE_NOINLINE_IF_DEBUG auto GetLinearRegionVirtualExtents()  {
                const auto physical = GetLinearRegionPhysicalExtents();
                return KMemoryRegion(GetInteger(GetLinearVirtualAddress(physical.GetAddress())), GetInteger(GetLinearVirtualAddress(physical.GetLastAddress())), 0, KMemoryRegionType_None);
            }

            static MESOSPHERE_NOINLINE_IF_DEBUG auto GetMainMemoryPhysicalExtents() { return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_Dram); }
            static MESOSPHERE_NOINLINE_IF_DEBUG auto GetCarveoutRegionExtents() { return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionAttr_CarveoutProtected); }

            static MESOSPHERE_NOINLINE_IF_DEBUG auto GetKernelRegionPhysicalExtents() { return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_DramKernelBase); }
            static MESOSPHERE_NOINLINE_IF_DEBUG auto GetKernelCodeRegionPhysicalExtents() { return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_DramKernelCode); }
            static MESOSPHERE_NOINLINE_IF_DEBUG auto GetKernelSlabRegionPhysicalExtents() { return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_DramKernelSlab); }
            static MESOSPHERE_NOINLINE_IF_DEBUG auto GetKernelSecureAppletMemoryRegionPhysicalExtents() { return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_DramKernelSecureAppletMemory); }
            static MESOSPHERE_NOINLINE_IF_DEBUG auto GetKernelPageTableHeapRegionPhysicalExtents() { return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_DramKernelPtHeap); }
            static MESOSPHERE_NOINLINE_IF_DEBUG auto GetKernelInitPageTableRegionPhysicalExtents() { return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_DramKernelInitPt); }

            static MESOSPHERE_NOINLINE_IF_DEBUG auto GetKernelPoolPartitionRegionPhysicalExtents() { return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_DramPoolPartition); }
            static MESOSPHERE_NOINLINE_IF_DEBUG auto GetKernelPoolManagementRegionPhysicalExtents() { return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_DramPoolManagement); }
            static MESOSPHERE_NOINLINE_IF_DEBUG auto GetKernelSystemPoolRegionPhysicalExtents() { return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_DramSystemPool); }
            static MESOSPHERE_NOINLINE_IF_DEBUG auto GetKernelSystemNonSecurePoolRegionPhysicalExtents() { return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_DramSystemNonSecurePool); }
            static MESOSPHERE_NOINLINE_IF_DEBUG auto GetKernelAppletPoolRegionPhysicalExtents() { return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_DramAppletPool); }
            static MESOSPHERE_NOINLINE_IF_DEBUG auto GetKernelApplicationPoolRegionPhysicalExtents() { return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_DramApplicationPool); }

            static MESOSPHERE_NOINLINE_IF_DEBUG bool HasKernelSystemNonSecurePoolRegion() { return GetPhysicalMemoryRegionTree().FindFirstDerived(KMemoryRegionType_DramSystemNonSecurePool) != nullptr; }
            static MESOSPHERE_NOINLINE_IF_DEBUG bool HasKernelAppletPoolRegion() { return GetPhysicalMemoryRegionTree().FindFirstDerived(KMemoryRegionType_DramAppletPool) != nullptr; }
            static MESOSPHERE_NOINLINE_IF_DEBUG bool HasKernelApplicationPoolRegion() { return GetPhysicalMemoryRegionTree().FindFirstDerived(KMemoryRegionType_DramApplicationPool) != nullptr; }

            static MESOSPHERE_NOINLINE_IF_DEBUG auto GetKernelTraceBufferRegionPhysicalExtents() { return GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionType_KernelTraceBuffer); }
    };

    namespace init {

        /* These should be generic, regardless of board. */
        void SetupPoolPartitionMemoryRegions();

        /* These may be implemented in a board-specific manner. */
        void SetupDevicePhysicalMemoryRegions();
        void SetupDramPhysicalMemoryRegions();

    }

}
