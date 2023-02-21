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
#include <mesosphere.hpp>

extern "C" void _start();
extern "C" void __end__();

namespace ams::kern {

    void ExceptionVectors();

}

namespace ams::kern::init {

    /* Prototypes for functions declared in ASM that we need to reference. */
    void StartOtherCore(const ams::kern::init::KInitArguments *init_args);

    void IdentityMappedFunctionAreaBegin();
    void IdentityMappedFunctionAreaEnd();

    size_t GetMiscUnknownDebugRegionSize();

    void InitializeDebugRegisters();
    void InitializeExceptionVectors();

    namespace {

        /* Global Allocator. */
        constinit KInitialPageAllocator g_initial_page_allocator;

        constinit KInitArguments g_init_arguments[cpu::NumCores];

        /* Globals for passing data between InitializeCorePhase1 and InitializeCorePhase2. */
        constinit InitialProcessBinaryLayout g_phase2_initial_process_binary_layout{};
        constinit KPhysicalAddress g_phase2_resource_end_phys_addr = Null<KPhysicalAddress>;
        constinit u64 g_phase2_linear_region_phys_to_virt_diff = 0;

        /* Page table attributes. */
        constexpr PageTableEntry KernelTextAttribute(PageTableEntry::Permission_KernelRX, PageTableEntry::PageAttribute_NormalMemory, PageTableEntry::Shareable_InnerShareable, PageTableEntry::MappingFlag_Mapped);
        constexpr PageTableEntry KernelRoDataAttribute(PageTableEntry::Permission_KernelR,  PageTableEntry::PageAttribute_NormalMemory, PageTableEntry::Shareable_InnerShareable, PageTableEntry::MappingFlag_Mapped);
        constexpr PageTableEntry KernelRwDataAttribute(PageTableEntry::Permission_KernelRW, PageTableEntry::PageAttribute_NormalMemory, PageTableEntry::Shareable_InnerShareable, PageTableEntry::MappingFlag_Mapped);
        constexpr PageTableEntry KernelMmioAttribute(PageTableEntry::Permission_KernelRW, PageTableEntry::PageAttribute_Device_nGnRE, PageTableEntry::Shareable_OuterShareable, PageTableEntry::MappingFlag_Mapped);

        constexpr PageTableEntry KernelRwDataUncachedAttribute(PageTableEntry::Permission_KernelRW, PageTableEntry::PageAttribute_NormalMemoryNotCacheable, PageTableEntry::Shareable_InnerShareable, PageTableEntry::MappingFlag_Mapped);

        void TurnOnAllCores() {
            cpu::MultiprocessorAffinityRegisterAccessor mpidr;
            const auto arg = mpidr.GetCpuOnArgument();
            const auto current_core = mpidr.GetAff0();

            for (s32 i = 0; i < static_cast<s32>(cpu::NumCores); i++) {
                if (static_cast<s32>(current_core) != i) {
                    KSystemControl::Init::TurnOnCpu(arg | i, g_init_arguments + i);
                }
            }
        }

        void InvokeMain(u64 core_id) {
            /* Clear cpacr_el1. */
            cpu::SetCpacrEl1(0);
            cpu::InstructionMemoryBarrier();

            /* Initialize registers. */
            InitializeDebugRegisters();
            InitializeExceptionVectors();

            /* Set exception stack. */
            cpu::SetCntvCvalEl0(GetInteger(KMemoryLayout::GetExceptionStackTopAddress(static_cast<s32>(core_id))) - sizeof(KThread::StackParameters));

            /* Call main. */
            HorizonKernelMain(static_cast<s32>(core_id));
        }

        void SetupInitialArguments() {
            /* Get parameters for initial arguments. */
            const u64 ttbr0    = cpu::GetTtbr0El1();
            const u64 ttbr1    = cpu::GetTtbr1El1();
            const u64 tcr      = cpu::GetTcrEl1();
            const u64 mair     = cpu::GetMairEl1();
            const u64 cpuactlr = cpu::GetCpuActlrEl1();
            const u64 cpuectlr = cpu::GetCpuEctlrEl1();
            const u64 sctlr    = cpu::GetSctlrEl1();

            for (s32 i = 0; i < static_cast<s32>(cpu::NumCores); ++i) {
                /* Get the arguments. */
                KInitArguments *init_args = g_init_arguments + i;

                /* Set the arguments. */
                init_args->ttbr0           = ttbr0;
                init_args->ttbr1           = ttbr1;
                init_args->tcr             = tcr;
                init_args->mair            = mair;
                init_args->cpuactlr        = cpuactlr;
                init_args->cpuectlr        = cpuectlr;
                init_args->sctlr           = sctlr;
                init_args->sp              = GetInteger(KMemoryLayout::GetMainStackTopAddress(i)) - sizeof(KThread::StackParameters);
                init_args->entrypoint      = reinterpret_cast<uintptr_t>(::ams::kern::init::InvokeMain);
                init_args->argument        = static_cast<u64>(i);
            }
        }

        KVirtualAddress GetRandomAlignedRegionWithGuard(size_t size, size_t alignment, KInitialPageTable &pt, KMemoryRegionTree &tree, u32 type_id, size_t guard_size) {
            /* Check that the size is valid. */
            MESOSPHERE_INIT_ABORT_UNLESS(size > 0);

            /* We want to find the total extents of the type id. */
            const auto extents = tree.GetDerivedRegionExtents(type_id);

            /* Ensure that our alignment is correct. */
            MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(extents.GetAddress(), alignment));

            const uintptr_t first_address = extents.GetAddress();
            const uintptr_t last_address  = extents.GetLastAddress();

            const uintptr_t first_index = first_address / alignment;
            const uintptr_t last_index  = last_address / alignment;

            while (true) {
                const uintptr_t candidate_start = KSystemControl::Init::GenerateRandomRange(first_index, last_index) * alignment;
                const uintptr_t candidate_end   = candidate_start + size + guard_size;

                /* Ensure that the candidate doesn't overflow with the size/guard. */
                if (!(candidate_start < candidate_end) || !(candidate_start >= guard_size)) {
                    continue;
                }

                const uintptr_t candidate_last = candidate_end - 1;

                /* Ensure that the candidate fits within the region. */
                if (candidate_last > last_address) {
                    continue;
                }

                /* Ensure that the candidate range is free. */
                if (!pt.IsFree(candidate_start, size)) {
                    continue;
                }

                /* Locate the candidate's guard start, and ensure the whole range fits/has the correct type id. */
                if (const auto &candidate_region = *tree.Find(candidate_start - guard_size); !(candidate_last <= candidate_region.GetLastAddress() && candidate_region.GetType() == type_id)) {
                    continue;
                }

                return candidate_start;
            }
        }

        KVirtualAddress GetRandomAlignedRegion(size_t size, size_t alignment, KInitialPageTable &pt, KMemoryRegionTree &tree, u32 type_id) {
            return GetRandomAlignedRegionWithGuard(size, alignment, pt, tree, type_id, 0);
        }

        void MapStackForCore(KInitialPageTable &page_table, KMemoryRegionType type, u32 core_id) {
            constexpr size_t StackSize  = PageSize;
            constexpr size_t StackAlign = PageSize;
            const KVirtualAddress  stack_start_virt = GetRandomAlignedRegionWithGuard(StackSize, StackAlign, page_table, KMemoryLayout::GetVirtualMemoryRegionTree(), KMemoryRegionType_KernelMisc, PageSize);
            const KPhysicalAddress stack_start_phys = g_initial_page_allocator.Allocate(PageSize);
            MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetVirtualMemoryRegionTree().Insert(GetInteger(stack_start_virt), StackSize, type, core_id));

            page_table.Map(stack_start_virt, StackSize, stack_start_phys, KernelRwDataAttribute, g_initial_page_allocator, 0);
        }

        class KInitialPageAllocatorForFinalizeIdentityMapping final {
            private:
                struct FreeListEntry {
                    FreeListEntry *next;
                };
            private:
                FreeListEntry *m_free_list_head;
                u64 m_phys_to_virt_offset;
            public:
                template<kern::arch::arm64::init::IsInitialPageAllocator PageAllocator>
                KInitialPageAllocatorForFinalizeIdentityMapping(PageAllocator &allocator, u64 phys_to_virt) : m_free_list_head(nullptr), m_phys_to_virt_offset(phys_to_virt) {
                    /* Allocate and free two pages. */
                    for (size_t i = 0; i < 2; ++i) {
                        KPhysicalAddress page = allocator.Allocate(PageSize);
                        MESOSPHERE_INIT_ABORT_UNLESS(page != Null<KPhysicalAddress>);

                        /* Free the pages. */
                        this->Free(page, PageSize);
                    }
                }
            public:
                KPhysicalAddress Allocate(size_t size) {
                    /* Check that the size is correct. */
                    MESOSPHERE_INIT_ABORT_UNLESS(size == PageSize);

                    /* Check that we have a free page. */
                    FreeListEntry *head = m_free_list_head;
                    MESOSPHERE_INIT_ABORT_UNLESS(head != nullptr);

                    /* Update the free list. */
                    m_free_list_head = head->next;

                    /* Return the page. */
                    return KPhysicalAddress(reinterpret_cast<uintptr_t>(head) - m_phys_to_virt_offset);
                }

                void Free(KPhysicalAddress phys_addr, size_t size) {
                    /* Check that the size is correct. */
                    MESOSPHERE_INIT_ABORT_UNLESS(size == PageSize);

                    /* Convert to a free list entry. */
                    FreeListEntry *fl = reinterpret_cast<FreeListEntry *>(GetInteger(phys_addr) + m_phys_to_virt_offset);

                    /* Insert into free list. */
                    fl->next = m_free_list_head;
                    m_free_list_head = fl;
                }
        };
        static_assert(kern::arch::arm64::init::IsInitialPageAllocator<KInitialPageAllocatorForFinalizeIdentityMapping>);

        void FinalizeIdentityMapping(KInitialPageTable &init_pt, KInitialPageAllocator &allocator, u64 phys_to_virt_offset) {
            /* Create an allocator for identity mapping finalization. */
            KInitialPageAllocatorForFinalizeIdentityMapping finalize_allocator(allocator, phys_to_virt_offset);

            /* Get the physical address of crt0. */
            const KPhysicalAddress start_phys_addr = init_pt.GetPhysicalAddress(reinterpret_cast<uintptr_t>(::ams::kern::init::IdentityMappedFunctionAreaBegin));

            /* Unmap the entire identity mapping. */
            init_pt.UnmapTtbr0Entries(phys_to_virt_offset);

            /* Re-map only the first page of code. */
            const size_t size = util::AlignUp<size_t>(reinterpret_cast<uintptr_t>(::ams::kern::init::IdentityMappedFunctionAreaEnd) - reinterpret_cast<uintptr_t>(::ams::kern::init::IdentityMappedFunctionAreaBegin), PageSize);
            init_pt.Map(KVirtualAddress(GetInteger(start_phys_addr)), size, start_phys_addr, KernelTextAttribute, finalize_allocator, phys_to_virt_offset);
        }

    }

    void InitializeCorePhase1(uintptr_t misc_unk_debug_phys_addr, void **initial_state) {
        /* Ensure our first argument is page aligned. */
        MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(misc_unk_debug_phys_addr, PageSize));

        /* Decode the initial state. */
        const auto initial_page_allocator_state = *static_cast<KInitialPageAllocator::State *>(initial_state[0]);
        g_phase2_initial_process_binary_layout  = *static_cast<InitialProcessBinaryLayout *>(initial_state[1]);

        /* Restore the page allocator state setup by kernel loader. */
        g_initial_page_allocator.InitializeFromState(std::addressof(initial_page_allocator_state));

        /* Ensure that the T1SZ is correct (and what we expect). */
        MESOSPHERE_INIT_ABORT_UNLESS((cpu::TranslationControlRegisterAccessor().GetT1Size() / arch::arm64::L1BlockSize) == arch::arm64::MaxPageTableEntries);

        /* Create page table object for use during initialization. */
        KInitialPageTable init_pt;

        /* Initialize the slab allocator counts. */
        InitializeSlabResourceCounts();

        /* Insert the root region for the virtual memory tree, from which all other regions will derive. */
        KMemoryLayout::GetVirtualMemoryRegionTree().InsertDirectly(KernelVirtualAddressSpaceBase, KernelVirtualAddressSpaceBase + KernelVirtualAddressSpaceSize - 1);

        /* Insert the root region for the physical memory tree, from which all other regions will derive. */
        KMemoryLayout::GetPhysicalMemoryRegionTree().InsertDirectly(KernelPhysicalAddressSpaceBase, KernelPhysicalAddressSpaceBase + KernelPhysicalAddressSpaceSize - 1);

        /* Save start and end for ease of use. */
        const uintptr_t code_start_virt_addr = reinterpret_cast<uintptr_t>(_start);
        const uintptr_t code_end_virt_addr   = reinterpret_cast<uintptr_t>(__end__);

        /* Setup the containing kernel region. */
        constexpr size_t KernelRegionSize  = 1_GB;
        constexpr size_t KernelRegionAlign = 1_GB;
        const KVirtualAddress kernel_region_start = util::AlignDown(code_start_virt_addr, KernelRegionAlign);
        size_t kernel_region_size = KernelRegionSize;
        if (!(kernel_region_start + KernelRegionSize - 1 <= KernelVirtualAddressSpaceLast)) {
            kernel_region_size = KernelVirtualAddressSpaceEnd - GetInteger(kernel_region_start);
        }
        MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetVirtualMemoryRegionTree().Insert(GetInteger(kernel_region_start), kernel_region_size, KMemoryRegionType_Kernel));

        /* Setup the code region. */
        constexpr size_t CodeRegionAlign = PageSize;
        const KVirtualAddress code_region_start = util::AlignDown(code_start_virt_addr, CodeRegionAlign);
        const KVirtualAddress code_region_end = util::AlignUp(code_end_virt_addr, CodeRegionAlign);
        const size_t code_region_size = GetInteger(code_region_end) - GetInteger(code_region_start);
        MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetVirtualMemoryRegionTree().Insert(GetInteger(code_region_start), code_region_size, KMemoryRegionType_KernelCode));

        /* Setup board-specific device physical regions. */
        SetupDevicePhysicalMemoryRegions();

        /* Determine the amount of space needed for the misc region. */
        size_t misc_region_needed_size;
        {
            /* Each core has a one page stack for all three stack types (Main, Idle, Exception). */
            misc_region_needed_size = cpu::NumCores * (3 * (PageSize + PageSize));

            /* Account for each auto-map device. */
            for (const auto &region : KMemoryLayout::GetPhysicalMemoryRegionTree()) {
                if (region.HasTypeAttribute(KMemoryRegionAttr_ShouldKernelMap)) {
                    /* Check that the region is valid. */
                    MESOSPHERE_INIT_ABORT_UNLESS(region.GetEndAddress() != 0);

                    /* Account for the region. */
                    const auto aligned_start     = util::AlignDown(region.GetAddress(), PageSize);
                    const auto aligned_end       = util::AlignUp(region.GetLastAddress(), PageSize);
                    const size_t cur_region_size = aligned_end - aligned_start;
                    misc_region_needed_size += cur_region_size;

                    /* Account for alignment requirements. */
                    const size_t min_align = std::min<size_t>(util::GetAlignment(cur_region_size), util::GetAlignment(aligned_start));
                    misc_region_needed_size += min_align >= KernelAslrAlignment ? KernelAslrAlignment : PageSize;
                }
            }

            /* Account for the unknown debug region. */
            misc_region_needed_size += GetMiscUnknownDebugRegionSize();

            /* Multiply the needed size by three, to account for the need for guard space. */
            misc_region_needed_size *= 3;
        }

        /* Decide on the actual size for the misc region. */
        constexpr size_t MiscRegionAlign = KernelAslrAlignment;
        constexpr size_t MiscRegionMinimumSize = 32_MB;
        const size_t misc_region_size = util::AlignUp(std::max(misc_region_needed_size, MiscRegionMinimumSize), MiscRegionAlign);
        MESOSPHERE_INIT_ABORT_UNLESS(misc_region_size > 0);

        /* Setup the misc region. */
        const KVirtualAddress misc_region_start = GetRandomAlignedRegion(misc_region_size, MiscRegionAlign, init_pt, KMemoryLayout::GetVirtualMemoryRegionTree(), KMemoryRegionType_Kernel);
        MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetVirtualMemoryRegionTree().Insert(GetInteger(misc_region_start), misc_region_size, KMemoryRegionType_KernelMisc));

        /* Determine if we'll use extra thread resources. */
        const bool use_extra_resources = KSystemControl::Init::ShouldIncreaseThreadResourceLimit();

        /* Setup the stack region. */
        const size_t stack_region_size = use_extra_resources ? 24_MB : 14_MB;
        constexpr size_t StackRegionAlign = KernelAslrAlignment;
        const KVirtualAddress stack_region_start = GetRandomAlignedRegion(stack_region_size, StackRegionAlign, init_pt, KMemoryLayout::GetVirtualMemoryRegionTree(), KMemoryRegionType_Kernel);
        MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetVirtualMemoryRegionTree().Insert(GetInteger(stack_region_start), stack_region_size, KMemoryRegionType_KernelStack));

        /* Determine the size of the resource region. */
        const size_t resource_region_size = KMemoryLayout::GetResourceRegionSizeForInit(use_extra_resources);

        /* Determine the size of the slab region. */
        const size_t slab_region_size = util::AlignUp(CalculateTotalSlabHeapSize(), PageSize);
        MESOSPHERE_INIT_ABORT_UNLESS(slab_region_size <= resource_region_size);

        /* Setup the slab region. */
        const KPhysicalAddress code_start_phys_addr = init_pt.GetPhysicalAddressOfRandomizedRange(code_start_virt_addr, code_region_size);
        const KPhysicalAddress code_end_phys_addr   = code_start_phys_addr + code_region_size;
        const KPhysicalAddress slab_start_phys_addr = code_end_phys_addr;
        const KPhysicalAddress slab_end_phys_addr   = slab_start_phys_addr + slab_region_size;
        constexpr size_t SlabRegionAlign = KernelAslrAlignment;
        const size_t slab_region_needed_size = util::AlignUp(GetInteger(code_end_phys_addr) + slab_region_size, SlabRegionAlign) - util::AlignDown(GetInteger(code_end_phys_addr), SlabRegionAlign);
        const KVirtualAddress slab_region_start = GetRandomAlignedRegion(slab_region_needed_size, SlabRegionAlign, init_pt, KMemoryLayout::GetVirtualMemoryRegionTree(), KMemoryRegionType_Kernel) + (GetInteger(code_end_phys_addr) % SlabRegionAlign);
        MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetVirtualMemoryRegionTree().Insert(GetInteger(slab_region_start), slab_region_size, KMemoryRegionType_KernelSlab));

        /* Setup the temp region. */
        constexpr size_t TempRegionSize  = 128_MB;
        constexpr size_t TempRegionAlign = KernelAslrAlignment;
        const KVirtualAddress temp_region_start = GetRandomAlignedRegion(TempRegionSize, TempRegionAlign, init_pt, KMemoryLayout::GetVirtualMemoryRegionTree(), KMemoryRegionType_Kernel);
        MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetVirtualMemoryRegionTree().Insert(GetInteger(temp_region_start), TempRegionSize, KMemoryRegionType_KernelTemp));

        /* Automatically map in devices that have auto-map attributes, from largest region to smallest region. */
        {
            /* We want to map the regions from largest to smallest. */
            KMemoryRegion *largest;
            do {
                /* Begin with no knowledge of the largest region. */
                largest = nullptr;

                for (auto &region : KMemoryLayout::GetPhysicalMemoryRegionTree()) {
                    /* We only care about kernel regions. */
                    if (!region.IsDerivedFrom(KMemoryRegionType_Kernel)) {
                        continue;
                    }

                    /* Check whether we should map the region. */
                    if (!region.HasTypeAttribute(KMemoryRegionAttr_ShouldKernelMap)) {
                        continue;
                    }

                    /* If this region has already been mapped, no need to consider it. */
                    if (region.HasTypeAttribute(KMemoryRegionAttr_DidKernelMap)) {
                        continue;
                    }

                    /* Check that the region is valid. */
                    MESOSPHERE_INIT_ABORT_UNLESS(region.GetEndAddress() != 0);

                    /* Update the largest region. */
                    if (largest == nullptr || largest->GetSize() < region.GetSize()) {
                        largest = std::addressof(region);
                    }
                }

                /* If we found a region, map it. */
                if (largest != nullptr) {
                    /* Set the attribute to note we've mapped this region. */
                    largest->SetTypeAttribute(KMemoryRegionAttr_DidKernelMap);

                    /* Create a virtual pair region and insert it into the tree. */
                    const KPhysicalAddress map_phys_addr = util::AlignDown(largest->GetAddress(), PageSize);
                    const size_t map_size  = util::AlignUp(largest->GetEndAddress(), PageSize) - GetInteger(map_phys_addr);
                    const size_t min_align = std::min<size_t>(util::GetAlignment(map_size), util::GetAlignment(GetInteger(map_phys_addr)));
                    const size_t map_align = min_align >= KernelAslrAlignment ? KernelAslrAlignment : PageSize;
                    const KVirtualAddress map_virt_addr = GetRandomAlignedRegionWithGuard(map_size, map_align, init_pt, KMemoryLayout::GetVirtualMemoryRegionTree(), KMemoryRegionType_KernelMisc, PageSize);
                    MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetVirtualMemoryRegionTree().Insert(GetInteger(map_virt_addr), map_size, KMemoryRegionType_KernelMiscMappedDevice));
                    largest->SetPairAddress(GetInteger(map_virt_addr) + largest->GetAddress() - GetInteger(map_phys_addr));

                    /* Map the page in to our page table. */
                    init_pt.Map(map_virt_addr, map_size, map_phys_addr, KernelMmioAttribute, g_initial_page_allocator, 0);
                }
            } while (largest != nullptr);
        }

        /* Setup the basic DRAM regions. */
        SetupDramPhysicalMemoryRegions();

        /* Automatically map in reserved physical memory that has auto-map attributes. */
        {
            /* We want to map the regions from largest to smallest. */
            KMemoryRegion *largest;
            do {
                /* Begin with no knowledge of the largest region. */
                largest = nullptr;

                for (auto &region : KMemoryLayout::GetPhysicalMemoryRegionTree()) {
                    /* We only care about reserved memory. */
                    if (!region.IsDerivedFrom(KMemoryRegionType_DramReservedBase)) {
                        continue;
                    }

                    /* Check whether we should map the region. */
                    if (!region.HasTypeAttribute(KMemoryRegionAttr_ShouldKernelMap)) {
                        continue;
                    }

                    /* If this region has already been mapped, no need to consider it. */
                    if (region.HasTypeAttribute(KMemoryRegionAttr_DidKernelMap)) {
                        continue;
                    }

                    /* Check that the region is valid. */
                    MESOSPHERE_INIT_ABORT_UNLESS(region.GetEndAddress() != 0);

                    /* Update the largest region. */
                    if (largest == nullptr || largest->GetSize() < region.GetSize()) {
                        largest = std::addressof(region);
                    }
                }

                /* If we found a region, map it. */
                if (largest != nullptr) {
                    /* Set the attribute to note we've mapped this region. */
                    largest->SetTypeAttribute(KMemoryRegionAttr_DidKernelMap);

                    /* Create a virtual pair region and insert it into the tree. */
                    const KPhysicalAddress map_phys_addr = util::AlignDown(largest->GetAddress(), PageSize);
                    const size_t map_size  = util::AlignUp(largest->GetEndAddress(), PageSize) - GetInteger(map_phys_addr);
                    const size_t min_align = std::min<size_t>(util::GetAlignment(map_size), util::GetAlignment(GetInteger(map_phys_addr)));
                    const size_t map_align = min_align >= KernelAslrAlignment ? KernelAslrAlignment : PageSize;
                    const KVirtualAddress map_virt_addr = GetRandomAlignedRegionWithGuard(map_size, map_align, init_pt, KMemoryLayout::GetVirtualMemoryRegionTree(), KMemoryRegionType_KernelMisc, PageSize);
                    MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetVirtualMemoryRegionTree().Insert(GetInteger(map_virt_addr), map_size, KMemoryRegionType_KernelMiscUnknownDebug));
                    largest->SetPairAddress(GetInteger(map_virt_addr) + largest->GetAddress() - GetInteger(map_phys_addr));

                    /* Map the page in to our page table. */
                    const auto attribute = largest->HasTypeAttribute(KMemoryRegionAttr_Uncached) ? KernelRwDataUncachedAttribute : KernelRwDataAttribute;
                    init_pt.Map(map_virt_addr, map_size, map_phys_addr, attribute, g_initial_page_allocator, 0);
                }
            } while (largest != nullptr);
        }

        /* Insert a physical region for the kernel code region. */
        MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(GetInteger(code_start_phys_addr), code_region_size, KMemoryRegionType_DramKernelCode));

        /* Insert a physical region for the kernel slab region. */
        MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(GetInteger(slab_start_phys_addr), slab_region_size, KMemoryRegionType_DramKernelSlab));

        /* Map the slab region. */
        init_pt.Map(slab_region_start, slab_region_size, slab_start_phys_addr, KernelRwDataAttribute, g_initial_page_allocator, 0);

        /* Physically randomize the slab region. */
        /* NOTE: Nintendo does this only on 10.0.0+ */
        init_pt.PhysicallyRandomize(slab_region_start, slab_region_size, false);

        /* Insert a physical region for the secure applet memory. */
        const auto secure_applet_end_phys_addr = slab_end_phys_addr + KSystemControl::SecureAppletMemorySize;
        if constexpr (KSystemControl::SecureAppletMemorySize > 0) {
            MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(GetInteger(slab_end_phys_addr), KSystemControl::SecureAppletMemorySize, KMemoryRegionType_DramKernelSecureAppletMemory));
        }

        /* Determine size available for kernel page table heaps. */
        const KPhysicalAddress resource_end_phys_addr = slab_start_phys_addr + resource_region_size;
        g_phase2_resource_end_phys_addr = resource_end_phys_addr;

        const size_t page_table_heap_size = GetInteger(resource_end_phys_addr) - GetInteger(secure_applet_end_phys_addr);

        /* Insert a physical region for the kernel page table heap region */
        MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(GetInteger(secure_applet_end_phys_addr), page_table_heap_size, KMemoryRegionType_DramKernelPtHeap));

        /* All DRAM regions that we haven't tagged by this point will be mapped under the linear mapping. Tag them. */
        for (auto &region : KMemoryLayout::GetPhysicalMemoryRegionTree()) {
            if (region.GetType() == KMemoryRegionType_Dram) {
                /* Check that the region is valid. */
                MESOSPHERE_INIT_ABORT_UNLESS(region.GetEndAddress() != 0);

                /* Set the linear map attribute. */
                region.SetTypeAttribute(KMemoryRegionAttr_LinearMapped);
            }
        }

        /* Get the linear region extents. */
        const auto linear_extents = KMemoryLayout::GetPhysicalMemoryRegionTree().GetDerivedRegionExtents(KMemoryRegionAttr_LinearMapped);
        MESOSPHERE_INIT_ABORT_UNLESS(linear_extents.GetEndAddress() != 0);

        /* Setup the linear mapping region. */
        constexpr size_t LinearRegionAlign = 1_GB;
        const KPhysicalAddress aligned_linear_phys_start = util::AlignDown(linear_extents.GetAddress(), LinearRegionAlign);
        const size_t linear_region_size = util::AlignUp(linear_extents.GetEndAddress(), LinearRegionAlign) - GetInteger(aligned_linear_phys_start);
        const KVirtualAddress linear_region_start = GetRandomAlignedRegionWithGuard(linear_region_size, LinearRegionAlign, init_pt, KMemoryLayout::GetVirtualMemoryRegionTree(), KMemoryRegionType_None, LinearRegionAlign);

        const uintptr_t linear_region_phys_to_virt_diff = GetInteger(linear_region_start) - GetInteger(aligned_linear_phys_start);

        /* Map and create regions for all the linearly-mapped data. */
        {
            uintptr_t cur_phys_addr = 0;
            uintptr_t cur_size = 0;
            for (auto &region : KMemoryLayout::GetPhysicalMemoryRegionTree()) {
                if (!region.HasTypeAttribute(KMemoryRegionAttr_LinearMapped)) {
                    continue;
                }

                MESOSPHERE_INIT_ABORT_UNLESS(region.GetEndAddress() != 0);

                if (cur_size == 0) {
                    cur_phys_addr = region.GetAddress();
                    cur_size      = region.GetSize();
                } else if (cur_phys_addr + cur_size == region.GetAddress()) {
                    cur_size     += region.GetSize();
                } else {
                    const uintptr_t cur_virt_addr = cur_phys_addr + linear_region_phys_to_virt_diff;
                    init_pt.Map(cur_virt_addr, cur_size, cur_phys_addr, KernelRwDataAttribute, g_initial_page_allocator, 0);
                    cur_phys_addr = region.GetAddress();
                    cur_size      = region.GetSize();
                }

                const uintptr_t region_virt_addr = region.GetAddress() + linear_region_phys_to_virt_diff;
                if (!region.HasTypeAttribute(KMemoryRegionAttr_ShouldKernelMap)) {
                    region.SetPairAddress(region_virt_addr);
                }
                MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetVirtualMemoryRegionTree().Insert(region_virt_addr, region.GetSize(), GetTypeForVirtualLinearMapping(region.GetType())));

                KMemoryRegion *virt_region = KMemoryLayout::GetVirtualMemoryRegionTree().FindModifiable(region_virt_addr);
                MESOSPHERE_INIT_ABORT_UNLESS(virt_region != nullptr);
                virt_region->SetPairAddress(region.GetAddress());
            }

            /* Map the last block, which we may have skipped. */
            if (cur_size != 0) {
                const uintptr_t cur_virt_addr = cur_phys_addr + linear_region_phys_to_virt_diff;
                init_pt.Map(cur_virt_addr, cur_size, cur_phys_addr, KernelRwDataAttribute, g_initial_page_allocator, 0);
            }
        }

        /* Clear the slab region. */
        std::memset(GetVoidPointer(slab_region_start), 0, slab_region_size);

        /* NOTE: Unknown function is called here which is ifdef'd out on retail kernel. */
        /* The unknown function is immediately before the function which gets an unknown debug region size, inside this translation unit. */
        /* It's likely that this is some kind of initializer for this unknown debug region. */

        /* Create regions for and map all core-specific stacks. */
        for (size_t i = 0; i < cpu::NumCores; i++) {
            MapStackForCore(init_pt, KMemoryRegionType_KernelMiscMainStack, i);
            MapStackForCore(init_pt, KMemoryRegionType_KernelMiscIdleStack, i);
            MapStackForCore(init_pt, KMemoryRegionType_KernelMiscExceptionStack, i);
        }

        /* Setup the initial arguments. */
        SetupInitialArguments();

        /* Set linear difference for Phase2. */
        g_phase2_linear_region_phys_to_virt_diff = linear_region_phys_to_virt_diff;
    }

    void InitializeCorePhase2() {
        /* Create page table object for use during remaining initialization. */
        KInitialPageTable init_pt;

        /* Unmap the identity mapping. */
        FinalizeIdentityMapping(init_pt, g_initial_page_allocator, g_phase2_linear_region_phys_to_virt_diff);

        /* Finalize the page allocator, we're done allocating at this point. */
        KInitialPageAllocator::State final_init_page_table_state;
        g_initial_page_allocator.GetFinalState(std::addressof(final_init_page_table_state));
        const KPhysicalAddress final_init_page_table_end_address = final_init_page_table_state.end_address;
        const size_t init_page_table_region_size = GetInteger(final_init_page_table_end_address) - GetInteger(g_phase2_resource_end_phys_addr);

        /* Insert regions for the initial page table region. */
        MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(GetInteger(g_phase2_resource_end_phys_addr), init_page_table_region_size, KMemoryRegionType_DramKernelInitPt));
        MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetVirtualMemoryRegionTree().Insert(GetInteger(g_phase2_resource_end_phys_addr) + g_phase2_linear_region_phys_to_virt_diff, init_page_table_region_size, KMemoryRegionType_VirtualDramKernelInitPt));

        /* Insert a physical region for the kernel trace buffer */
        if constexpr (IsKTraceEnabled) {
            const KPhysicalAddress ktrace_buffer_phys_addr = GetInteger(g_phase2_resource_end_phys_addr) + init_page_table_region_size;
            MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(GetInteger(ktrace_buffer_phys_addr), KTraceBufferSize, KMemoryRegionType_KernelTraceBuffer));
            MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().Insert(GetInteger(ktrace_buffer_phys_addr) + g_phase2_linear_region_phys_to_virt_diff, KTraceBufferSize, GetTypeForVirtualLinearMapping(KMemoryRegionType_KernelTraceBuffer)));
        }

        /* All linear-mapped DRAM regions that we haven't tagged by this point will be allocated to some pool partition. Tag them. */
        for (auto &region : KMemoryLayout::GetPhysicalMemoryRegionTree()) {
            constexpr auto UntaggedLinearDram = util::FromUnderlying<KMemoryRegionType>(util::ToUnderlying<KMemoryRegionType>(KMemoryRegionType_Dram) | util::ToUnderlying(KMemoryRegionAttr_LinearMapped));
            if (region.GetType() == UntaggedLinearDram) {
                region.SetType(KMemoryRegionType_DramPoolPartition);
            }
        }

        /* Set the linear memory offsets, to enable conversion between physical and virtual addresses. */
        KMemoryLayout::InitializeLinearMemoryAddresses(g_phase2_linear_region_phys_to_virt_diff);

        /* Set the initial process binary physical address. */
        /* NOTE: Nintendo does this after pool partition setup, but it's a requirement that we do it before */
        /* to retain compatibility with < 5.0.0. */
        const KPhysicalAddress ini_address = g_phase2_initial_process_binary_layout.address;
        MESOSPHERE_INIT_ABORT_UNLESS(ini_address != Null<KPhysicalAddress>);
        SetInitialProcessBinaryPhysicalAddress(ini_address);

        /* Setup all other memory regions needed to arrange the pool partitions. */
        SetupPoolPartitionMemoryRegions();

        /* Validate the initial process binary address. */
        {
            const KMemoryRegion *ini_region = KMemoryLayout::Find(ini_address);

            /* Check that the region is non-kernel dram. */
            MESOSPHERE_INIT_ABORT_UNLESS(ini_region->IsDerivedFrom(KMemoryRegionType_DramUserPool));

            /* Check that the region contains the ini. */
            MESOSPHERE_INIT_ABORT_UNLESS(ini_region->GetAddress() <= GetInteger(ini_address));
            MESOSPHERE_INIT_ABORT_UNLESS(GetInteger(ini_address) + InitialProcessBinarySizeMax <= ini_region->GetEndAddress());
            MESOSPHERE_INIT_ABORT_UNLESS(ini_region->GetEndAddress() != 0);
        }

        /* Cache all linear regions in their own trees for faster access, later. */
        KMemoryLayout::InitializeLinearMemoryRegionTrees();

        /* Turn on all other cores. */
        TurnOnAllCores();
    }

    KInitArguments *GetInitArguments(s32 core_id) {
        return g_init_arguments + core_id;
    }

    void InitializeDebugRegisters() {
        /* Determine how many watchpoints and breakpoints we have */
        cpu::DebugFeatureRegisterAccessor aa64dfr0;
        const auto num_watchpoints = aa64dfr0.GetNumWatchpoints();
        const auto num_breakpoints = aa64dfr0.GetNumBreakpoints();
        cpu::EnsureInstructionConsistencyFullSystem();

        /* Clear the debug monitor register and the os lock access register. */
        cpu::MonitorDebugSystemControlRegisterAccessor(0).Store();
        cpu::EnsureInstructionConsistencyFullSystem();
        cpu::OsLockAccessRegisterAccessor(0).Store();
        cpu::EnsureInstructionConsistencyFullSystem();

        /* Clear all debug watchpoints/breakpoints. */
        #define FOR_I_IN_15_TO_1(HANDLER, ...)                                                                              \
            HANDLER(15, ## __VA_ARGS__) HANDLER(14, ## __VA_ARGS__) HANDLER(13, ## __VA_ARGS__) HANDLER(12, ## __VA_ARGS__) \
            HANDLER(11, ## __VA_ARGS__) HANDLER(10, ## __VA_ARGS__) HANDLER(9,  ## __VA_ARGS__) HANDLER(8,  ## __VA_ARGS__) \
            HANDLER(7,  ## __VA_ARGS__) HANDLER(6,  ## __VA_ARGS__) HANDLER(5,  ## __VA_ARGS__) HANDLER(4,  ## __VA_ARGS__) \
            HANDLER(3,  ## __VA_ARGS__) HANDLER(2,  ## __VA_ARGS__) HANDLER(1,  ## __VA_ARGS__)

        #define MESOSPHERE_INITIALIZE_WATCHPOINT_CASE(ID, ...) \
            case ID:                                           \
                cpu::SetDbgWcr##ID##El1(__VA_ARGS__);          \
                cpu::SetDbgWvr##ID##El1(__VA_ARGS__);          \
            [[fallthrough]];

        #define MESOSPHERE_INITIALIZE_BREAKPOINT_CASE(ID, ...) \
            case ID:                                           \
                cpu::SetDbgBcr##ID##El1(__VA_ARGS__);          \
                cpu::SetDbgBvr##ID##El1(__VA_ARGS__);          \
            [[fallthrough]];


        switch (num_watchpoints) {
            FOR_I_IN_15_TO_1(MESOSPHERE_INITIALIZE_WATCHPOINT_CASE, 0)
            case 0:
                cpu::SetDbgWcr0El1(0);
                cpu::SetDbgWvr0El1(0);
            [[fallthrough]];
            default:
                break;
        }

        switch (num_breakpoints) {
            FOR_I_IN_15_TO_1(MESOSPHERE_INITIALIZE_BREAKPOINT_CASE, 0)
            default:
                break;
        }
        cpu::SetDbgBcr0El1(0);
        cpu::SetDbgBvr0El1(0);

        #undef MESOSPHERE_INITIALIZE_WATCHPOINT_CASE
        #undef MESOSPHERE_INITIALIZE_BREAKPOINT_CASE
        #undef FOR_I_IN_15_TO_1

        cpu::EnsureInstructionConsistencyFullSystem();

        /* Initialize the context id register to all 1s. */
        cpu::ContextIdRegisterAccessor(0).SetProcId(std::numeric_limits<u32>::max()).Store();
        cpu::EnsureInstructionConsistencyFullSystem();

        /* Configure the debug monitor register. */
        cpu::MonitorDebugSystemControlRegisterAccessor(0).SetMde(true).SetTdcc(true).Store();
        cpu::EnsureInstructionConsistencyFullSystem();
    }

    void InitializeExceptionVectors() {
        cpu::SetVbarEl1(reinterpret_cast<uintptr_t>(::ams::kern::ExceptionVectors));
        cpu::SetTpidrEl1(0);
        cpu::SetExceptionThreadStackTop(0);
        cpu::EnsureInstructionConsistencyFullSystem();
    }

    size_t GetMiscUnknownDebugRegionSize() {
        return 0;
    }

}